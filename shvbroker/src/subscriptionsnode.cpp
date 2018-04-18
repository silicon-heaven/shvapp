#include "subscriptionsnode.h"

#include "brokerapp.h"
#include "rpc/serverconnection.h"

#include <shv/chainpack/rpcmessage.h>
#include <shv/chainpack/rpc.h>
#include <shv/chainpack/metamethod.h>
#include <shv/core/exception.h>
#include <shv/core/log.h>
#include <shv/core/stringview.h>
#include <shv/core/stringview.h>

namespace cp = shv::chainpack;

namespace {
const char ND_BY_ID[] = "byId";
const char ND_BY_PATH[] = "byPath";

const char METH_PATH[] = "path";
const char METH_METHOD[] = "method";

std::vector<cp::MetaMethod> meta_methods1 {
	{cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, false},
	{cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, false},
};
std::vector<cp::MetaMethod> meta_methods2 {
	{cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, false},
	{METH_PATH, cp::MetaMethod::Signature::RetVoid, false},
	{METH_METHOD, cp::MetaMethod::Signature::RetVoid, false},
};
}

SubscriptionsNode::SubscriptionsNode(rpc::ServerConnection *conn)
	: Super(nullptr)
	, m_client(conn)
{
}

size_t SubscriptionsNode::methodCount(const std::string &shv_path)
{
	if(shv_path.empty() || shv_path == ND_BY_ID || shv_path == ND_BY_PATH)
		return meta_methods1.size();
	return meta_methods2.size();
}

const shv::chainpack::MetaMethod *SubscriptionsNode::metaMethod(size_t ix, const std::string &shv_path)
{
	const std::vector<cp::MetaMethod> &mms = (shv_path.empty() || shv_path == ND_BY_ID || shv_path == ND_BY_PATH)? meta_methods1: meta_methods2;
	if(mms.size() <= ix)
		SHV_EXCEPTION("Invalid method index: " + std::to_string(ix) + " of: " + std::to_string(meta_methods1.size()));
	return &(mms[ix]);
}

shv::iotqt::node::ShvNode::StringList SubscriptionsNode::childNames(const std::string &shv_path)
{
	if(shv_path.empty())
		return shv::iotqt::node::ShvNode::StringList{ND_BY_ID, ND_BY_PATH};
	if(shv_path == ND_BY_ID) {
		shv::iotqt::node::ShvNode::StringList ret;
		for (size_t i = 0; i < m_client->subscriptionCount(); ++i) {
			ret.push_back(std::to_string(i));
		}
		return ret;
	}
	if(shv_path == ND_BY_PATH) {
		shv::iotqt::node::ShvNode::StringList ret;
		for (size_t i = 0; i < m_client->subscriptionCount(); ++i) {
			const rpc::ServerConnection::Subscription &subs = m_client->subscriptionAt(i);
			ret.push_back(subs.pathPattern + ':' + subs.method);
		}
		return ret;
	}
	return shv::iotqt::node::ShvNode::StringList{};
}

shv::chainpack::RpcValue SubscriptionsNode::call(const std::string &method, const shv::chainpack::RpcValue &params, const std::string &shv_path)
{
	if(method == METH_PATH || method == METH_METHOD) {
		std::vector<shv::core::StringView> lst = shv::core::StringView(shv_path).split('/');
		const rpc::ServerConnection::Subscription *subs = nullptr;
		if(lst.at(0) == ND_BY_ID) {
			subs = &m_client->subscriptionAt(std::stoul(lst.at(1).toString()));
		}
		else if(lst.at(0) == ND_BY_PATH) {
			shv::core::StringView path = lst.at(1);
			for (size_t i = 0; i < m_client->subscriptionCount(); ++i) {
				const rpc::ServerConnection::Subscription &subs1 = m_client->subscriptionAt(i);
				std::string p = subs1.pathPattern + ':' + subs1.method;
				if(path == p) {
					subs = &subs1;
					break;
				}
			}
		}
		if(subs == nullptr)
			SHV_EXCEPTION("Method " + method + " called on invalid path " + shv_path);
		if(method == METH_PATH)
			return subs->pathPattern;
		if(method == METH_METHOD)
			return subs->method;
	}
	return Super::call(method, params, shv_path);
}

/*
shv::iotqt::node::ShvNode::StringList SubscriptionsNode::childNames(const std::string &shv_path) const
{
	shv::iotqt::node::ShvNode::StringList ret;
	if(shv_path.empty()) {
		for (size_t i = 0; i < m_client->subscriptionCount(); ++i)
			ret.push_back(std::to_string(i));
	}
	return ret;
}

shv::chainpack::RpcValue SubscriptionsNode::processRpcRequest(const shv::chainpack::RpcRequest &rq)
{
	shv::chainpack::RpcValue shv_path = rq.shvPath();
	shv::core::StringView sv(shv_path.toString());
	if(sv.empty())
		return Super::processRpcRequest(rq);
	size_t ix = std::stoul(sv.getToken('/').toString());
	const rpc::ServerConnection::Subscription &su = m_client->subscriptionAt(ix);
	const std::string method = rq.method().toString();
}
*/

