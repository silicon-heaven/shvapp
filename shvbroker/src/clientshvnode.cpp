#include "clientshvnode.h"
#include "brokerapp.h"

#include "rpc/serverconnection.h"
#include "rpc/masterbrokerconnection.h"

#include <shv/coreqt/log.h>

#define logRpcMsg() shvCDebug("RpcMsg")

namespace cp = shv::chainpack;

//======================================================================
// ClientShvNode
//======================================================================
ClientShvNode::ClientShvNode(rpc::ServerConnection *conn, ShvNode *parent)
	: Super(parent)
{
	shvInfo() << "Creating client node:" << this << nodeId() << "connection:" << conn->connectionId();
	addConnection(conn);
}

ClientShvNode::~ClientShvNode()
{
	shvInfo() << "Destroying client node:" << this << nodeId();// << "connections:" << [this]() { std::string s; for(auto c : m_connections) s += std::to_string(c->connectionId()) + " "; return s;}();
}

void ClientShvNode::addConnection(rpc::ServerConnection *conn)
{
	m_connections << conn;
	connect(conn, &rpc::ServerConnection::destroyed, [this, conn]() {removeConnection(conn);});
}

void ClientShvNode::removeConnection(rpc::ServerConnection *conn)
{
	//shvWarning() << this << "removing connection:" << conn;
	m_connections.removeOne(conn);
	if(m_connections.isEmpty() && ownChildren().isEmpty())
		deleteLater();
}

void ClientShvNode::handleRawRpcRequest(shv::chainpack::RpcValue::MetaData &&meta, std::string &&data)
{
	rpc::ServerConnection *conn = connection();
	if(conn)
		conn->sendRawData(std::move(meta), std::move(data));
}

shv::chainpack::RpcValue ClientShvNode::hasChildren(const StringViewList &shv_path)
{
	Q_UNUSED(shv_path)
	//shvWarning() << "ShvClientNode::hasChildren path:" << StringView::join(shv_path, '/');// << "ret:" << nullptr;
	return nullptr;
}

//======================================================================
// MasterBrokerShvNode
//======================================================================
MasterBrokerShvNode::MasterBrokerShvNode(rpc::MasterBrokerConnection *conn, ShvNode *parent)
	: Super(parent)
{
	shvInfo() << "Creating master broker connection node:" << this << "connection:" << conn->connectionId();
	addConnection(conn);
}

MasterBrokerShvNode::~MasterBrokerShvNode()
{
	shvInfo() << "Destroying master broker connection node:" << this << "connections:" << [this]() { std::string s; for(auto c : m_connections) s += std::to_string(c->connectionId()) + " "; return s;}();
}

void MasterBrokerShvNode::addConnection(rpc::MasterBrokerConnection *conn)
{
	m_connections << conn;
	connect(conn, &rpc::ServerConnection::destroyed, [this, conn]() {removeConnection(conn);});
}

void MasterBrokerShvNode::removeConnection(rpc::MasterBrokerConnection *conn)
{
	//shvWarning() << this << "removing connection:" << conn;
	m_connections.removeOne(conn);
	if(m_connections.isEmpty() && ownChildren().isEmpty())
		deleteLater();
}

void MasterBrokerShvNode::handleRawRpcRequest(shv::chainpack::RpcValue::MetaData &&meta, std::string &&data)
{
	rpc::MasterBrokerConnection *conn = connection();
	if(conn)
		conn->sendRawData(std::move(meta), std::move(data));
}
