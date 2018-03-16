#include "shvbrokernodeitem.h"

#include <shv/iotqt/rpc/clientconnection.h>

#include <shv/chainpack/rpcmessage.h>
#include <shv/core/stringview.h>
#include <shv/core/utils.h>
#include <shv/coreqt/log.h>

#include <QApplication>
#include <QElapsedTimer>
#include <QTimer>

namespace cp = shv::chainpack;

struct ShvBrokerNodeItem::RpcRequestInfo
{
	std::string nodePath;
	QElapsedTimer startTS;

	RpcRequestInfo()
	{
		startTS.start();
	}
};

ShvBrokerNodeItem::ShvBrokerNodeItem(const std::string &server_name)
	: Super(server_name)
{
	QTimer *rpc_rq_timeout = new QTimer(this);
	rpc_rq_timeout->start(5000);
	connect(rpc_rq_timeout, &QTimer::timeout, [this]() {
		QElapsedTimer tm2;
		tm2.start();
		auto it = m_runningRpcRequests.begin();
		while (it != m_runningRpcRequests.end()) {
			if(it->second.startTS.msecsTo(tm2) > shv::iotqt::rpc::ClientConnection::defaultRpcTimeout()) {
				shvWarning() << "RPC request timeout expired for node:" << it->second.nodePath;
				it = m_runningRpcRequests.erase(it);
			}
			else
				++it;
		}
	});
}

ShvBrokerNodeItem::~ShvBrokerNodeItem()
{
	SHV_SAFE_DELETE(m_clientConnection);
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
	QVariantMap ret;
	if(m_clientConnection) {
		//ret["oid"] = oid();
		ret["name"] = QString::fromStdString(nodeId());
		ret["host"] = QString::fromStdString(m_clientConnection->host());
		ret["port"] = m_clientConnection->port();
		ret["user"] = QString::fromStdString(m_clientConnection->user());
		ret["password"] = QString::fromStdString(m_clientConnection->password());
	}
	return ret;
}

void ShvBrokerNodeItem::setServerProperties(const QVariantMap &props)
{
	//setOid(props.value("oid").toInt());
	shv::iotqt::rpc::ClientConnection *cli = clientConnection();
	//cli->setServerName(props.value("name").toString());
	cli->setHost(props.value("host").toString().toStdString());
	cli->setPort(props.value("port").toInt());
	cli->setUser(props.value("user").toString().toStdString());
	cli->setPassword(props.value("password").toString().toStdString());
	emitDataChanged();
}

void ShvBrokerNodeItem::open()
{
	close();
	clientConnection()->open();
	m_openStatus = OpenStatus::Connecting;
	emitDataChanged();
}

void ShvBrokerNodeItem::close()
{
	if(openStatus() == OpenStatus::Disconnected)
		return;
	if(m_clientConnection)
		m_clientConnection->close();
	m_openStatus = OpenStatus::Disconnected;
	removeRows(0, rowCount());
	emitDataChanged();
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
	if(!m_clientConnection) {
		m_clientConnection = new shv::iotqt::rpc::ClientConnection(nullptr);
		m_clientConnection->setCheckBrokerConnectedInterval(0);
		connect(m_clientConnection, &shv::iotqt::rpc::ClientConnection::brokerConnectedChanged, this, [this](bool is_connected) {
			m_openStatus = is_connected? OpenStatus::Connected: OpenStatus::Disconnected;
			emitDataChanged();
			if(is_connected)
				loadChildren();
		});
		connect(m_clientConnection, &shv::iotqt::rpc::ClientConnection::rpcMessageReceived, this, &ShvBrokerNodeItem::onRpcMessageReceived);
	}
	return m_clientConnection;
}

ShvNodeItem* ShvBrokerNodeItem::findNode(const std::string &path, std::string *path_rest)
{
	ShvNodeItem *ret = this;
	shv::core::StringView sv(path);
	std::vector<shv::core::StringView> id_list = sv.split('/');
	/*
	if(id_list.empty()) {
		if(path_rest)
			*path_rest = path.substr(node_id.start());
	}
	*/
	for(const shv::core::StringView &node_id : id_list) {
		int i;
		int row_cnt = ret->rowCount();
		for (i = 0; i < row_cnt; ++i) {
			QStandardItem *it = ret->child(i);
			ShvNodeItem *nd = dynamic_cast<ShvNodeItem*>(it);
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

unsigned ShvBrokerNodeItem::requestLoadChildren(const std::string &path)
{
	shv::iotqt::rpc::ClientConnection *cc = clientConnection();
	unsigned rqid = cc->callShvMethod(path, "ls");
	m_runningRpcRequests[rqid].nodePath = path;
	return rqid;
}

void ShvBrokerNodeItem::onRpcMessageReceived(const shv::chainpack::RpcMessage &msg)
{
	if(msg.isResponse()) {
		cp::RpcResponse resp(msg);
		unsigned rqid = resp.requestId();
		auto it = m_runningRpcRequests.find(rqid);
		if(it == m_runningRpcRequests.end()) {
			shvWarning() << "unexpected request id:" << rqid;
			return;
		}
		const std::string &path = it->second.nodePath;
		ShvNodeItem *nd = findNode(path);
		if(nd) {
			nd->processRpcMessage(msg);
		}
		else {
			shvError() << "cannot find node on path:" << path;
		}
		m_runningRpcRequests.erase(it);
	}
}

