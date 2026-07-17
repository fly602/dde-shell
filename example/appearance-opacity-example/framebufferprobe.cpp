// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "framebufferprobe.h"

#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QQuickWindow>
#include <QSGRenderNode>

#include <cmath>
#include <limits>

namespace {

class FramebufferProbeNode : public QSGRenderNode
{
public:
    void sync(const QRectF &rect, const QSize &framebufferSize, qreal expectedAlpha, int mode)
    {
        m_rect = rect;
        m_framebufferSize = framebufferSize;
        m_expectedAlpha = expectedAlpha;
        if (m_mode != mode)
            m_loggedAlpha = std::numeric_limits<qreal>::quiet_NaN();
        m_mode = mode;
    }

    void render(const RenderState *) override
    {
        if (m_mode != 2)
            return;
        if (qFuzzyCompare(m_expectedAlpha, m_loggedAlpha))
            return;

        auto *context = QOpenGLContext::currentContext();
        if (!context || m_framebufferSize.isEmpty())
            return;

        auto *gl = context->functions();
        while (gl->glGetError() != GL_NO_ERROR) {
        }

        GLint framebuffer = 0;
        GLint sourceRgb = 0;
        GLint destinationRgb = 0;
        GLint sourceAlpha = 0;
        GLint destinationAlpha = 0;
        gl->glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &framebuffer);
        gl->glGetIntegerv(GL_BLEND_SRC_RGB, &sourceRgb);
        gl->glGetIntegerv(GL_BLEND_DST_RGB, &destinationRgb);
        gl->glGetIntegerv(GL_BLEND_SRC_ALPHA, &sourceAlpha);
        gl->glGetIntegerv(GL_BLEND_DST_ALPHA, &destinationAlpha);

        GLubyte pixel[4] = {};
        gl->glReadPixels(m_framebufferSize.width() / 2,
                         m_framebufferSize.height() / 2,
                         1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
        const GLenum error = gl->glGetError();

        qInfo().nospace()
            << "Scene graph probe expectedAlpha=" << m_expectedAlpha
            << " framebuffer=" << framebuffer
            << " blendEnabled=" << bool(gl->glIsEnabled(GL_BLEND))
            << " blendRgb=" << sourceRgb << ',' << destinationRgb
            << " blendAlpha=" << sourceAlpha << ',' << destinationAlpha
            << " pixelRGBA=" << pixel[0] << ',' << pixel[1] << ',' << pixel[2] << ',' << pixel[3]
            << " glError=0x" << Qt::hex << error << Qt::dec;
        m_loggedAlpha = m_expectedAlpha;
    }

    StateFlags changedStates() const override
    {
        return {};
    }

    RenderingFlags flags() const override
    {
        return BoundedRectRendering;
    }

    QRectF rect() const override
    {
        return m_rect;
    }

private:
    QRectF m_rect;
    QSize m_framebufferSize;
    qreal m_expectedAlpha = -1.0;
    qreal m_loggedAlpha = std::numeric_limits<qreal>::quiet_NaN();
    int m_mode = 2;
};

} // namespace

FramebufferProbeItem::FramebufferProbeItem(QQuickItem *parent)
    : QQuickItem(parent)
{
    setFlag(ItemHasContents);
}

qreal FramebufferProbeItem::expectedAlpha() const
{
    return m_expectedAlpha;
}

void FramebufferProbeItem::setExpectedAlpha(qreal expectedAlpha)
{
    if (qFuzzyCompare(m_expectedAlpha, expectedAlpha))
        return;

    m_expectedAlpha = expectedAlpha;
    Q_EMIT expectedAlphaChanged();
    update();
}

int FramebufferProbeItem::mode() const
{
    return m_mode;
}

void FramebufferProbeItem::setMode(int mode)
{
    if (m_mode == mode)
        return;

    m_mode = mode;
    Q_EMIT modeChanged();
    update();
}

QSGNode *FramebufferProbeItem::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
{
    if (!window()
        || m_mode == 0
        || window()->rendererInterface()->graphicsApi() != QSGRendererInterface::OpenGL) {
        delete oldNode;
        return nullptr;
    }

    auto *node = static_cast<FramebufferProbeNode *>(oldNode);
    if (!node)
        node = new FramebufferProbeNode;

    const qreal scale = window()->effectiveDevicePixelRatio();
    node->sync(boundingRect(), QSize(qRound(window()->width() * scale),
                                     qRound(window()->height() * scale)),
               m_expectedAlpha, m_mode);
    return node;
}
