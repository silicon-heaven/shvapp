#include "shvbrokernodeitem.h"
#include "../theapp.h"
#include "../appclioptions.h"

#include <shv/iotqt/rpc/clientconnection.h>
#include <shv/iotqt/rpc/deviceconnection.h>

#include <shv/chainpack/rpcmessage.h>
#include <shv/core/stringview.h>
#include <shv/core/utils.h>
#include <shv/coreqt/log.h>

#include <QApplication>
#include <QElapsedTimer>
#include <QIcon>
#include <QTimer>

namespace cp = shv::chainpack;

struct ShvBrokerNodeItem::RpcRequestInfo
{
	std::string shvPath;
	QElapsedTimer startTS;

	RpcRequestInfo()
	{
		startTS.start();
	}
};

ShvBrokerNodeItem::ShvBrokerNodeItem(ServerTreeModel *m, const std::string &server_name)
	: Super(m, server_name)
{
	QTimer *rpc_rq_timeout = new QTimer(this);
	rpc_rq_timeout->start(5000);
	connect(rpc_rq_timeout, &QTimer::timeout, [this]() {
		QElapsedTimer tm2;
		tm2.start();
		auto it = m_runningRpcRequests.begin();
		while (it != m_runningRpcRequests.end()) {
			if(it->second.startTS.msecsTo(tm2) > shv::iotqt::rpc::ClientConnection::defaultRpcTimeout()) {
				shvWarning() << "RPC request timeout expired for node:" << it->second.shvPath;
				it = m_runningRpcRequests.erase(it);
			}
			else
				++it;
		}
	});
}

ShvBrokerNodeItem::~ShvBrokerNodeItem()
{
	if(m_rpcConnection) {
		disconnect(m_rpcConnection, 0, this, 0);
		delete m_rpcConnection;
	}
}

QVariant ShvBrokerNodeItem::data(int role) const
{
	QVariant ret;
	if(role == Qt::DisplayRole) {
		ret = QString::fromStdString(nodeId());
		//if(m_clientConnection) {
		//	ret = m_clientConnection->serverName();
		//}
	}
	else if(role == Qt::DecorationRole) {
		static QIcon ico_connected = QIcon(QStringLiteral(":/shvspy/images/connected.png"));
		static QIcon ico_connecting = QIcon(QStringLiteral(":/shvspy/images/connecting.png"));
		static QIcon ico_disconnected = QIcon(QStringLiteral(":/shvspy/images/disconnected.png"));
		switch (openStatus()) {
		case OpenStatus::Connected: return ico_connected;
		case OpenStatus::Connecting: return ico_connecting;
		case OpenStatus::Disconnected: return ico_disconnected;
		default: return QIcon();
		}
	}
	else
		ret = Super::data(role);
	return ret;
}

QVariantMap ShvBrokerNodeItem::serverProperties() const
{
	return m_serverPropeties;
}

void ShvBrokerNodeItem::setServerSubscriptionProperties(const QVariantMap &props)
{
	m_serverPropeties = props;
}

void ShvBrokerNodeItem::setServerProperties(const QVariantMap &props)
{
	if(m_rpcConnection) {
		delete m_rpcConnection;
		m_rpcConnection = nullptr;
	}
	m_serverPropeties = props;
	setNodeId(m_serverPropeties.value("name").toString().toStdString());
}

void ShvBrokerNodeItem::open()
{
	close();
	shv::iotqt::rpc::ClientConnection *cli = clientConnection();
	//cli->setServerName(props.value("name").toString());
	cli->setHost(m_serverPropeties.value("host").toString().toStdString());
	cli->setPort(m_serverPropeties.value("port").toInt());
	cli->setUser(m_serverPropeties.value("user").toString().toStdString());
	cli->setPassword(m_serverPropeties.value("password").toString().toStdString());
	cli->open();
	m_openStatus = OpenStatus::Connecting;
	emitDataChanged();
}

void ShvBrokerNodeItem::close()
{
	if(openStatus() == OpenStatus::Disconnected)
		return;
	if(m_rpcConnection)
		m_rpcConnection->close();
	m_openStatus = OpenStatus::Disconnected;
	deleteChildren();
}
/*
QString ServerNode::connectionErrorString()
{
	QString ret;
	if(m_clientConnection)
		ret = m_clientConnection->errorString();
	return ret;
}
*/
shv::iotqt::rpc::ClientConnection *ShvBrokerNodeItem::clientConnection()
{

    if(!m_rpcConnection) {
		QString conn_type = m_serverPropeties.value(cp::Rpc::KEY_CONNECTION_TYPE).toString();

		shv::iotqt::rpc::DeviceAppCliOptions opts;
		{
			int proto_type = m_serverPropeties.value("rpc.protocolType").toInt();
			if(proto_type == (int)cp::Rpc::ProtocolType::JsonRpc)
				opts.setProtocolType("jsonrpc");
			else if(proto_type == (int)cp::Rpc::ProtocolType::Cpon)
				opts.setProtocolType("cpon");
			else
				opts.setProtocolType("chainpack");
		}
		{
			QVariant v = m_serverPropeties.value("rpc.reconnectInterval");
			if(v.isValid())
				opts.setReconnectInterval(v.toInt());
		}
		{
			QVariant v = m_serverPropeties.value("rpc.heartbeatInterval");
			if(v.isValid())
				opts.setHeartbeatInterval(v.toInt());
		}
		{
			QString dev_id = m_serverPropeties.value("device.id").toString();
			if(!dev_id.isEmpty())
				opts.setDeviceId(dev_id);
		}
		{
			QString mount_point = m_serverPropeties.value("device.mountPoint").toString();
			if(!mount_point.isEmpty())
				opts.setMountPoint(mount_point);
		}

		if(conn_type == cp::Rpc::TYPE_DEVICE) {
			auto *c = new shv::iotqt::rpc::DeviceConnection(nullptr);
			c->setCliOptions(&opts);
			m_rpcConnection = c;
		}
		else {
			m_rpcConnection = new shv::iotqt::rpc::ClientConnection(nullptr);
			m_rpcConnection->setCliOptions(&opts);
		}
		//m_rpcConnection->setCheckBrokerConnectedInterval(0);
		connect(m_rpcConnection, &shv::iotqt::rpc::ClientConnection::brokerConnectedChanged, this, &ShvBrokerNodeItem::onBrokerConnectedChanged);
		connect(m_rpcConnection, &shv::iotqt::rpc::ClientConnection::rpcMessageReceived, this, &ShvBrokerNodeItem::onRpcMessageReceived);
    }
	return m_rpcConnection;
}

