#include "shvclientnode.h"

#include "rpc/serverconnection.h"

#include <shv/coreqt/log.h>

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


