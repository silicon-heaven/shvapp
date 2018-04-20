#pragma once

#include <shv/iotqt/node/shvtreenode.h>

namespace shv { namespace chainpack { class RpcValue; class RpcRequest;/*class RpcMessage; */ }}

class BrokerNode : public shv::iotqt::node::ShvTreeNode
{
	Q_OBJECT

	using Super = shv::iotqt::node::ShvTreeNode;
public:
	BrokerNode(shv::iotqt::node::ShvNode *parent = nullptr);

	shv::chainpack::RpcValue processRpcRequest(const shv::chainpack::RpcRequest &rq) override;

	shv::chainpack::RpcValue call2(const std::string &method, const shv::chainpack::RpcValue &params, const std::string &shv_path = std::string()) override;

	StringList childNames2(const std::string &shv_path) override;

	size_t methodCount2(const std::string &shv_path = std::string()) override;
	const shv::chainpack::MetaMethod* metaMethod2(size_t ix, const std::string &shv_path = std::string()) override;
};
