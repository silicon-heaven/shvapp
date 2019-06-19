#include "pathsmodel.h"

#include "../theapp.h"

#include <shv/chainpack/rpcvalue.h>
#include <shv/coreqt/log.h>

#include <QBrush>

namespace cp = shv::chainpack;

static const std::string PATH = "path";
static const std::string GRANT = "grant";
static const std::string WEIGHT = "weight";
static const std::string FORWARD_GRANT = "forwardGrant";
static const int INVALID = -1;

PathsModel::PathsModel(QObject *parent)
	: Super(parent)
{
}

PathsModel::~PathsModel()
{
}

void PathsModel::setPaths(const shv::chainpack::RpcValue::Map &paths)
{
	beginResetModel();
	m_paths.clear();

	std::vector<std::string> keys = paths.keys();

	for (size_t i = 0; i < keys.size(); i++){
		shv::chainpack::RpcValue::Map path = paths.value(keys[i]).toMap();
		path[PATH] = keys[i];
		m_paths.push_back(path);
	}

	endResetModel();
}

shv::chainpack::RpcValue::Map PathsModel::paths()
{
	shv::chainpack::RpcValue::Map paths;

	for (int i = 0; i < m_paths.count(); i++){
		const shv::chainpack::RpcValue::Map &p = m_paths.at(i);
		shv::chainpack::RpcValue::Map path_settings;

		if (p.hasKey(GRANT)){
			path_settings[GRANT] = p.value(GRANT);
		}

		if (p.hasKey(WEIGHT)){
			path_settings[WEIGHT] = p.value(WEIGHT);
		}

		if (p.hasKey(FORWARD_GRANT)){
			path_settings[FORWARD_GRANT] = p.value(FORWARD_GRANT);
		}

		if (!path_settings.empty()){
			std::string path_name = p.value(PATH).toStdString();
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
	else if (ix.column() == Columns::ColForwardGrant){
		return Super::flags(ix) |= Qt::ItemIsUserCheckable | Qt::ItemIsUserTristate;
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
			return QString::fromStdString(path.value(PATH).toStdString());
		case Columns::ColGrant:
			return QString::fromStdString(path.value(GRANT).toStdString());
		case Columns::ColWeight:
			return QString::fromStdString(path.value(WEIGHT).toStdString());
		}
	}
	else if(role == Qt::CheckStateRole) {
		switch (ix.column()) {
			case Columns::ColForwardGrant:{
				if (path.hasKey(FORWARD_GRANT)){
					return (path.value(FORWARD_GRANT).toBool()) ? Qt::Checked : Qt::Unchecked;
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
			return QString::fromStdString(path.value(PATH).toStdString());
		case Columns::ColGrant:
			return QString::fromStdString(path.value(GRANT).toStdString());
		case Columns::ColWeight:
			return (path.hasKey(WEIGHT)) ? path.value(WEIGHT).toInt() : INVALID;
		case Columns::ColForwardGrant:
			if (path.hasKey(FORWARD_GRANT)){
				return (path.value(FORWARD_GRANT).toBool()) ? Qt::Checked : Qt::Unchecked;
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
		shv::chainpack::RpcValue::Map &path = m_paths[ix.row()];
		int col = ix.column();

		if (col == Columns::ColForwardGrant){
			if (val.toInt() != Qt::CheckState::PartiallyChecked){
				path[FORWARD_GRANT] = (val.toInt() == Qt::CheckState::Checked);
			}
			else{
				path.erase(FORWARD_GRANT);
			}

			return true;
		}
	}
	else if (role == Qt::EditRole){
		shv::chainpack::RpcValue::Map &path = m_paths[ix.row()];

		if (ix.column() == Columns::ColPath){
			path[PATH] = (!val.toString().isEmpty()) ? val.toString().toStdString() : "";
		}
		else if (ix.column() == Columns::ColGrant){
			if (!val.toString().isEmpty()) {
				path[GRANT] = val.toString().toStdString();
			}
			else{
				path.erase(GRANT);
			}
		}
		else if (ix.column() == Columns::ColWeight){
			if (val.toInt() > INVALID) {
				path[WEIGHT] = val.toInt();
			}
			else {
				path.erase(WEIGHT);
			}

			return true;
		}
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
				return tr("Grant weight");
			case Columns::ColForwardGrant:
				return tr("Forward grant");
			default:
				return tr("Unknown");
			}
		}
	}

	return ret;
}

void PathsModel::addPath()
{
	beginInsertRows(QModelIndex(), m_paths.count(), m_paths.count());
	m_paths.append(shv::chainpack::RpcValue::Map());
	endInsertRows();
}

void PathsModel::deletePath(int index)
{
	if ((index >= 0) && (index < m_paths.count())){
		beginResetModel();
		m_paths.remove(index);
		endResetModel();
	}
}

