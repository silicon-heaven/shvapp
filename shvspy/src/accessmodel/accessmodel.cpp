#include "accessmodel.h"

#include "../theapp.h"

#include <shv/coreqt/log.h>
#include <shv/core/exception.h>
#include <shv/chainpack/rpcvalue.h>

#include <QBrush>

namespace cp = shv::chainpack;

AccessModel::AccessModel(QObject *parent)
	: Super(parent)
{
}

AccessModel::~AccessModel()
{
}

void AccessModel::setRules(const shv::chainpack::RpcValue &role_rules)
{
	beginResetModel();
	m_legacyRulesFormat = role_rules.isMap();
	m_rules = shv::broker::AclRoleAccessRules::fromRpcValue(role_rules);
	endResetModel();
}

shv::chainpack::RpcValue AccessModel::rules()
{
	if(m_legacyRulesFormat)
		return m_rules.toRpcValue_legacy();
	return m_rules.toRpcValue();
}

int AccessModel::rowCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent)
	return m_rules.size();
}

int AccessModel::columnCount(const QModelIndex &parent) const
{
	if (parent.isValid())
		return 0;

	return Columns::ColCount;
}

Qt::ItemFlags AccessModel::flags(const QModelIndex &ix) const
{
	if (!ix.isValid()){
		return Qt::NoItemFlags;
	}

	return  Super::flags(ix) |= Qt::ItemIsEditable;
}

QVariant AccessModel::data(const QModelIndex &ix, int role) const
{
	if (m_rules.empty() || ix.row() >= (int)m_rules.size() || ix.row() < 0){
		return QVariant();
	}

	const shv::broker::AclAccessRule &rule = m_rules.at(ix.row());

	if(role == Qt::DisplayRole || role == Qt::EditRole) {
		switch (ix.column()) {
		case Columns::ColPath:
			return QString::fromStdString(rule.pathPattern);
		case Columns::ColMethod:
			return QString::fromStdString(rule.method);
		case Columns::ColGrant:
			return QString::fromStdString(rule.grant.toRpcValue().toCpon());
		}
	}

	return QVariant();
}

bool AccessModel::setData(const QModelIndex &ix, const QVariant &val, int role)
{
	if (m_rules.empty() || ix.row() >= (int)m_rules.size() || ix.row() < 0){
		return false;
	}

	if (role == Qt::EditRole){
		shv::broker::AclAccessRule &rule = m_rules[ix.row()];
		if (ix.column() == Columns::ColPath) {
			rule.pathPattern = val.toString().toStdString();
		}
		else if (ix.column() == Columns::ColMethod) {
			rule.method = val.toString().toStdString();
		}
		else if (ix.column() == Columns::ColGrant) {
			std::string cpon = val.toString().toStdString();
			std::string err;
			shv::chainpack::RpcValue rv = cp::RpcValue::fromCpon(cpon, &err);

			if(err.empty()) {
				rule.grant = shv::chainpack::AccessGrant::fromRpcValue(rv);
			}
			else {
				//shvError() << "Invalid access grant definition:" << cpon;
				throw shv::core::Exception(tr("Invalid access grant definition: %1").arg(val.toString()).toStdString());
			}
		}
	}

	return false;
}

QVariant AccessModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	QVariant ret;
	if(orientation == Qt::Horizontal) {
		if(role == Qt::DisplayRole) {
			switch (section){
			case Columns::ColPath:
				return tr("Path");
			case Columns::ColMethod:
				return tr("Method");
			case Columns::ColGrant:
				return tr("Grant");
			default:
				return tr("Unknown");
			}
		}
	}

	return ret;
}

void AccessModel::addRule()
{
	beginInsertRows(QModelIndex(), rowCount(), rowCount());
	m_rules.push_back(shv::broker::AclAccessRule());
	endInsertRows();
}

void AccessModel::deleteRule(int index)
{
	if ((index >= 0) && (index < rowCount())){
		beginResetModel();
		m_rules.erase(m_rules.begin() + index);
		endResetModel();
	}
}

bool AccessModel::isRulesValid()
{
	for (int i = 0; i < rowCount(); i++){
		if (!m_rules[i].isValid())
			return false;
	}

	return true;
}

