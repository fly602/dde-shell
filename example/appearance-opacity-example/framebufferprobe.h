// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef FRAMEBUFFERPROBE_H
#define FRAMEBUFFERPROBE_H

#include <QQuickItem>

class FramebufferProbeItem : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(qreal expectedAlpha READ expectedAlpha WRITE setExpectedAlpha NOTIFY expectedAlphaChanged FINAL)
    Q_PROPERTY(int mode READ mode WRITE setMode NOTIFY modeChanged FINAL)

public:
    explicit FramebufferProbeItem(QQuickItem *parent = nullptr);

    qreal expectedAlpha() const;
    void setExpectedAlpha(qreal expectedAlpha);
    int mode() const;
    void setMode(int mode);

Q_SIGNALS:
    void expectedAlphaChanged();
    void modeChanged();

protected:
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *) override;

private:
    qreal m_expectedAlpha = -1.0;
    int m_mode = 2;
};

#endif
