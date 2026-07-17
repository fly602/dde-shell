// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "Appearance1.h"

#include <QScopedPointer>
#include <QObject>

class QDBusServiceWatcher;

class OpacityMonitor : public QObject
{
    Q_OBJECT
    Q_PROPERTY(qreal opacity READ opacity NOTIFY opacityChanged FINAL)
    Q_PROPERTY(bool available READ available NOTIFY availableChanged FINAL)

public:
    explicit OpacityMonitor(QObject *parent = nullptr);

    qreal opacity() const;
    bool available() const;

Q_SIGNALS:
    void opacityChanged();
    void availableChanged();

private:
    void connectToService();
    void disconnectFromService();
    void updateOpacity(qreal opacity);
    void updateAvailable(bool available);

    QDBusServiceWatcher *m_serviceWatcher = nullptr;
    QScopedPointer<org::deepin::dde::Appearance1> m_interface;
    qreal m_opacity = -1.0;
    bool m_available = false;
};
