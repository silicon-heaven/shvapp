#include "brokernode.h"

#include <shv/chainpack/rpcmessage.h>
#include <shv/chainpack/rpc.h>
#include <shv/core/exception.h>

namespace cp = shv::chainpack;

BrokerNode::BrokerNode(shv::iotqt::node::ShvNode *parent)
	: Super(parent)
{
}

shv::chainpack::RpcValue BrokerNode::dir(const shv::chainpack::RpcValue &methods_params)
{
	Q_UNUSED(methods_params)
	static cp::RpcValue::List ret{cp::Rpc::METH_DIR, cp::Rpc::METH_PING, cp::Rpc::METH_ECHO};
	return ret;
}

shv::chainpack::RpcValue BrokerNode::processRpcRequest(const shv::chainpack::RpcRequest &rq)
{
	if(!rq.shvPath().empty())
		SHV_EXCEPTION("Subtree '" + shvPath() + "' not exists!");
	shv::chainpack::RpcValue::String method = rq.method();
	if(method == cp::Rpc::METH_PING) {
		return true;
	}
	else if(method == cp::Rpc::METH_ECHO) {
		return rq.params();
	}
	return Super::processRpcRequest(rq);
}