void ShvBrokerNodeItem::onBrokerConnectedChanged(bool is_connected)
{
	m_openStatus = is_connected? OpenStatus::Connected: OpenStatus::Disconnected;
	emitDataChanged();
	if(is_connected) {
		createSubscriptions();
		loadChildren();
	}
	else {
		deleteChildren();
	}
}

ShvNodeItem* ShvBrokerNodeItem::findNode(const std::string &path, std::string *path_rest)
{
	ShvNodeItem *ret = this;
	shv::core::StringView sv(path);
	std::vector<shv::core::StringView> id_list = sv.split('/');

	for(const shv::core::StringView &node_id : id_list) {
		int i;
		int row_cnt = ret->childCount();
		for (i = 0; i < row_cnt; ++i) {
			ShvNodeItem *nd = ret->childAt(i);
			if(nd) {
				if(node_id == nd->nodeId()) {
					ret = nd;
					break;
				}
			}
		}
		if(i == row_cnt) {
			if(path_rest)
				*path_rest = path.substr(node_id.start());
			return nullptr;
		}
	}
	return ret;
}

unsigned ShvBrokerNodeItem::callCreateSubscription(const std::string &shv_path, std::string method)
{
	shv::iotqt::rpc::ClientConnection *cc = clientConnection();
	unsigned rqid = cc->createSubscription(shv_path, method);
	m_runningRpcRequests[rqid].shvPath = shv_path;
	return rqid;
}

unsigned ShvBrokerNodeItem::callNodeRpcMethod(const std::string &calling_node_shv_path, const std::string &method, const cp::RpcValue &params)
{
	shv::iotqt::rpc::ClientConnection *cc = clientConnection();
	unsigned rqid = cc->callShvMethod(calling_node_shv_path, method, params);
	m_runningRpcRequests[rqid].shvPath = calling_node_shv_path;
	return rqid;
}

void ShvBrokerNodeItem::onRpcMessageReceived(const shv::chainpack::RpcMessage &msg)
{
	if(msg.isResponse()) {
		cp::RpcResponse resp(msg);
		unsigned rqid = resp.requestId().toUInt();
        shvInfo() << rqid;
		auto it = m_runningRpcRequests.find(rqid);
		if(it == m_runningRpcRequests.end()) {
            shvWarning() << "unexpected request id:" << rqid;
			// can be load attributes request
			return;
		}
		const std::string &path = it->second.shvPath;
		ShvNodeItem *nd = findNode(path);
		if(nd) {
			nd->processRpcMessage(msg);
		}
		else {
			shvError() << "cannot find node on path:" << path;
		}
		m_runningRpcRequests.erase(it);
	}
	else if(msg.isRequest()) {
		cp::RpcRequest rq(msg);
		cp::RpcResponse resp = cp::RpcResponse::forRequest(rq);
		try {
			//shvInfo() << "RPC request received:" << rq.toCpon();
			cp::RpcValue shv_path = rq.shvPath();
			if(!shv_path.toString().empty())
				SHV_EXCEPTION("Invalid path: " + shv_path.toString());
			const cp::RpcValue method = rq.method();
			if(method == cp::Rpc::METH_DIR) {
				resp.setResult(cp::RpcValue::List{
								   cp::Rpc::METH_DIR,
								   //cp::Rpc::METH_PING,
								   cp::Rpc::METH_APP_NAME,
								   cp::Rpc::METH_CONNECTION_TYPE,
							   });
			}
			//else if(method.toString() == cp::Rpc::METH_PING) {
			//	resp.setResult(true);
			//}
			else if(method.toString() == cp::Rpc::METH_APP_NAME) {
				resp.setResult(QCoreApplication::instance()->applicationName().toStdString());
			}
			else if(method.toString() == cp::Rpc::METH_CONNECTION_TYPE) {
				resp.setResult(m_rpcConnection->connectionType());
			}
		}
		catch (shv::core::Exception &e) {
			resp.setError(cp::RpcResponse::Error::create(cp::RpcResponse::Error::MethodCallException, e.message()));
		}
		m_rpcConnection->sendMessage(resp);
	}
}

void ShvBrokerNodeItem::createSubscriptions()
{
	QVariantMap proprs = serverProperties();
	QVariant v = proprs.value("subscriptions");
	if(v.isValid()) {
		QVariantList subs = v.toList();
		for (int i = 0; i < subs.size(); i++) {
			QVariantMap subscription = subs.at(i).toMap();
			shvInfo() << "Create subscription:" << nodeId() << "creating subscription" << subscription["path"].toString() << ":" << subscription["method"].toString();
			callCreateSubscription(subscription["path"].toString().toStdString(), subscription["method"].toString().toStdString());
		}
	}
}

