#include "shvclientnode.h"
#include "brokerapp.h"

#include "rpc/serverconnection.h"

#include <shv/coreqt/log.h>
#include <shv/iotqt/rpc/tunnelhandle.h>

#define logRpcMsg() shvCDebug("RpcMsg")

namespace cp = shv::chainpack;

ShvClientNode::ShvClientNode(rpc::ServerConnection *conn, ShvNode *parent)
	: Super(parent)
{
	shvInfo() << "Creating client node:" << this << "connection:" << conn->connectionId();
	addConnection(conn);
}

ShvClientNode::~ShvClientNode()
{
	shvInfo() << "Destroying client node:" << this << "connections:" << [this]() { std::string s; for(auto c : m_connections) s += std::to_string(c->connectionId()) + " "; return s;}();
}

void ShvClientNode::addConnection(rpc::ServerConnection *conn)
{
	m_connections << conn;
}

void ShvClientNode::removeConnection(rpc::ServerConnection *conn)
{
	m_connections.removeOne(conn);
	if(m_connections.isEmpty())
		deleteLater();
}

void ShvClientNode::handleRawRpcRequest(shv::chainpack::RpcValue::MetaData &&meta, std::string &&data)
{
	rpc::ServerConnection *conn = connection();
	if(conn)
		conn->sendRawData(std::move(meta), std::move(data));
}

shv::chainpack::RpcValue ShvClientNode::hasChildren(const StringViewList &shv_path)
{
	Q_UNUSED(shv_path)
	//shvWarning() << "ShvClientNode::hasChildren path:" << StringView::join(shv_path, '/');// << "ret:" << nullptr;
	return true;
}


