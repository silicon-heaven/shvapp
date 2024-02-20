#pragma once

#include "slavehpinfo.h"
#include <shv/iotqt/node/shvnode.h>
#include <set>

class ValueCacheNode : public shv::iotqt::node::ShvNode
{
	Q_OBJECT

	using Super = shv::iotqt::node::ShvNode;

public:
	ValueCacheNode(ShvNode* parent = nullptr);

	size_t methodCount(const StringViewList& shv_path) override;
	const shv::chainpack::MetaMethod* metaMethod(const StringViewList& shv_path, size_t ix) override;
	shv::chainpack::RpcValue callMethodRq(const shv::chainpack::RpcRequest &rq) override;

	Q_SIGNAL void valueChanged(const std::string& path, const shv::chainpack::RpcValue& value);

private:
	void onRpcMessageReceived(const shv::chainpack::RpcMessage &msg);
	void saveToCache(const std::string& path, const shv::chainpack::RpcValue& value);
	// path -> value
	shv::chainpack::RpcValue::Map m_cache;
};
