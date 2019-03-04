#include "brclabnode.h"

#include <shv/chainpack/metamethod.h>
#include <shv/chainpack/rpcvalue.h>
#include <shv/chainpack/rpc.h>
#include <shv/core/exception.h>
#include <shv/core/log.h>

namespace cp = shv::chainpack;

static std::vector<cp::MetaMethod> meta_methods_brclab {
	{cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, false},
	{cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, false}
};

BrclabNode::BrclabNode(ShvNode *parent):
	Super(".brclab", parent)
{
	m_BrclabUsersNode = new BrclabUsersNode("brclabusers", this);
}

size_t BrclabNode::methodCount(const shv::iotqt::node::ShvNode::StringViewList &shv_path)
{
	if(shv_path.empty()){
		return meta_methods_brclab.size();
	}

	return Super::methodCount(shv_path);
}

const shv::chainpack::MetaMethod *BrclabNode::metaMethod(const shv::iotqt::node::ShvNode::StringViewList &shv_path, size_t ix)
{
	if(shv_path.empty()) {
		if (ix < meta_methods_brclab.size()){
			return &(meta_methods_brclab[ix]);
		}
		else{
			SHV_EXCEPTION("Invalid method index: " + std::to_string(ix) + " of: " + std::to_string(meta_methods_brclab.size()));
		}
	}

	return Super::metaMethod(shv_path, ix);
}

shv::chainpack::RpcValue BrclabNode::callMethod(const shv::iotqt::node::ShvNode::StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params)
{
	if(method == cp::Rpc::METH_DIR)
		return dir(shv_path, params);
	if(method == cp::Rpc::METH_LS)
		return ls(shv_path, params);

	return Super::callMethod(shv_path, method, params);
}
