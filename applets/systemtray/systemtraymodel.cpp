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

#include "systemtraymodel.h"
#include "debug.h"

#include <QIcon>
#include <QQuickItem>

#include <Plasma/Applet>
#include <Plasma/DataContainer>
#include <Plasma/Service>
#include <PluginLoader>

#include <KLocalizedString>

BaseModel::BaseModel(QObject *parent)
    : QAbstractListModel(parent),
      m_showAllItems(false)
{
}

QHash<int, QByteArray> BaseModel::roleNames() const
{
    return {
        {Qt::DisplayRole, QByteArrayLiteral("display")},
        {Qt::DecorationRole, QByteArrayLiteral("decoration")},
        {static_cast<int>(BaseRole::ItemType), QByteArrayLiteral("itemType")},
        {static_cast<int>(BaseRole::ItemId), QByteArrayLiteral("itemId")},
        {static_cast<int>(BaseRole::CanRender), QByteArrayLiteral("canRender")},
        {static_cast<int>(BaseRole::Category), QByteArrayLiteral("category")},
        {static_cast<int>(BaseRole::Status), QByteArrayLiteral("status")},
        {static_cast<int>(BaseRole::EffectiveStatus), QByteArrayLiteral("effectiveStatus")}
    };
}

void BaseModel::onConfigurationChanged(const KConfigGroup &config)
{
    if (!config.isValid()) {
        return;
    }

    const KConfigGroup generalGroup = config.group("General");

    m_showAllItems = generalGroup.readEntry("showAllItems", false);
    m_shownItems = generalGroup.readEntry("shownItems", QStringList());
    m_hiddenItems = generalGroup.readEntry("hiddenItems", QStringList());

    for (int i = 0; i < rowCount(); i++) {
        dataChanged(index(i, 0), index(i, 0), {static_cast<int>(BaseModel::BaseRole::EffectiveStatus)});
    }
}

Plasma::Types::ItemStatus BaseModel::calculateEffectiveStatus(bool canRender, Plasma::Types::ItemStatus status, QString itemId) const
{
    if (!canRender) {
        return Plasma::Types::ItemStatus::HiddenStatus;
    }

    if (status == Plasma::Types::ItemStatus::HiddenStatus) {
        return Plasma::Types::ItemStatus::HiddenStatus;
    }

    bool forcedShown = m_showAllItems || m_shownItems.contains(itemId);
    bool forcedHidden = m_hiddenItems.contains(itemId);

    if (forcedShown || (!forcedHidden && status != Plasma::Types::ItemStatus::PassiveStatus)) {
        return Plasma::Types::ItemStatus::ActiveStatus;
    } else {
        return Plasma::Types::ItemStatus::PassiveStatus;
    }
}


static QString plasmoidCategoryForMetadata(const KPluginMetaData &metadata)
{
    QString category = QStringLiteral("UnknownCategory");

    if (metadata.isValid()) {
        const QString notificationAreaCategory = metadata.value(QStringLiteral("X-Plasma-NotificationAreaCategory"));
        if (!notificationAreaCategory.isEmpty()) {
            category = notificationAreaCategory;
        }
    }

    return category;
}

PlasmoidModel::PlasmoidModel(QObject *parent) : BaseModel(parent)
{
    for (const auto &info : Plasma::PluginLoader::self()->listAppletMetaData(QString())) {
        if (!info.isValid() || info.value(QStringLiteral("X-Plasma-NotificationArea")) != "true") {
            continue;
        }
        appendRow(info);
    }
}

