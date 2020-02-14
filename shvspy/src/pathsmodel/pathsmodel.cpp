#include "pathsmodel.h"

#include "../theapp.h"

#include <shv/chainpack/rpcvalue.h>

#include <QBrush>

namespace cp = shv::chainpack;

static const std::string PATH = "path";
static const std::string GRANT = "grant";

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
		shv::chainpack::RpcValue::Map path;
		path[PATH] = keys[i];
		path[GRANT] = paths.value(keys[i]).toString();
		m_paths.push_back(path);
	}

	endResetModel();
}

shv::chainpack::RpcValue::Map PathsModel::paths()
{
	shv::chainpack::RpcValue::Map paths;

	for (int i = 0; i < m_paths.count(); i++){
		const shv::chainpack::RpcValue::Map &p = m_paths.at(i);

		if (!p.value(PATH).toString().empty() && !p.value(GRANT).toStdString().empty()){
			paths[p.value(PATH).toString()] = p.value(GRANT).toStdString();
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
		}
	}
	else if(role == Qt::EditRole){
		switch (ix.column()) {
		case Columns::ColPath:
			return QString::fromStdString(path.value(PATH).toStdString());
		case Columns::ColGrant:
			return QString::fromStdString(path.value(GRANT).toStdString());
		}
	}

	return QVariant();
}

bool PathsModel::setData(const QModelIndex &ix, const QVariant &val, int role)
{
	if (m_paths.isEmpty() || ix.row() >= m_paths.count() || ix.row() < 0){
		return false;
	}

	if (role == Qt::EditRole){
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

