#pragma once

#include <shv/iotqt/shvnode.h>

namespace shv { namespace chainpack { class RpcValue; /*class RpcMessage; class RpcRequest;*/ }}

class BrokerNode : public shv::iotqt::ShvNode
{
	Q_OBJECT

	using Super = shv::iotqt::ShvNode;
public:
	BrokerNode(QObject *parent = nullptr);

	shv::chainpack::RpcValue dir(const shv::chainpack::RpcValue &methods_params) override;
	//shv::chainpack::RpcValue ls(shv::chainpack::RpcValue methods_params) override;
	shv::chainpack::RpcValue processRpcRequest(const shv::chainpack::RpcRequest &rq) override;
};
