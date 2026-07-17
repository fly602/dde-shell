// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "framebufferprobe.h"
#include "opacitymonitor.h"

#include <DApplication>
#include <DGuiApplicationHelper>
#include <QDir>
#include <QCommandLineOption>
#include <QDateTime>
#include <QCommandLineParser>
#include <QDebug>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QScreen>
#include <QQuickWindow>
#include <QTimer>
#include <qqml.h>

namespace {

QString optionValue(const QCommandLineParser &parser,
                    const QCommandLineOption &option,
                    const char *environmentName,
                    const QString &fallback)
{
    if (parser.isSet(option))
        return parser.value(option);
    if (qEnvironmentVariableIsSet(environmentName))
        return qEnvironmentVariable(environmentName);
    return fallback;
}

bool parseBoolean(const QString &text, bool *value)
{
    const QString normalized = text.trimmed().toLower();
    if (normalized == QLatin1String("1") || normalized == QLatin1String("true")
        || normalized == QLatin1String("on") || normalized == QLatin1String("yes")) {
        *value = true;
        return true;
    }
    if (normalized == QLatin1String("0") || normalized == QLatin1String("false")
        || normalized == QLatin1String("off") || normalized == QLatin1String("no")) {
        *value = false;
        return true;
    }
    return false;
}

bool parseRenderMode(const QString &text, int *mode)
{
    const QString normalized = text.trimmed().toLower();
    if (normalized == QLatin1String("normal")) {
        *mode = 0;
        return true;
    }
    if (normalized == QLatin1String("node")) {
        *mode = 1;
        return true;
    }
    if (normalized == QLatin1String("readback")) {
        *mode = 2;
        return true;
    }
    return false;
}

} // namespace

