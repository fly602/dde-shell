// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "opacitymonitor.h"

#include <QDBusConnection>
#include <QDBusError>
#include <QDBusServiceWatcher>
#include <QDebug>

namespace {
constexpr auto AppearanceService = "org.deepin.dde.Appearance1";
constexpr auto AppearancePath = "/org/deepin/dde/Appearance1";
}

OpacityMonitor::OpacityMonitor(QObject *parent)
    : QObject(parent)
    , m_serviceWatcher(new QDBusServiceWatcher(QString::fromLatin1(AppearanceService),
                                               QDBusConnection::sessionBus(),
                                               QDBusServiceWatcher::WatchForRegistration
                                                   | QDBusServiceWatcher::WatchForUnregistration,
                                               this))
{
    connect(m_serviceWatcher, &QDBusServiceWatcher::serviceRegistered,
            this, &OpacityMonitor::connectToService);
    connect(m_serviceWatcher, &QDBusServiceWatcher::serviceUnregistered,
            this, &OpacityMonitor::disconnectFromService);

    connectToService();
}

qreal OpacityMonitor::opacity() const
{
    return m_opacity;
}

bool OpacityMonitor::available() const
{
    return m_available;
}

void OpacityMonitor::connectToService()
{
    m_interface.reset(new org::deepin::dde::Appearance1(QString::fromLatin1(AppearanceService),
                                                        QString::fromLatin1(AppearancePath),
                                                        QDBusConnection::sessionBus()));
    if (!m_interface->isValid()) {
        qWarning() << "Failed to connect to Appearance1:" << m_interface->lastError();
        disconnectFromService();
        return;
    }

    connect(m_interface.data(), &org::deepin::dde::Appearance1::OpacityChanged,
            this, &OpacityMonitor::updateOpacity);
    updateOpacity(m_interface->opacity());
    updateAvailable(true);
}

void OpacityMonitor::disconnectFromService()
{
    m_interface.reset();
    updateAvailable(false);
    updateOpacity(-1.0);
}

void OpacityMonitor::updateOpacity(qreal opacity)
{
    if (qFuzzyCompare(m_opacity, opacity))
        return;

    m_opacity = opacity;
    qInfo() << "System opacity changed:" << m_opacity;
    Q_EMIT opacityChanged();
}

void OpacityMonitor::updateAvailable(bool available)
{
    if (m_available == available)
        return;

    m_available = available;
    Q_EMIT availableChanged();
}
