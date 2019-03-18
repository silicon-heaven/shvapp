#include "subscriptionsnode.h"

#include "brokerapp.h"
#include "rpc/clientbrokerconnection.h"

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

SubscriptionsNode::SubscriptionsNode(rpc::ClientBrokerConnection *conn)
	: Super(nullptr)
	, m_client(conn)
{
}

size_t SubscriptionsNode::methodCount(const StringViewList &shv_path)
{
	return (shv_path.size() < 2) ?  meta_methods1.size() : meta_methods2.size();
}

const shv::chainpack::MetaMethod *SubscriptionsNode::metaMethod(const StringViewList &shv_path, size_t ix)
{
	const std::vector<cp::MetaMethod> &mms = (shv_path.size() < 2) ?  meta_methods1 : meta_methods2;
	if(mms.size() <= ix)
		SHV_EXCEPTION("Invalid method index: " + std::to_string(ix) + " of: " + std::to_string(meta_methods1.size()));
	return &(mms[ix]);
}

shv::iotqt::node::ShvNode::StringList SubscriptionsNode::childNames(const StringViewList &shv_path)
{
	//shvLogFuncFrame() << shv_path;
	if(shv_path.empty())
		return shv::iotqt::node::ShvNode::StringList{ND_BY_ID, ND_BY_PATH};
	if(shv_path[0] == ND_BY_ID) {
		shv::iotqt::node::ShvNode::StringList ret;
		for (size_t i = 0; i < m_client->subscriptionCount(); ++i) {
			ret.push_back(std::to_string(i));
		}
		return ret;
	}
	if(shv_path[0] == ND_BY_PATH) {
		shv::iotqt::node::ShvNode::StringList ret;
		for (size_t i = 0; i < m_client->subscriptionCount(); ++i) {
			const rpc::ClientBrokerConnection::Subscription &subs = m_client->subscriptionAt(i);
			ret.push_back('"' + subs.absolutePath + ':' + subs.method + '"');
		}
		return ret;
	}
	//if(shv::core::StringView(shv_path).startsWith(ND_BY_ID))
	//	return
	return shv::iotqt::node::ShvNode::StringList{};
}

shv::chainpack::RpcValue SubscriptionsNode::callMethod(const StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params)
{
	if(shv_path.size() == 2) {
		if(method == METH_PATH || method == METH_METHOD) {
			const rpc::ClientBrokerConnection::Subscription *subs = nullptr;
			if(shv_path.at(0) == ND_BY_ID) {
				subs = &m_client->subscriptionAt(std::stoul(shv_path.at(1).toString()));
			}
			else if(shv_path.at(0) == ND_BY_PATH) {
				shv::core::StringView path = shv_path.at(1);
				for (size_t i = 0; i < m_client->subscriptionCount(); ++i) {
					const rpc::ClientBrokerConnection::Subscription &subs1 = m_client->subscriptionAt(i);
					std::string p = '"' + subs1.absolutePath + ':' + subs1.method + '"';
					if(path == p) {
						subs = &subs1;
						break;
					}
				}
			}
			if(subs == nullptr)
				SHV_EXCEPTION("Method " + method + " called on invalid path " + shv_path.join('/'));
			if(method == METH_PATH)
				return subs->absolutePath;
			if(method == METH_METHOD)
				return subs->method;
		}
	}
	return Super::callMethod(shv_path, method, params);
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