int main(int argc, char *argv[])
{
    Dtk::Gui::DGuiApplicationHelper::setAttribute(
        Dtk::Gui::DGuiApplicationHelper::UseInactiveColorGroup, false);
    Dtk::Widget::DApplication app(argc, argv);

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("任务栏透明度渲染链路对比 Demo"));
    parser.addHelpOption();
    const QCommandLineOption renderModeOption(
        QStringList{QStringLiteral("render-mode")},
        QStringLiteral("渲染方法：normal（正常渲染）、node（插入空 QSGRenderNode）、readback（渲染节点执行 glReadPixels）。环境变量：OPACITY_DEMO_RENDER_MODE"),
        QStringLiteral("normal|node|readback"));
    const QCommandLineOption frameGrabOption(
        QStringList{QStringLiteral("frame-grab")},
        QStringLiteral("是否启用 QQuickWindow::grabWindow：on/off。环境变量：OPACITY_DEMO_FRAME_GRAB"),
        QStringLiteral("on|off"));
    const QCommandLineOption screenGrabDirOption(
        QStringList{QStringLiteral("screen-grab-dir")},
        QStringLiteral("抓取桌面最终合成画面到指定目录。环境变量：OPACITY_DEMO_SCREEN_GRAB_DIR"),
        QStringLiteral("directory"));
    parser.addOptions({renderModeOption, frameGrabOption, screenGrabDirOption});
    parser.process(app);

    int renderMode = 0;
    const QString renderModeText = optionValue(parser, renderModeOption,
                                               "OPACITY_DEMO_RENDER_MODE", QStringLiteral("normal"));
    if (!parseRenderMode(renderModeText, &renderMode)) {
        qCritical().noquote() << "render-mode 必须是 normal、node 或 readback：" << renderModeText;
        return 2;
    }

    bool frameGrabEnabled = false;
    const QString frameGrabText = optionValue(parser, frameGrabOption,
                                              "OPACITY_DEMO_FRAME_GRAB", QStringLiteral("off"));
    if (!parseBoolean(frameGrabText, &frameGrabEnabled)) {
        qCritical().noquote() << "无效的 frame-grab 值:" << frameGrabText;
        return 2;
    }

    const QString screenGrabDir = optionValue(parser, screenGrabDirOption,
                                              "OPACITY_DEMO_SCREEN_GRAB_DIR", QString());
    if (!screenGrabDir.isEmpty() && !QDir().mkpath(screenGrabDir)) {
        qCritical().noquote() << "无法创建 screen-grab-dir:" << screenGrabDir;
        return 2;
    }


    qInfo().nospace()
        << "A/B configuration renderMode=" << renderModeText
        << " frameGrab=" << frameGrabEnabled
        << " screenGrabDir=" << screenGrabDir
        << " qsgRhiBackend=" << qEnvironmentVariable("QSG_RHI_BACKEND", "default");

    OpacityMonitor opacityMonitor;
    qmlRegisterType<FramebufferProbeItem>("OpacityProbe", 1, 0, "FramebufferProbe");
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("opacityMonitor"), &opacityMonitor);
    engine.rootContext()->setContextProperty(QStringLiteral("demoRenderMode"), renderMode);
    engine.load(QUrl(QStringLiteral("qrc:/org/deepin/appearance-opacity-example/main.qml")));
    if (engine.rootObjects().isEmpty())
        return -1;

    auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().constFirst());
    if (!window)
        return -1;

    QTimer frameGrabTimer;
    frameGrabTimer.setSingleShot(true);
    frameGrabTimer.setInterval(200);
    if (frameGrabEnabled) {
        QObject::connect(window, SIGNAL(effectiveOpacityChanged()),
                         &frameGrabTimer, SLOT(start()));
        QObject::connect(&frameGrabTimer, &QTimer::timeout, window, [window] {
            const auto image = window->grabWindow();
            const auto center = image.pixelColor(image.width() / 2, image.height() / 2);
            qInfo() << "Frame grab probe expectedAlpha="
                    << window->property("effectiveOpacity").toReal()
                    << "format=" << image.format() << "center=" << center;
        });
        frameGrabTimer.start();
    }

    QTimer screenGrabTimer;
    qsizetype screenGrabIndex = 0;
    screenGrabTimer.setSingleShot(true);
    screenGrabTimer.setInterval(450);
    if (!screenGrabDir.isEmpty()) {
        QObject::connect(window, SIGNAL(effectiveOpacityChanged()),
                         &screenGrabTimer, SLOT(start()));
        QObject::connect(&screenGrabTimer, &QTimer::timeout, window,
                         [window, &screenGrabIndex, screenGrabDir, renderModeText] {
            const QRect geometry(window->position(), window->size());
            QScreen *screen = window->screen();
            const QPixmap pixmap = screen->grabWindow(0, geometry.x(), geometry.y(),
                                                      geometry.width(), geometry.height());
            const QImage image = pixmap.toImage();
            const qreal expectedAlpha = window->property("effectiveOpacity").toReal();
            const QString fileName = QStringLiteral("mode-%1-sample-%2-alpha-%3.png")
                                         .arg(renderModeText)
                                         .arg(++screenGrabIndex, 2, 10, QLatin1Char('0'))
                                         .arg(expectedAlpha, 0, 'f', 3);
            const QString filePath = QDir(screenGrabDir).filePath(fileName);
            const bool saved = pixmap.save(filePath, "PNG");
            const QColor center = image.isNull()
                ? QColor()
                : image.pixelColor(image.width() / 2, image.height() / 2);
            const QColor upperRight = image.isNull()
                ? QColor()
                : image.pixelColor(image.width() * 3 / 4, image.height() / 4);
            qInfo() << "Screen grab probe"
                    << "sample=" << screenGrabIndex
                    << "expectedAlpha=" << expectedAlpha
                    << "windowGeometry=" << geometry
                    << "screen=" << screen->name()
                    << "devicePixelRatio=" << pixmap.devicePixelRatio()
                    << "imageSize=" << image.size()
                    << "center=" << center
                    << "upperRight=" << upperRight
                    << "saved=" << saved
                    << "file=" << filePath;
        });
        screenGrabTimer.start();
    }


    return app.exec();
}