QVariant PlasmoidModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= rowCount() || index.column() != 0) {
        return QVariant();
    }

    const KPluginMetaData &pluginMetaData = m_plugins[index.row()];
    Plasma::Applet *applet = m_applets.value(index.row(), nullptr);

    if (role <= Qt::UserRole) {
        switch (role) {
        case Qt::DisplayRole: {
            QString name = pluginMetaData.name();
            const QString dbusactivation = pluginMetaData.value(QStringLiteral("X-Plasma-DBusActivationService"));
            if (!dbusactivation.isEmpty()) {
                name += i18n(" (Automatic load)");
            }
            return name;
        }
        case Qt::DecorationRole: {
            QIcon icon = QIcon::fromTheme(pluginMetaData.iconName());
            return icon.isNull() ? QVariant() : icon;
        }
        default:
            return QVariant();
        }
    }

    if (role < static_cast<int>(Role::Applet)) {
        Plasma::Types::ItemStatus status = Plasma::Types::ItemStatus::UnknownStatus;
        if (applet) {
            status = applet->status();
        }

        switch (static_cast<BaseRole>(role)) {
        case BaseRole::ItemType: return QStringLiteral("Plasmoid");
        case BaseRole::ItemId: return pluginMetaData.pluginId();
        case BaseRole::CanRender: return applet != nullptr;
        case BaseRole::Category: return plasmoidCategoryForMetadata(pluginMetaData);
        case BaseRole::Status: return status;
        case BaseRole::EffectiveStatus: return calculateEffectiveStatus(applet != nullptr, status, pluginMetaData.pluginId());
        default: return QVariant();
        }
    }

    switch (static_cast<Role>(role)) {
    case Role::Applet: return applet ? applet->property("_plasma_graphicObject") : QVariant();
    case Role::HasApplet: return applet != nullptr;
    default: return QVariant();
    }
}

int PlasmoidModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_plugins.size();
}

QHash<int, QByteArray> PlasmoidModel::roleNames() const
{
    QHash<int, QByteArray> roles = BaseModel::roleNames();

    roles.insert(static_cast<int>(Role::Applet), QByteArrayLiteral("applet"));
    roles.insert(static_cast<int>(Role::HasApplet), QByteArrayLiteral("hasApplet"));

    return roles;
}

void PlasmoidModel::addApplet(Plasma::Applet *applet)
{
    auto pluginMetaData = applet->pluginMetaData();

    int idx = indexOfPluginId(pluginMetaData.pluginId());

    if (idx < 0) {
        idx = rowCount();
        appendRow(pluginMetaData);
    }

    m_applets.insert(idx, applet);
    connect(applet, &Plasma::Applet::statusChanged, this, [this, applet] (Plasma::Types::ItemStatus status) {
        Q_UNUSED(status)
        int idx = indexOfPluginId(applet->pluginMetaData().pluginId());
        dataChanged(index(idx, 0), index(idx, 0), {static_cast<int>(BaseRole::Status)});
    });

    dataChanged(index(idx, 0), index(idx, 0));
}

void PlasmoidModel::removeApplet(Plasma::Applet *applet)
{
    int idx = indexOfPluginId(applet->pluginMetaData().pluginId());
    if (idx > 0) {
        applet->disconnect(this);
        m_applets.remove(idx);
        dataChanged(index(idx, 0), index(idx, 0));
    }
}

void PlasmoidModel::appendRow(const KPluginMetaData &pluginMetaData)
{
    int idx = rowCount();
    beginInsertRows(QModelIndex(), idx, idx);
    m_plugins.append(pluginMetaData);
    endInsertRows();
}

int PlasmoidModel::indexOfPluginId(const QString &pluginId) const {
    for (int i = 0; i < rowCount(); i++) {
        if (m_plugins[i].pluginId() == pluginId) {
            return i;
        }
    }
    return -1;
}


StatusNotifierModel::StatusNotifierModel(QObject *parent) : BaseModel(parent)
{
    m_dataEngine = dataEngine(QStringLiteral("statusnotifieritem"));

    connect(m_dataEngine, &Plasma::DataEngine::sourceAdded, this, &StatusNotifierModel::addSource);
    connect(m_dataEngine, &Plasma::DataEngine::sourceRemoved, this, &StatusNotifierModel::removeSource);

    m_dataEngine->connectAllSources(this);
}

