#include "attributesmodel.h"

#include "../theapp.h"
#include "../servertreemodel/shvnodeitem.h"

#include <shv/chainpack/cponreader.h>
//#include <shv/chainpack/cponwriter.h>
#include <shv/chainpack/rpcvalue.h>
#include <shv/core/utils.h>
#include <shv/core/assert.h>
#include <shv/coreqt/log.h>
#include <shv/iotqt/rpc/rpc.h>

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
	if(ix.column() < 0 || ix.column() >= ColCnt)
		return QVariant();

	switch (role) {
	case Qt::DisplayRole: {
		switch (ix.column()) {
		case ColMethodName:
		case ColSignature:
		case ColParams: {
			//QVariant v = m_rows.value(ix.row()).value(ix.column());
			cp::RpcValue rv = m_rows[ix.row()][ix.column()];
			return rv.isValid()? QString::fromStdString(rv.toCpon()): QVariant();
		}
		case ColResult: {
			if(m_rows[ix.row()][ColError].isIMap()) {
				cp::RpcResponse::Error err(m_rows[ix.row()][ColError].toIMap());
				return QString::fromStdString(err.message());
			}
			else {
				cp::RpcValue rv = m_rows[ix.row()][ColResult];
				if(!rv.isValid())
					return QVariant();

				static constexpr int MAX_TT_SIZE = 1024;
				std::string tts = rv.toCpon();
				if(tts.size() > MAX_TT_SIZE)
					tts = tts.substr(0, MAX_TT_SIZE) + " < ... " + std::to_string(tts.size() - MAX_TT_SIZE) + " more bytes >";

				return QString::fromStdString(tts);
			}
		}
		case ColFlags:
			return QString::fromStdString(m_rows[ix.row()][ix.column()].toString());
		default:
			break;
		}
		break;
	}
	case Qt::EditRole: {
		switch (ix.column()) {
		case ColResult:
		case ColParams: {
			cp::RpcValue rv = m_rows[ix.row()][ix.column()];
			return rv.isValid()? QString::fromStdString(rv.toCpon()): QVariant();
		}
		default:
			break;
		}
		break;
	}
	case RpcValueRole: {
		switch (ix.column()) {
		case ColResult:
		case ColParams: {
			cp::RpcValue rv = m_rows[ix.row()][ix.column()];
			return QVariant::fromValue(rv);
		}
		default:
			break;
		}
		break;
	}
	case Qt::DecorationRole: {
		if(ix.column() == ColBtRun) {
			bool is_notify = m_rows[ix.row()][ColFlags].toBool();
			if(!is_notify) {
				static QIcon ico_run = QIcon(QStringLiteral(":/shvspy/images/run"));
				static QIcon ico_reload = QIcon(QStringLiteral(":/shvspy/images/reload"));
				auto v = m_rows[ix.row()][ColBtRun];
				return (v.toUInt() > 0)? ico_reload: ico_run;
			}
		}
		break;
	}
	case Qt::ToolTipRole: {
		if(ix.column() == ColBtRun) {
			return tr("Call remote method");
		}
		else if(ix.column() == ColResult) {
			return data(ix, Qt::DisplayRole);
		}
		else if(ix.column() == ColFlags) {
			bool is_notify = m_rows[ix.row()][ColFlags].toUInt() & cp::MetaMethod::Flag::IsSignal;
			return is_notify? tr("Method is notify signal"): QVariant();
		}
		else {
			return data(ix, Qt::DisplayRole);
		}
	}
	default:
		break;
	}
	return QVariant();
}

bool AttributesModel::setData(const QModelIndex &ix, const QVariant &val, int role)
{
	shvLogFuncFrame() << val.toString() << val.typeName() << "role:" << role;
	if(role == Qt::EditRole) {
		if(ix.column() == ColParams) {
			if(!m_shvTreeNodeItem.isNull()) {
				std::string cpon = val.toString().toStdString();
				cp::RpcValue params;
				if(!cpon.empty()) {
					std::string err;
					params = cp::RpcValue::fromCpon(cpon, &err);
					if(!err.empty())
						shvError() << "cannot set invalid cpon data";
				}
				m_shvTreeNodeItem->setMethodParams(ix.row(), params);
				loadRow(ix.row());
				return true;
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
			else if(section == ColSignature)
				ret = tr("Signature");
			else if(section == ColFlags)
				ret = tr("Flags");
			else if(section == ColAccessGrant)
				ret = tr("ACG");
			else if(section == ColParams)
				ret = tr("Params");
			else if(section == ColResult)
				ret = tr("Result");
		}
		else if(role == Qt::ToolTipRole) {
			if(section == ColAccessGrant)
				ret = tr("Acess Grant");
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

QString AttributesModel::path() const
{
	if (m_shvTreeNodeItem.isNull()) {
		return QString();
	}
	return QString::fromStdString(m_shvTreeNodeItem->shvPath());
}

QString AttributesModel::method(int row) const
{
	if (m_shvTreeNodeItem.isNull()) {
		return QString();
	}
	return QString::fromStdString(m_shvTreeNodeItem->methods()[row].method);
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
	emit methodCallResultChanged(method_ix);
}

void AttributesModel::emitRowChanged(int row_ix)
{
	QModelIndex ix1 = index(row_ix, 0);
	QModelIndex ix2 = index(row_ix, ColCnt - 1);
	emit dataChanged(ix1, ix2);
}

void AttributesModel::callGet()
{
	for (unsigned i = 0; i < m_rows.size(); ++i) {
		std::string mn = m_rows[i][ColMethodName].toString();
		if(mn == cp::Rpc::METH_GET) {
			callMethod(i);
		}
	}
}

void AttributesModel::loadRow(int method_ix)
{
	if(method_ix < 0 || method_ix >= (int)m_rows.size() || m_shvTreeNodeItem.isNull())
		return;
	const QVector<ShvMetaMethod> &mm = m_shvTreeNodeItem->methods();
	const ShvMetaMethod & mtd = mm[method_ix];
	RowVals &rv = m_rows[method_ix];
	shvDebug() << "load row:" << mtd.method << "flags:" << mtd.flags << mtd.flagsStr();
	rv[ColMethodName] = mtd.method;
	rv[ColSignature] = mtd.signatureStr();
	rv[ColFlags] = mtd.flagsStr();
	rv[ColAccessGrant] = mtd.accessGrant;
	if(mtd.params.isValid()) {
		rv[ColParams] = mtd.params;
	}
	shvDebug() << "\t response:" << mtd.response.toCpon() << "is valid:" << mtd.response.isValid();
	if(mtd.response.isError()) {
		rv[ColResult] = mtd.response.error();
	}
	else {
		shv::chainpack::RpcValue result = mtd.response.result();
		rv[ColResult] = result;
	}
	rv[ColBtRun] = mtd.rpcRequestId;
}

void AttributesModel::loadRows()
{
	m_rows.clear();
	if(!m_shvTreeNodeItem.isNull()) {
		const QVector<ShvMetaMethod> &mm = m_shvTreeNodeItem->methods();
		for (int i = 0; i < mm.count(); ++i) {
			RowVals rv;
			rv.resize(ColCnt);
			m_rows.push_back(rv);
			loadRow(m_rows.size() - 1);
		}
	}
	emit layoutChanged();
	emit reloaded();
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
