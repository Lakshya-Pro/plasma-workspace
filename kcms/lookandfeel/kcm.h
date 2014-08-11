/*
   Copyright (c) 2014 Marco Martin <mart@kde.org>
   Copyright (c) 2014 Vishesh Handa <me@vhanda.in>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef _KCM_SEARCH_H
#define _KCM_SEARCH_H

#include <KCModule>
#include <KConfig>
#include <KConfigGroup>
#include <QListWidget>

#include "../../lookandfeelaccess/lookandfeelaccess.h"

class QQuickWidget;
class QStandardItemModel;

class KCMLookandFeel : public KCModule
{
    Q_OBJECT
    Q_PROPERTY(QStandardItemModel *lookAndFeelModel READ lookAndFeelModel CONSTANT)
    Q_PROPERTY(QString selectedPlugin READ selectedPlugin WRITE setSelectedPlugin NOTIFY selectedPluginChanged)

public:
    enum Roles {
        PluginNameRole = Qt::UserRole +1,
        ScreenhotRole,
        HasSplashRole,
        HasLockScreenRole,
        HasRunCommandRole,
        HasLogoutRole,
        HasColorsRole,
        HasWidgetStyleRole,
        HasIconsRole
    };
    KCMLookandFeel(QWidget* parent, const QVariantList& args);

    QStandardItemModel *lookAndFeelModel();

    QString selectedPlugin() const;
    void setSelectedPlugin(const QString &plugin);

public Q_SLOTS:
    void load();
    void save();
    void defaults();

Q_SIGNALS:
    void selectedPluginChanged();

private:
    QQuickWidget *m_quickWidget;
    QStandardItemModel *m_model;
    LookAndFeelAccess m_access;
    QString m_selectedPlugin;

    KConfig m_config;
    KConfigGroup m_configGroup;
};

#endif
