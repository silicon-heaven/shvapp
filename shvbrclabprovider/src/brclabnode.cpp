#include "brclabnode.h"
#include "shvbrclabproviderapp.h"

#include <shv/chainpack/metamethod.h>
#include <shv/chainpack/rpcvalue.h>
#include <shv/chainpack/rpc.h>

namespace cp = shv::chainpack;

BrclabNode::BrclabNode(const std::string &node_id, ShvNode *parent):
	Super(node_id, parent)
{
	m_BrclabUsersNode = new BrclabUsersNode("brclabusers", ShvBrclabProviderApp::instance()->brclabUsersFileName(), this);
}

