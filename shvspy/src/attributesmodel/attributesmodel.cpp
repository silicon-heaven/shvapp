#include "attributesmodel.h"

#include "../theapp.h"
#include "../servertreemodel/shvnodeitem.h"

#include <shv/chainpack/cponreader.h>
//#include <shv/chainpack/cponwriter.h>
#include <shv/chainpack/rpcvalue.h>
#include <shv/core/utils.h>
#include <shv/coreqt/log.h>
#include <shv/core/assert.h>

#include <QSettings>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QIcon>

namespace cp = shv::chainpack;

AttributesModel::AttributesModel(QObject *parent)
	: Super(parent)
{
}

AttributesModel::~AttributesModel()
{
}

int AttributesModel::rowCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent)
	if(m_shvTreeNodeItem.isNull())
		return 0;
	return m_shvTreeNodeItem->methods().count();
}

Qt::ItemFlags AttributesModel::flags(const QModelIndex &ix) const
{
	Qt::ItemFlags ret = Super::flags(ix);
	if(ix.column() == ColParams)
		ret |= Qt::ItemIsEditable;
	if(ix.column() == ColBtRun)
		ret &= ~Qt::ItemIsSelectable;
	return ret;
}

QVariant AttributesModel::data(const QModelIndex &ix, int role) const
{
	if(m_shvTreeNodeItem.isNull())
		return QVariant();
	const QVector<ShvMetaMethod> &mms = m_shvTreeNodeItem->methods();
	if(ix.row() < 0 || ix.row() >= mms.count())
		return QVariant();

	switch (role) {
	case Qt::DisplayRole: {
		switch (ix.column()) {
		case ColMethodName:
		case ColParams:
		case ColResult:
			return m_rows.value(ix.row()).value(ix.column());
		default:
			break;
		}
		break;
	}
	case Qt::EditRole: {
		switch (ix.column()) {
		case ColParams:
			return m_rows.value(ix.row()).value(ix.column());
		default:
			break;
		}
		break;
	}
	case Qt::DecorationRole: {
		if(ix.column() == ColBtRun) {
			static QIcon ico_run = QIcon(QStringLiteral(":/shvspy/images/run"));
			static QIcon ico_reload = QIcon(QStringLiteral(":/shvspy/images/reload"));
			auto v = m_rows.value(ix.row()).value(ix.column());
			return (v.toUInt() > 0)? ico_reload: ico_run;
		}
		break;
	}
	case Qt::ToolTipRole: {
		if(ix.column() == ColBtRun) {
			return tr("Call remote method");
		}
		break;
	}
	default:
		break;
	}
	return QVariant();
	/*
	AttributeNodeBase *nd = dynamic_cast<AttributeNodeBase*>(itemFromIndex(ix.sibling(ix.row(), 0)));
	SHV_ASSERT(nd != nullptr, QString("Internal error ix(%1, %2) %3").arg(ix.row()).arg(ix.column()).arg(ix.internalId()), return QVariant());
	if(ix.column() == 0) {
		if(role == Qt::DisplayRole)
			ret = nd->name();
		else
			ret = Super::data(ix, role);
	}
	else if(ix.column() == 1) {
		if(role == Qt::DisplayRole)
			ret = nd->displayValue();
		else if(role == Qt::ToolTipRole)
			ret = nd->displayValue();
		else if(role == Qt::EditRole)
			ret = nd->toEditorValue(nd->value());
		else
			ret = Super::data(ix, role);
	}
	*/
}

bool AttributesModel::setData(const QModelIndex &ix, const QVariant &val, int role)
{
	shvLogFuncFrame() << val.toString() << val.typeName() << "role:" << role;
	if(role == Qt::EditRole) {
		if(ix.column() == ColParams) {
			std::string cpon = val.toString().toStdString();
			cp::RpcValue params;
			if(!cpon.empty()) {
				try {
					std::istringstream is(cpon);
					cp::CponReader rd(is);
					rd >> params;
					if(!m_shvTreeNodeItem.isNull()) {
						m_shvTreeNodeItem->setMethodParams(ix.row(), params);
						loadRow(ix.row());
						return true;
					}
				}
				catch (cp::CponReader::ParseException &e) {
					shvError() << "error parsing params:" << e.mesage();
				}
			}
		}
	}
	return false;
}

