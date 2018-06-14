#include "shvclientnode.h"
#include "brokerapp.h"

#include "rpc/serverconnection.h"

#include <shv/coreqt/log.h>
#include <shv/iotqt/rpc/tunnelhandle.h>

#define logRpcMsg() shvCDebug("RpcMsg")

namespace cp = shv::chainpack;

ShvClientNode::ShvClientNode(rpc::ServerConnection *connection, ShvNode *parent)
	: Super(parent)
	, m_connection(connection)
{
	shvInfo() << "Creating client node:" << this << "connection:" << connection->connectionId();
}

ShvClientNode::~ShvClientNode()
{
	shvInfo() << "Destroying client node:" << this << "connection:" << m_connection->connectionId();
}

void ShvClientNode::processRawData(const shv::chainpack::RpcValue::MetaData &meta, std::string &&data)
{
	rpc::ServerConnection *conn = connection();
	conn->sendRawData(meta, std::move(data));
}

shv::chainpack::RpcValue ShvClientNode::hasChildren()
{
	//shvWarning() << "ShvClientNode::hasChildren path:" << shv_path << "ret:" << true;
	return nullptr;
}


