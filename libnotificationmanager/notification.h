/*
 * Copyright 2018-2019 Kai Uwe Broulik <kde@privat.broulik.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) version 3, or any
 * later version accepted by the membership of KDE e.V. (or its
 * successor approved by the membership of KDE e.V.), which shall
 * act as a proxy defined in Section 6 of version 3 of the license.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <QDateTime>
#include <QImage>
#include <QString>
#include <QStringList>
#include <QUrl>

#include "notifications.h"

//#include "notificationmanager_export.h"

namespace NotificationManager
{

/**
 * @short Represents a single notification
 *
 * TODO
 *
 * @author Kai Uwe Broulik <kde@privat.broulik.de>
 **/
class Notification
{
public:
    explicit Notification(uint id = 0);
    ~Notification();

    /*enum class Urgency {
        Low = 0,
        Normal = 1,
        Critical = 2
    };*/

    uint id() const;

    QDateTime created() const;

    QDateTime updated() const;
    void setUpdated();

    QString summary() const;
    void setSummary(const QString &summary);

    QString body() const;
    void setBody(const QString &body);

    QString iconName() const;
    void setIconName(const QString &iconName);

    QImage image() const;
    void setImage(const QImage &image);

    QString applicationName() const;
    void setApplicationName(const QString &applicationName);

    QString applicationIconName() const;
    void setApplicationIconName(const QString &applicationIconName);

    QStringList actionNames() const;
    QStringList actionLabels() const;
    bool hasDefaultAction() const;
    void setActions(const QStringList &actions);

    // QVector?
    QList<QUrl> urls() const;
    void setUrls(const QList<QUrl> &urls);

    Notifications::Urgencies urgency() const;
    void setUrgency(Notifications::Urgencies urgency);

    int timeout() const;
    void setTimeout(int timeout);

    bool configurable() const;
    QString configureActionLabel() const;

    bool expired() const;
    void setExpired(bool expired);

    bool dismissed() const;
    void setDismissed(bool dismissed);

    void processHints(const QVariantMap &hints);

    bool operator==(const Notification &other) const;

    friend class NotificationModel;

private:
    uint m_id = 0;
    QDateTime m_created;
    QDateTime m_updated;

    QString m_summary;
    QString m_body;
    QString m_iconName;
    QImage m_image;

    QString m_applicationName;
    QString m_applicationIconName;

    QStringList m_actionNames;
    QStringList m_actionLabels;
    bool m_hasDefaultAction = false;

    bool m_hasConfigureAction = false;
    QString m_configureActionLabel;

    bool m_configurableNotifyRc = false;
    QString m_notifyRcName;
    QString m_eventId;

    QList<QUrl> m_urls;

    Notifications::Urgencies m_urgency = Notifications::NormalUrgency;
    int m_timeout = -1;

    bool m_expired = false;
    bool m_dismissed = false;

};

} // namespace NotificationManager
