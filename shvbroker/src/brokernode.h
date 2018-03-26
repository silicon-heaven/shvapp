#pragma once

#include <shv/iotqt/node/shvnode.h>

namespace shv { namespace chainpack { class RpcValue; /*class RpcMessage; class RpcRequest;*/ }}

class BrokerNode : public shv::iotqt::node::ShvNode
{
	Q_OBJECT

	using Super = shv::iotqt::node::ShvNode;
public:
	BrokerNode(shv::iotqt::node::ShvNode *parent = nullptr);

	shv::chainpack::RpcValue dir(const shv::chainpack::RpcValue &methods_params) override;
	//shv::chainpack::RpcValue ls(shv::chainpack::RpcValue methods_params) override;
	shv::chainpack::RpcValue processRpcRequest(const shv::chainpack::RpcRequest &rq) override;
};
