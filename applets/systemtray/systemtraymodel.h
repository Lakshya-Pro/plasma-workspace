/***************************************************************************
 *   Copyright (C) 2019 Konrad Materka <materka@gmail.com>                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .        *
 ***************************************************************************/

#ifndef SYSTEMTRAYMODEL_H
#define SYSTEMTRAYMODEL_H

#include <QAbstractListModel>
#include <QList>

#include <KItemModels/KConcatenateRowsProxyModel>
#include <Plasma/DataEngineConsumer>
#include <Plasma/DataEngine>

namespace Plasma {
    class Applet;
}

class BaseModel: public QAbstractListModel
{
    Q_OBJECT
public:
    enum class BaseRole {
        ItemType = Qt::UserRole + 1,
        ItemId,
        CanRender,
        Category,
        Status,
        EffectiveStatus,
        LastBaseRole
    };

    explicit BaseModel(QObject *parent = nullptr);

    QHash<int, QByteArray> roleNames() const override;

public slots:
    void onConfigurationChanged(const KConfigGroup &config);

protected:
    Plasma::Types::ItemStatus calculateEffectiveStatus(bool canRender, Plasma::Types::ItemStatus status, QString itemId) const;

private:
    bool m_showAllItems;
    QStringList m_shownItems;
    QStringList m_hiddenItems;
};

class PlasmoidModel: public BaseModel
{
    Q_OBJECT
public:
    enum class Role {
        Applet = static_cast<int>(BaseModel::BaseRole::LastBaseRole) + 1,
        HasApplet
    };

    explicit PlasmoidModel(QObject *parent = nullptr);

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QHash<int, QByteArray> roleNames() const override;

public slots:
    void addApplet(Plasma::Applet *applet);
    void removeApplet(Plasma::Applet *applet);

private:
    void appendRow(const KPluginMetaData &pluginMetaData);
    int indexOfPluginId(const QString &pluginId) const;

    QList<KPluginMetaData> m_plugins;
    QHash<int, Plasma::Applet*> m_applets;
};

class StatusNotifierModel : public BaseModel, public Plasma::DataEngineConsumer {
    Q_OBJECT
public:
    enum class Role {
        DataEngineSource = static_cast<int>(BaseModel::BaseRole::LastBaseRole) + 100,
        AttentionIcon,
        AttentionIconName,
        AttentionMovieName,
        Category,
        Icon,
        IconName,
        IconThemePath,
        Id,
        ItemIsMenu,
        OverlayIconName,
        Status,
        Title,
        ToolTipSubTitle,
        ToolTipTitle,
        WindowId
    };

    StatusNotifierModel(QObject* parent);

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QHash<int, QByteArray> roleNames() const override;

    Plasma::Service *serviceForSource(const QString &source);

public slots:
    void addSource(const QString &source);
    void removeSource(const QString &source);
    void dataUpdated(const QString &sourceName, const Plasma::DataEngine::Data &data);

private:
    Plasma::DataEngine *m_dataEngine = nullptr;
    QStringList m_sources;
    QHash<QString, Plasma::Service *> m_services;
};

class SystemTrayModel : public KConcatenateRowsProxyModel
{
    Q_OBJECT
public:
    explicit SystemTrayModel(QObject *parent = nullptr);

    QHash<int, QByteArray> roleNames() const override;

    void addSourceModel(QAbstractItemModel *sourceModel);

private:
    QHash<int, QByteArray> m_roleNames;
};

#endif // SYSTEMTRAYMODEL_H
