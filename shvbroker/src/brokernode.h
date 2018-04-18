#pragma once

#include <shv/iotqt/node/shvnode.h>

namespace shv { namespace chainpack { class RpcValue; class RpcRequest;/*class RpcMessage; */ }}

class BrokerNode : public shv::iotqt::node::ShvNode
{
	Q_OBJECT

	using Super = shv::iotqt::node::ShvNode;
public:
	BrokerNode(shv::iotqt::node::ShvNode *parent = nullptr);

	shv::chainpack::RpcValue processRpcRequest(const shv::chainpack::RpcRequest &rq) override;

	//size_t childCount(const std::string &shv_path = std::string()) override;
	//std::string childName(size_t ix, const std::string &shv_path = std::string()) override;
	shv::chainpack::RpcValue call(const std::string &method, const shv::chainpack::RpcValue &params, const std::string &shv_path = std::string()) override;
	//shv::chainpack::RpcValue ls(const shv::chainpack::RpcValue &methods_params, const std::string &shv_path = std::string()) override;

	//shv::chainpack::RpcValue dir(const shv::chainpack::RpcValue &methods_params, const std::string &shv_path = std::string()) override;
	size_t methodCount(const std::string &shv_path = std::string()) override;
	const shv::chainpack::MetaMethod* metaMethod(size_t ix, const std::string &shv_path = std::string()) override;
};