static Plasma::Types::ItemStatus status(const Plasma::DataEngine::Data &sniData)
{
    QString status = sniData.value("Status").toString();
    if (status == QLatin1String("Active")) {
        return Plasma::Types::ItemStatus::ActiveStatus;
    } else if (status == QLatin1String("NeedsAttention")) {
        return Plasma::Types::ItemStatus::NeedsAttentionStatus;
    } else if (status == QLatin1String("Passive")) {
        return Plasma::Types::ItemStatus::PassiveStatus;
    } else {
        return Plasma::Types::ItemStatus::UnknownStatus;
    }
}

QVariant StatusNotifierModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= rowCount() || index.column() != 0) {
        return QVariant();
    }

    QString source = m_sources[index.row()];
    Plasma::DataContainer *dataContainer = m_dataEngine->containerForSource(source);
    const Plasma::DataEngine::Data &sniData = dataContainer->data();

    if (role <= Qt::UserRole) {
        switch (role) {
        case Qt::DisplayRole:
            return sniData.value("Title");
        case Qt::DecorationRole: {
            QVariant icon = sniData.value("Icon");
            if (icon.isValid() && icon.canConvert<QIcon>() && !icon.value<QIcon>().isNull()) {
                return icon.value<QIcon>();
            } else {
                return sniData.value("IconName");
            }
        }
        default:
            return QVariant();
        }
    }

    if (role < static_cast<int>(Role::DataEngineSource)) {
        switch (static_cast<BaseRole>(role)) {
        case BaseRole::ItemType: return QStringLiteral("StatusNotifier");
        case BaseRole::ItemId: return sniData.value("Id");
        case BaseRole::CanRender: return true;
        case BaseRole::Category: {
            QVariant category = sniData.value("Category");
            return  category.isNull() ? QStringLiteral("UnknownCategory") : sniData.value("Category");
        }
        case BaseRole::Status: {
            return status(sniData);
        }
        case BaseRole::EffectiveStatus: {
            return calculateEffectiveStatus(true, status(sniData), sniData.value("Id").toString());
        }
        default: return QVariant();
        }
    }

    switch (static_cast<Role>(role)) {
    case Role::DataEngineSource: return source;
    case Role::AttentionIcon: {
        QVariant attentionIcon = sniData.value("AttentionIcon");
        if (attentionIcon.isValid() && attentionIcon.canConvert<QIcon>() && !attentionIcon.value<QIcon>().isNull()) {
            return attentionIcon;
        } else {
            return QVariant();
        }
    }
    case Role::AttentionIconName: return sniData.value("AttentionIconName");
    case Role::AttentionMovieName: return sniData.value("AttentionMovieName");
    case Role::Category: return sniData.value("Category"); // TODO: remove?
    case Role::Icon: {
        QVariant icon = sniData.value("Icon");
        if (icon.isValid() && icon.canConvert<QIcon>() && !icon.value<QIcon>().isNull()) {
            return icon.value<QIcon>();
        } else {
            return QVariant();
        }
    }
    case Role::IconName: return sniData.value("IconName");
    case Role::IconThemePath: return sniData.value("IconThemePath");
    case Role::Id: return sniData.value("Id");
    case Role::ItemIsMenu: return sniData.value("ItemIsMenu");
    case Role::OverlayIconName: return sniData.value("OverlayIconName");
    case Role::Status: return status(sniData); // TODO: remove?
    case Role::Title: return sniData.value("Title");
    case Role::ToolTipSubTitle: return sniData.value("ToolTipSubTitle");
    case Role::ToolTipTitle: return sniData.value("ToolTipTitle");
    case Role::WindowId: return sniData.value("WindowId");
    default: return QVariant();
    }
}

int StatusNotifierModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_sources.size();
}

