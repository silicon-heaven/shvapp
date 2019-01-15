#include "sitesmodel.h"

#include <shv/coreqt/log.h>

QVariant SitesModel::value(int row, const QString &role_name) const
{
	QByteArray nm = role_name.toUtf8();
	const QHash<int, QByteArray> nms = roleNames();
	for (int i = Qt::UserRole+1; i < SiteRoles::COUNT; ++i) {
		if(nms.value(i) == nm) {
			return data(createIndex(row, 0), i);
		}
	}
	return QVariant();
}

QVariant SitesModel::value(int row, int role) const
{
	return data(createIndex(row, 0), role);
}

void SitesModel::clear()
{
	m_sites.clear();
}

QHash<int, QByteArray> SitesModel::roleNames() const
{
	static QHash<int, QByteArray> roles;
	if(roles.isEmpty()) {
		roles[ShvPathRole] = "ShvPathRole";
		roles[NameRole] = "NameRole";
		roles[GpsRole] = "GpsRole";
		roles[BrokerConnectedRole] = "BrokerConnectedRole";
	}
	return roles;
}

QVariant SitesModel::data(const QModelIndex &index, int role) const
{
	if(role == ShvPathRole) {
		return m_sites[index.row()].shvPath;
	}
	if(role == NameRole || role == Qt::DisplayRole) {
		return m_sites[index.row()].name;
	}
	if(role == GpsRole) {
		GPS gps = m_sites[index.row()].gps;
		return QVariantList{gps.first, gps.second};
	}
	if(role == BrokerConnectedRole) {
		return m_sites[index.row()].brokerConnected;
	}
	return QVariant();
}

ShvSite *SitesModel::addSite(const QString &shv_path, const QString &name, GPS gps)
{
	shvInfo() << "adding site:" << shv_path;
	beginInsertRows(QModelIndex(), m_sites.count(), m_sites.count());
	m_sites.append(ShvSite(shv_path, name, gps));
	endInsertRows();
	return &m_sites.last();
}

void SitesModel::setSiteBrokerConnected(const std::string &shv_path, bool is_connected)
{
	//shvInfo() << "setSiteBrokerConnected:" << shv_path << is_connected;
	QString qshv_path = QString::fromStdString(shv_path);
	for (int i = 0; i < m_sites.count(); ++i) {
		ShvSite &site = m_sites[i];
		if(site.shvPath == qshv_path) {
			if(site.brokerConnected != is_connected) {
				site.brokerConnected = is_connected;
				//shvInfo() << "emit:" << qshv_path << i << is_connected;
				QVector<int> roles;
				for (int i = 0; i < BrokerConnectedRole; ++i)
					roles << i;
				emit dataChanged(createIndex(i, 0), createIndex(i, 0), QVector<int>{BrokerConnectedRole});
			}
			break;
		}
	}
}
