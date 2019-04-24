#include "clientshvnode.h"
#include "brokerapp.h"

#include "rpc/clientbrokerconnection.h"
#include "rpc/masterbrokerconnection.h"

#include <shv/coreqt/log.h>

#define logRpcMsg() shvCDebug("RpcMsg")

namespace cp = shv::chainpack;

//======================================================================
// ClientShvNode
//======================================================================
ClientShvNode::ClientShvNode(rpc::ClientBrokerConnection *conn, ShvNode *parent)
	: Super(parent)
{
	shvInfo() << "Creating client node:" << this << nodeId() << "connection:" << conn->connectionId();
	addConnection(conn);
}

ClientShvNode::~ClientShvNode()
{
	shvInfo() << "Destroying client node:" << this << nodeId();// << "connections:" << [this]() { std::string s; for(auto c : m_connections) s += std::to_string(c->connectionId()) + " "; return s;}();
}

void ClientShvNode::addConnection(rpc::ClientBrokerConnection *conn)
{
	// prefere new connections, old one might not work
	m_connections.insert(0, conn);
	connect(conn, &rpc::ClientBrokerConnection::destroyed, [this, conn]() {removeConnection(conn);});
}

void ClientShvNode::removeConnection(rpc::ClientBrokerConnection *conn)
{
	//shvWarning() << this << "removing connection:" << conn;
	m_connections.removeOne(conn);
	if(m_connections.isEmpty() && ownChildren().isEmpty())
		deleteLater();
}

void ClientShvNode::handleRawRpcRequest(shv::chainpack::RpcValue::MetaData &&meta, std::string &&data)
{
	rpc::ClientBrokerConnection *conn = connection();
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
MasterBrokerShvNode::MasterBrokerShvNode(ShvNode *parent)
	: Super(parent)
{
	shvInfo() << "Creating master broker connection node:" << this;
}

MasterBrokerShvNode::~MasterBrokerShvNode()
{
	shvInfo() << "Destroying master broker connection node:" << this;
}

