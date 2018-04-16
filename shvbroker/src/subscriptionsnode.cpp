#include "subscriptionsnode.h"

#include "brokerapp.h"
#include "rpc/serverconnection.h"

#include <shv/chainpack/rpcmessage.h>
#include <shv/chainpack/rpc.h>
#include <shv/core/exception.h>
#include <shv/core/log.h>
#include <shv/core/stringview.h>

namespace cp = shv::chainpack;

SubscriptionsNode::SubscriptionsNode(rpc::ServerConnection *conn)
	: Super(nullptr)
	, m_client(conn)
{
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