QVariant AttributesModel::headerData(int section, Qt::Orientation o, int role) const
{
	QVariant ret;
	if(o == Qt::Horizontal) {
		if(role == Qt::DisplayRole) {
			if(section == ColMethodName)
				ret = tr("Method");
			else if(section == ColParams)
				ret = tr("Params");
			else if(section == ColResult)
				ret = tr("Result");
		}
	}
	return ret;
}

void AttributesModel::load(ShvNodeItem *nd)
{
	m_rows.clear();
	if(!m_shvTreeNodeItem.isNull())
		m_shvTreeNodeItem->disconnect(this);
	m_shvTreeNodeItem = nd;
	if(nd) {
		connect(nd, &ShvNodeItem::methodsLoaded, this, &AttributesModel::onMethodsLoaded, Qt::UniqueConnection);
		connect(nd, &ShvNodeItem::rpcMethodCallFinished, this, &AttributesModel::onRpcMethodCallFinished, Qt::UniqueConnection);
		nd->checkMethodsLoaded();
	}
	loadRows();
}

void AttributesModel::callMethod(int method_ix)
{
	if(m_shvTreeNodeItem.isNull())
		return;
	unsigned rqid = m_shvTreeNodeItem->callMethod(method_ix);
	m_rows[method_ix][ColBtRun] = rqid;
	emitRowChanged(method_ix);
}

void AttributesModel::onMethodsLoaded()
{
	loadRows();
	callGet();
}

void AttributesModel::onRpcMethodCallFinished(int method_ix)
{
	loadRow(method_ix);
	emitRowChanged(method_ix);
}

void AttributesModel::emitRowChanged(int row_ix)
{
	QModelIndex ix1 = index(row_ix, 0);
	QModelIndex ix2 = index(row_ix, ColCnt - 1);
	emit dataChanged(ix1, ix2);
}

void AttributesModel::callGet()
{
	for (int i = 0; i < m_rows.count(); ++i) {
		QString mn = m_rows[i].value(ColMethodName).toString();
		if(mn == QLatin1String(cp::Rpc::METH_GET)) {
			callMethod(i);
		}
	}
}

void AttributesModel::loadRow(int method_ix)
{
	if(method_ix < 0 || method_ix >= m_rows.count() || m_shvTreeNodeItem.isNull())
		return;
	const QVector<ShvMetaMethod> &mm = m_shvTreeNodeItem->methods();
	const ShvMetaMethod & mtd = mm[method_ix];
	RowVals &rv = m_rows[method_ix];
	rv[ColMethodName] = QString::fromStdString(mtd.method);
	if(mtd.params.isValid()) {
		rv[ColParams] = QString::fromStdString(mtd.params.toCpon());
	}
	if(mtd.response.isError()) {
		rv[ColResult] = QString::fromStdString(mtd.response.error().toString());
	}
	else if(mtd.response.result().isValid()) {
		rv[ColResult] = QString::fromStdString(mtd.response.result().toCpon());
	}
	rv[ColBtRun] = mtd.rpcRequestId;
}

void AttributesModel::loadRows()
{
	m_rows.clear();
	if(!m_shvTreeNodeItem.isNull()) {
		const QVector<ShvMetaMethod> &mm = m_shvTreeNodeItem->methods();
		for (int i = 0; i < mm.count(); ++i) {
			const ShvMetaMethod & mtd = mm[i];
			RowVals rv;
			rv.resize(ColCnt);
			m_rows.insert(m_rows.count(), rv);
			loadRow(m_rows.count() - 1);
			rv[ColMethodName] = QString::fromStdString(mtd.method);
			rv[ColParams] = QString::fromStdString(mtd.params.toCpon());
			if(mtd.response.isError())
				rv[ColResult] = QString::fromStdString(mtd.response.error().toString());
			else
				rv[ColResult] = QString::fromStdString(mtd.response.toCpon());
		}
	}
	emit layoutChanged();
}

/*
void AttributesModel::onRpcMessageReceived(const shv::chainpack::RpcMessage &msg)
{
	if(msg.isResponse()) {
		cp::RpcResponse resp(msg);
		if(resp.requestId() == m_rpcRqId) {
			for(const cp::RpcValue &val : resp.result().toList()) {
				appendRow(QList<QStandardItem*>{
							  new QStandardItem(QString::fromStdString(val.toString())),
							  new QStandardItem("<not called>"),
						  });
			}
		}
	}
}
*/
