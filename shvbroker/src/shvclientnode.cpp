#include "shvclientnode.h"

#include <shv/coreqt/log.h>

ShvClientNode::ShvClientNode(rpc::ServerConnection *connection, QObject *parent)
 : Super(parent)
 , m_connection(connection)
{
	shvInfo() << "Creating client node:" << this;
}

ShvClientNode::~ShvClientNode()
{
	shvInfo() << "Destroying client node:" << this;
}


