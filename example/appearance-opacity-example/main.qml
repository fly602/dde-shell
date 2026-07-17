// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import OpacityProbe

import org.deepin.dtk 1.0 as D
import org.deepin.dtk.style 1.0 as DStyle
Window {
    id: root

    width: 760
    height: 320
    x: 100
    y: 100
    minimumWidth: 620
    minimumHeight: 260
    visible: true
    title: qsTr("任务栏同源透明度 Demo")
    color: "transparent"
    palette.window: "white"
    palette.windowText: "black"
    palette.text: "black"
    palette.button: "#f5f5f5"
    palette.buttonText: "black"
    palette.highlight: "#0067c0"
    palette.highlightedText: "white"
    palette.base: "white"
    palette.alternateBase: "#f5f5f5"
    palette.toolTipBase: "white"
    palette.toolTipText: "black"

    readonly property real systemOpacity: opacityMonitor.available ? opacityMonitor.opacity : -1.0
    property int renderMode: demoRenderMode
    readonly property real effectiveOpacity: systemOpacity < 0 ? -1.0 : Math.max(0.2, systemOpacity)
    readonly property bool darkTheme: false
    readonly property color foregroundColor: "black"

    function blendColorAlpha(fallback) {
        return effectiveOpacity < 0 ? fallback : effectiveOpacity
    }
    function logOpacityState(reason) {
        console.info("[opacity-demo]", reason,
                     "available=", opacityMonitor.available,
                     "systemOpacity=", systemOpacity.toFixed(3),
                     "effectiveOpacity=", effectiveOpacity.toFixed(3),
                     "valid=", blurBackground.valid,
                     "blendColor=", blurBackground.blendColor,
                     "blendAlpha=", blurBackground.blendColor.a.toFixed(3),
                     "renderMode=", renderMode,
                     "platform=", Qt.platform.pluginName)
    }

    Component.onCompleted: logOpacityState("completed")
    onSystemOpacityChanged: Qt.callLater(function () { logOpacityState("systemOpacityChanged") })
    onEffectiveOpacityChanged: Qt.callLater(function () { logOpacityState("effectiveOpacityChanged") })

    D.DWindow.enabled: true
    D.DWindow.windowRadius: 0
    D.DWindow.shadowColor: Qt.rgba(0, 0, 0, 0.1)
    D.DWindow.shadowOffset: Qt.point(0, 0)
    D.DWindow.shadowRadius: 40
    D.DWindow.borderWidth: 1
    D.DWindow.enableBlurWindow: Qt.platform.pluginName !== "xcb"
    D.DWindow.themeType: D.ApplicationHelper.LightType
    D.DWindow.borderColor: Qt.rgba(0, 0, 0, 0.15)
    D.ColorSelector.family: D.Palette.CrystalColor

    Item {
        id: dockContainer
        anchors.fill: parent

        D.StyledBehindWindowBlur {
            id: blurBackground
            control: parent
            anchors.fill: parent
            cornerRadius: 0
            blendColor: valid
                        ? Qt.rgba(235 / 255.0, 235 / 255.0, 235 / 255.0,
                                  root.blendColorAlpha(0.6))
                        : DStyle.Style.behindWindowBlur.lightNoBlurColor
            onValidChanged: Qt.callLater(function () { root.logOpacityState("validChanged") })
            onBlendColorChanged: Qt.callLater(function () { root.logOpacityState("blendColorChanged") })
        }

        Loader {
            anchors.fill: parent
            active: root.renderMode !== 0
            sourceComponent: FramebufferProbe {
                expectedAlpha: root.effectiveOpacity
                mode: root.renderMode
            }
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 28
        spacing: 36

        ColumnLayout {
            Layout.preferredWidth: 350
            Layout.fillHeight: true
            spacing: 12

            Label {
                text: opacityMonitor.available
                      ? qsTr("Appearance1 已连接")
                      : qsTr("Appearance1 服务不可用")
                color: opacityMonitor.available ? "#35c46a" : "#ef5350"
                font.bold: true
            }

            Label {
                Layout.fillWidth: true
                text: qsTr("渲染方法：%1")
                          .arg(root.renderMode === 0 ? qsTr("正常渲染")
                               : root.renderMode === 1 ? qsTr("插入空 QSGRenderNode")
                                                       : qsTr("渲染节点执行 glReadPixels"))
                color: root.foregroundColor
                font.pixelSize: 22
                font.bold: true
                wrapMode: Text.Wrap
            }

            ProgressBar {
                Layout.fillWidth: true
                from: 0
                to: 1
                value: Math.max(0, root.effectiveOpacity)
            }

            Label {
                Layout.fillWidth: true
                text: opacityMonitor.available
                      ? qsTr("Appearance1 系统值：%1%（%2）")
                            .arg(Math.round(root.systemOpacity * 100))
                            .arg(root.systemOpacity.toFixed(3))
                      : qsTr("Appearance1 服务不可用")
                color: root.foregroundColor
                opacity: 0.8
            }

            Label {
                Layout.fillWidth: true
                text: qsTr("模糊组件 valid：%1\n平台：%2\n实际 blendColor：%3\n实际 Alpha：%4\n渲染方法：%5")
                          .arg(blurBackground.valid)
                          .arg(Qt.platform.pluginName)
                          .arg(blurBackground.blendColor)
                          .arg(blurBackground.blendColor.a.toFixed(3))
                          .arg(root.renderMode === 0 ? qsTr("正常渲染")
                               : root.renderMode === 1 ? qsTr("插入空 QSGRenderNode")
                                                       : qsTr("渲染节点执行 glReadPixels"))
                color: root.foregroundColor
                opacity: 0.8
            }

            Item { Layout.fillHeight: true }
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            Label {
                anchors.centerIn: parent
                width: parent.width - 24
                text: qsTr("用同一系统透明度分别启动并对比桌面透出程度：\n--render-mode normal\n--render-mode node\n--render-mode readback")
                color: root.foregroundColor
                opacity: 0.75
                font.pixelSize: 18
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
            }
        }
    }
}
