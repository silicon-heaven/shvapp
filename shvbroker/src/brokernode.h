#pragma once

#include <shv/iotqt/node/shvnode.h>
#include <shv/chainpack/rpcvalue.h>

//namespace shv { namespace chainpack { class RpcRequest;/*class RpcMessage; */ }}

class BrokerNode : public shv::iotqt::node::ShvNode
{
	Q_OBJECT

	using Super = shv::iotqt::node::ShvNode;
public:
	BrokerNode(shv::iotqt::node::ShvNode *parent = nullptr);

	//void processRawRpcRequest(shv::chainpack::RpcValue::MetaData &&meta, const std::string &data) override;
	shv::chainpack::RpcValue processRpcRequest(const shv::chainpack::RpcRequest &rq) override;
	shv::chainpack::RpcValue callMethod(const StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params) override;

	StringList childNames(const StringViewList &shv_path) override;

	size_t methodCount(const StringViewList &shv_path) override;
	const shv::chainpack::MetaMethod* metaMethod(const StringViewList &shv_path, size_t ix) override;
};
