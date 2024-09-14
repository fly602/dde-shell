// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "notifyitem.h"

#include <QDateTime>
#include <QLoggingCategory>

#include "notifyentity.h"
#include "notifyaccessor.h"

namespace notifycenter {
Q_DECLARE_LOGGING_CATEGORY(notifyLog)

AppNotifyItem::AppNotifyItem(const NotifyEntity &entity)
    : m_appId(entity.appName())
{
    setEntity(entity);

    auto pin = NotifyAccessor::instance()->applicationPin(m_entity.appName());
    setPinned(pin);
}

void AppNotifyItem::setEntity(const NotifyEntity &entity)
{
    m_entity = entity;
    refresh();
}

NotifyEntity AppNotifyItem::entity() const
{
    Q_ASSERT(m_entity.isValid());
    return m_entity;
}

NotifyType AppNotifyItem::type() const
{
    return NotifyType::Normal;
}

QString AppNotifyItem::appId() const
{
    return m_appId;
}

QString AppNotifyItem::appName() const
{
    Q_ASSERT(m_entity.isValid());
    return m_entity.appName();
}

QString AppNotifyItem::id() const
{
    Q_ASSERT(m_entity.isValid());
    return m_entity.id();
}

QString AppNotifyItem::time() const
{
    return m_time;
}

void AppNotifyItem::updateTime()
{
    QDateTime time = QDateTime::fromMSecsSinceEpoch(m_entity.time());
    if (!time.isValid())
        return;

    QString ret;
    QDateTime currentTime = QDateTime::currentDateTime();
    auto elapsedDay = time.daysTo(currentTime);
    if (elapsedDay == 0) {
        qint64 msec = QDateTime::currentMSecsSinceEpoch() - m_entity.time();
        auto minute = msec / 1000 / 60;
        if (minute <= 0) {
            ret = tr("Just now");
        } else if (minute > 0 && minute < 60) {
            ret = tr("%1 minutes ago").arg(minute);
        } else {
            ret = tr("%1 hours ago").arg(minute / 60);
        }
    } else if (elapsedDay >= 1 && elapsedDay < 2) {
        ret = tr("Yesterday ") + " " + time.toString("hh:mm");
    } else if (elapsedDay >= 2 && elapsedDay < 7) {
        ret = time.toString("ddd hh:mm");
    } else {
        ret = time.toString("yyyy/MM/dd");
    }

    m_time = ret;
}

bool AppNotifyItem::strongInteractive() const
{
    return m_strongInteractive;
}

QString AppNotifyItem::contentIcon() const
{
    return m_contentIcon;
}

QString AppNotifyItem::defaultAction() const
{
    return m_defaultAction;
}

QVariantList AppNotifyItem::actions() const
{
    return m_actions;
}

void AppNotifyItem::updateActions()
{
    const auto action = m_entity.action();
    if (action.isEmpty())
        return;

    QStringList actions = NotifyEntity::parseAction(action);
    const auto defaultIndex = actions.indexOf(QLatin1String("default"));
    if (defaultIndex >= 0) {
        actions.remove(defaultIndex, 2);
        m_defaultAction = QLatin1String("default");
    }

    QVariantList array;
    for (int i = 0; i < actions.size(); i += 2) {
        const auto id = actions[i];
        const auto text = actions[i + 1];
        QVariantMap item;
        item["id"] = id;
        item["text"] = text;
        array.append(item);
    }

    m_actions = array;
}

void AppNotifyItem::updateStrongInteractive()
{
    QMap<QString, QVariant> hints = NotifyEntity::parseHint(m_entity.hint());
    if (hints.isEmpty())
        return;
    bool ret = false;
    QMap<QString, QVariant>::const_iterator i = hints.constBegin();
    while (i != hints.constEnd()) {
        if (i.key() == QLatin1String("urgency")) {
            ret = i.value().toString() == QLatin1String("SOH");
            break;
        }
        ++i;
    }
    m_strongInteractive = ret;
}

void AppNotifyItem::updateContentIcon()
{
    QMap<QString, QVariant> hints = NotifyEntity::parseHint(m_entity.hint());
    if (hints.isEmpty())
        return;
    QString ret;
    QMap<QString, QVariant>::const_iterator i = hints.constBegin();
    while (i != hints.constEnd()) {
        if (i.key() == QLatin1String("icon")) {
            ret = i.value().toString();
            break;
        }
        ++i;
    }
    m_contentIcon = ret;
}

void AppNotifyItem::refresh()
{
    updateTime();
    updateActions();
    updateStrongInteractive();
    updateContentIcon();
}

bool AppNotifyItem::pinned() const
{
    return m_pinned;
}

void AppNotifyItem::setPinned(bool newPinned)
{
    m_pinned = newPinned;
}

OverlapAppNotifyItem::OverlapAppNotifyItem(const NotifyEntity &entity)
    : AppNotifyItem(entity)
{
}

NotifyType OverlapAppNotifyItem::type()const
{
    return NotifyType::Overlap;
}

void OverlapAppNotifyItem::updateCount(int source)
{
    int count = source - 1;
    if (count > FullCount) {
        m_count = FullCount;
    } else if (count <= EmptyCount) {
        m_count = EmptyCount;
    } else {
        m_count = count;
    }
}

int OverlapAppNotifyItem::count() const
{
    return m_count;
}

bool OverlapAppNotifyItem::isEmpty() const
{
    return m_count <= EmptyCount;
}

AppGroupNotifyItem::AppGroupNotifyItem(const QString &appName)
    : AppNotifyItem(NotifyEntity(QLatin1String("Invalid"), appName))
{
}

NotifyType AppGroupNotifyItem::type() const
{
    return NotifyType::Group;
}

void AppGroupNotifyItem::updateLastEntity(const NotifyEntity &entity)
{
    m_lastEntity = entity;
}

NotifyEntity AppGroupNotifyItem::lastEntity() const
{
    return m_lastEntity;
}
}