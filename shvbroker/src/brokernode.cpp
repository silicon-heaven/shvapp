#include "brokernode.h"

#include "brokerapp.h"

#include <shv/chainpack/metamethod.h>
#include <shv/chainpack/rpcmessage.h>
#include <shv/chainpack/rpc.h>
#include <shv/core/exception.h>
#include <shv/core/log.h>

namespace cp = shv::chainpack;

BrokerNode::BrokerNode(shv::iotqt::node::ShvNode *parent)
	: Super(parent)
{
}

shv::chainpack::RpcValue BrokerNode::call2(const std::string &method, const shv::chainpack::RpcValue &params, const std::string &shv_path)
{
	if(!shv_path.empty())
		SHV_EXCEPTION("Subtree '" + shv_path + "' not exists!");
	if(method == cp::Rpc::METH_PING) {
		return true;
	}
	if(method == cp::Rpc::METH_ECHO) {
		return params;
	}
	return Super::call(method, params);
}

shv::iotqt::node::ShvNode::StringList BrokerNode::childNames2(const std::string &shv_path)
{
	Q_UNUSED(shv_path)
	return shv::iotqt::node::ShvNode::StringList{};
}

static std::vector<cp::MetaMethod> meta_methods {
	{cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, false},
	{cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, false},
	{cp::Rpc::METH_PING, cp::MetaMethod::Signature::VoidVoid, false},
	{cp::Rpc::METH_ECHO, cp::MetaMethod::Signature::RetParam, false},
	{cp::Rpc::METH_SUBSCRIBE, cp::MetaMethod::Signature::RetParam, false},
};

size_t BrokerNode::methodCount2(const std::string &shv_path)
{
	Q_UNUSED(shv_path)
	return meta_methods.size();
}

const cp::MetaMethod *BrokerNode::metaMethod2(size_t ix, const std::string &shv_path)
{
	Q_UNUSED(shv_path)
	if(meta_methods.size() <= ix)
		SHV_EXCEPTION("Invalid method index: " + std::to_string(ix) + " of: " + std::to_string(meta_methods.size()));
	return &(meta_methods[ix]);
}
/*
shv::chainpack::RpcValue BrokerNode::dir(const std::string &shv_path, const shv::chainpack::RpcValue &methods_params)
{
	Q_UNUSED(methods_params)
	Q_UNUSED(shv_path)
	static cp::RpcValue::List ret{cp::Rpc::METH_DIR, cp::Rpc::METH_PING, cp::Rpc::METH_ECHO, cp::Rpc::METH_SUBSCRIBE};
	return ret;
}
*/
shv::chainpack::RpcValue BrokerNode::processRpcRequest(const shv::chainpack::RpcRequest &rq)
{
	if(!rq.shvPath().toString().empty())
		SHV_EXCEPTION("Subtree '" + shvPath() + "' not exists!");
	shv::chainpack::RpcValue method = rq.method();
	if(method.toString() == cp::Rpc::METH_SUBSCRIBE) {
		const shv::chainpack::RpcValue parms = rq.params();
		const shv::chainpack::RpcValue::Map &pm = parms.toMap();
		std::string path = pm.value(cp::Rpc::PAR_PATH).toString();
		std::string method = pm.value(cp::Rpc::PAR_METHOD).toString();
		shv::chainpack::RpcRequest rq2 = rq;
		int client_id = rq2.popCallerId();
		BrokerApp::instance()->createSubscription(client_id, path, method);
		return true;
	}
	return Super::processRpcRequest(rq);
}