QHash<int, QByteArray> StatusNotifierModel::roleNames() const
{
    QHash<int, QByteArray> roles = BaseModel::roleNames();

    roles.insert(static_cast<int>(Role::DataEngineSource), QByteArrayLiteral("DataEngineSource"));
    roles.insert(static_cast<int>(Role::AttentionIcon), QByteArrayLiteral("AttentionIcon"));
    roles.insert(static_cast<int>(Role::AttentionIconName), QByteArrayLiteral("AttentionIconName"));
    roles.insert(static_cast<int>(Role::AttentionMovieName), QByteArrayLiteral("AttentionMovieName"));
    roles.insert(static_cast<int>(Role::Category), QByteArrayLiteral("Category"));
    roles.insert(static_cast<int>(Role::Icon), QByteArrayLiteral("Icon"));
    roles.insert(static_cast<int>(Role::IconName), QByteArrayLiteral("IconName"));
    roles.insert(static_cast<int>(Role::IconThemePath), QByteArrayLiteral("IconThemePath"));
    roles.insert(static_cast<int>(Role::Id), QByteArrayLiteral("Id"));
    roles.insert(static_cast<int>(Role::ItemIsMenu), QByteArrayLiteral("ItemIsMenu"));
    roles.insert(static_cast<int>(Role::OverlayIconName), QByteArrayLiteral("OverlayIconName"));
    roles.insert(static_cast<int>(Role::Status), QByteArrayLiteral("Status"));
    roles.insert(static_cast<int>(Role::Title), QByteArrayLiteral("Title"));
    roles.insert(static_cast<int>(Role::ToolTipSubTitle), QByteArrayLiteral("ToolTipSubTitle"));
    roles.insert(static_cast<int>(Role::ToolTipTitle), QByteArrayLiteral("ToolTipTitle"));
    roles.insert(static_cast<int>(Role::WindowId), QByteArrayLiteral("WindowId"));

    return roles;
}

Plasma::Service *StatusNotifierModel::serviceForSource(const QString &source)
{
    if (m_services.contains(source)) {
        return m_services.value(source);
    }

    Plasma::Service *service = m_dataEngine->serviceForSource(source);
    if (!service) {
        return nullptr;
    }
    m_services[source] = service;
    return service;
}

void StatusNotifierModel::addSource(const QString &source)
{
    m_dataEngine->connectSource(source, this);
}

void StatusNotifierModel::removeSource(const QString &source)
{
    m_dataEngine->disconnectSource(source, this);
    if (m_sources.contains(source)) {
        int idx = m_sources.indexOf(source);
        beginRemoveRows(QModelIndex(), idx, idx);
        m_sources.removeAll(source);
        endRemoveRows();
    }

    QHash<QString, Plasma::Service *>::iterator it = m_services.find(source);
    if (it != m_services.end()) {
        delete it.value();
        m_services.erase(it);
    }
}

void StatusNotifierModel::dataUpdated(const QString &sourceName, const Plasma::DataEngine::Data &data)
{
    Q_UNUSED(data)

    if (m_sources.contains(sourceName)) {
        int idx = m_sources.indexOf(sourceName);
        dataChanged(index(idx, 0), index(idx, 0));
    } else {
        int count = rowCount();
        beginInsertRows(QModelIndex(), count, count);
        m_sources.append(sourceName);
        endInsertRows();
    }
}


SystemTrayModel::SystemTrayModel(QObject *parent) : KConcatenateRowsProxyModel(parent)
{
    m_roleNames = KConcatenateRowsProxyModel::roleNames();
}

QHash<int, QByteArray> SystemTrayModel::roleNames() const
{
    return m_roleNames;
}

void SystemTrayModel::addSourceModel(QAbstractItemModel *sourceModel)
{
    QHashIterator<int, QByteArray> it(sourceModel->roleNames());
    while (it.hasNext()) {
        it.next();

        if (!m_roleNames.contains(it.key())) {
            m_roleNames.insert(it.key(), it.value());
        }
    }

    KConcatenateRowsProxyModel::addSourceModel(sourceModel);
}
