#include "brclabnode.h"
#include "shvbrclabproviderapp.h"

#include <shv/chainpack/metamethod.h>
#include <shv/chainpack/rpcvalue.h>
#include <shv/chainpack/rpc.h>
#include <shv/core/log.h>

namespace cp = shv::chainpack;

BrclabNode::BrclabNode(const std::string &brclab_users_fn, ShvNode *parent):
	Super(brclab_users_fn, parent)
{
	m_BrclabUsersNode = new BrclabUsersNode("brclabusers", this);
}

size_t BrclabNode::methodCount(const shv::iotqt::node::ShvNode::StringViewList &shv_path)
{
	return Super::methodCount(shv_path);
}

const shv::chainpack::MetaMethod *BrclabNode::metaMethod(const shv::iotqt::node::ShvNode::StringViewList &shv_path, size_t ix)
{
	return Super::metaMethod(shv_path, ix);
}

shv::chainpack::RpcValue BrclabNode::callMethod(const shv::iotqt::node::ShvNode::StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params)
{
	return Super::callMethod(shv_path, method, params);
}
