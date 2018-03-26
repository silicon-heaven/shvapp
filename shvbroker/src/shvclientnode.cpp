#include "shvclientnode.h"

#include <shv/coreqt/log.h>

ShvClientNode::ShvClientNode(rpc::ServerConnection *connection, ShvNode *parent)
 : Super(parent)
 , m_connection(connection)
{
	shvInfo() << "Creating client node:" << this;
}

ShvClientNode::~ShvClientNode()
{
	shvInfo() << "Destroying client node:" << this;
}


