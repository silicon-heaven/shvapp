#include "pathsmodel.h"

#include "../theapp.h"

#include <shv/chainpack/rpcvalue.h>
#include <shv/coreqt/log.h>

#include <QBrush>

namespace cp = shv::chainpack;

PathsModel::PathsModel(QObject *parent)
	: Super(parent)
{
}

PathsModel::~PathsModel()
{
}

void PathsModel::setPaths(const shv::chainpack::RpcValue::Map &paths)
{
	m_paths.clear();

	std::vector<std::string> keys = paths.keys();

	for (size_t i = 0; i < keys.size(); i++){
		shv::chainpack::RpcValue::Map path = paths.value(keys[i]).toMap();
		path["path"] = keys[i];
		m_paths.push_back(path);
	}

	reload();
}

shv::chainpack::RpcValue::Map PathsModel::paths()
{
	shv::chainpack::RpcValue::Map paths;

	for (int i = 0; i < m_paths.count(); i++){
		const shv::chainpack::RpcValue::Map &p = m_paths.at(i);
		shv::chainpack::RpcValue::Map path_settings;

		if (p.hasKey("grant")){
			path_settings["grant"] = p.value("grant");
		}

		if (p.hasKey("weight")){
			path_settings["weight"] = p.value("weight");
		}

		if (p.hasKey("forwardGrant")){
			path_settings["forwardGrant"] = p.value("forwardGrant");
		}

		if (!path_settings.empty()){
			std::string path_name = p.value("path").toStdString();
			paths[path_name] = path_settings;
		}
	}
	return paths;
}

int PathsModel::rowCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent)
	return m_paths.count();
}

int PathsModel::columnCount(const QModelIndex &parent) const
{
	if (parent.isValid())
		return 0;

	return Columns::ColCount;
}

Qt::ItemFlags PathsModel::flags(const QModelIndex &ix) const
{
	if (!ix.isValid()){
		return Qt::NoItemFlags;
	}

	if (ix.column() == Columns::ColForwardGrant){
		return Super::flags(ix) |= Qt::ItemIsUserCheckable;
	}

	return  Super::flags(ix) |= Qt::ItemIsEditable;
}

QVariant PathsModel::data(const QModelIndex &ix, int role) const
{
	if (m_paths.isEmpty() || ix.row() >= m_paths.count() || ix.row() < 0){
		return QVariant();
	}

	const shv::chainpack::RpcValue::Map &path = m_paths.at(ix.row());

	if(role == Qt::DisplayRole) {
		switch (ix.column()) {
		case Columns::ColPath:
			return QString::fromStdString(path.value("path").toStdString());
		case Columns::ColGrant:
			return QString::fromStdString(path.value("grant").toStdString());
		case Columns::ColWeight:
			return QString::fromStdString(path.value("weight").toStdString());
		case Columns::ColForwardGrant:
			return QString::fromStdString(path.value("forwardGrant").toStdString());
		}
	}
	else if(role == Qt::CheckStateRole) {
		switch (ix.column()) {
			case Columns::ColForwardGrant:{
				if (path.hasKey("forwardGrant")){
					return (path.value("forwardGrant").toBool()) ? Qt::Checked : Qt::Unchecked;
				}
				else{
					return Qt::PartiallyChecked;
				}
			}
		}
	}
	else if(role == Qt::EditRole){
		switch (ix.column()) {
		case Columns::ColPath:
			return QString::fromStdString(path.value("path").toStdString());
		case Columns::ColGrant:
			return QString::fromStdString(path.value("grant").toStdString());
		case Columns::ColWeight:
			return (path.hasKey("weight")) ? path.value("weight").toInt() : -1;
		case Columns::ColForwardGrant:
			if (path.hasKey("forwardGrant")){
				return (path.value("forwardGrant").toBool()) ? Qt::Checked : Qt::Unchecked;
			}
			else{
				return Qt::PartiallyChecked;
			}
		}
	}

	return QVariant();
}

bool PathsModel::setData(const QModelIndex &ix, const QVariant &val, int role)
{
	if (m_paths.isEmpty() || ix.row() >= m_paths.count() || ix.row() < 0){
		return false;
	}

	if (role == Qt::CheckStateRole){
/*		shv::chainpack::RpcValue::Map &path = m_paths[ix.row()];
		int col = ix.column();
		bool v = (val == Qt::Checked) ? true : false;

		if (col == Columns::ColPermanent || col == Columns::ColEnabled){
			ShvBrokerNodeItem *nd = TheApp::instance()->serverTreeModel()->brokerById(sub.brokerId());
			if (nd == nullptr){
				return false;
			}

			if (col == Columns::ColPermanent){
				sub.setIsPermanent(v);
			}
			else if (col == Columns::ColEnabled){
				sub.setIsEnabled(v);
				nd->enableSubscription(sub.shvPath().toStdString(), sub.method().toStdString(), v);
			}

			QVariantList new_subs = brokerSubscriptions(sub.brokerId());
			nd->setSubscriptionList(new_subs);

			return true;
		}*/
	}
	else if (role == Qt::EditRole){
	/*	if (ix.column() == Columns::ColMethod){
			Subscription &sub = m_subscriptions[ix.row()];

			ShvBrokerNodeItem *nd = TheApp::instance()->serverTreeModel()->brokerById(sub.brokerId());
			if (nd == nullptr){
				return false;
			}

			nd->enableSubscription(sub.shvPath().toStdString(), sub.method().toStdString(), false);
			sub.setMethod(val.toString());
			nd->enableSubscription(sub.shvPath().toStdString(), sub.method().toStdString(), true);

			QVariantList new_subs = brokerSubscriptions(sub.brokerId());
			nd->setSubscriptionList(new_subs);
			return true;
		}*/
	}

	return false;
}

QVariant PathsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	QVariant ret;
	if(orientation == Qt::Horizontal) {
		if(role == Qt::DisplayRole) {
			switch (section){
			case Columns::ColPath:
				return tr("Path");
			case Columns::ColGrant:
				return tr("Grant");
			case Columns::ColWeight:
				 return tr("Weight");
			case Columns::ColForwardGrant:
				return tr("Forward grant");
			default:
				return tr("Unknown");
			}
		}
	}
	return ret;
}

void PathsModel::reload()
{
	beginResetModel();
	endResetModel();
}

void PathsModel::addPath()
{
	beginInsertRows(QModelIndex(), m_paths.count(), m_paths.count());
	m_paths.append(shv::chainpack::RpcValue::Map());
	endInsertRows();
}

void PathsModel::deletePath(int index)
{
	if ((index > 0) && (index < m_paths.count())){
		m_paths.remove(index);
		reload();
	}
}

