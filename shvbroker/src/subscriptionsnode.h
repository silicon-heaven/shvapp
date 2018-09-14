#pragma once

#include <shv/iotqt/node/shvnode.h>

namespace rpc { class ServerConnection; }

class SubscriptionsNode : public shv::iotqt::node::ShvNode
{
	Q_OBJECT

	using Super = shv::iotqt::node::ShvNode;
public:
	SubscriptionsNode(rpc::ServerConnection *conn);

	size_t methodCount(const StringViewList &shv_path) override;
	const shv::chainpack::MetaMethod* metaMethod(const StringViewList &shv_path, size_t ix) override;

	StringList childNames(const StringViewList &shv_path) override;

	shv::chainpack::RpcValue callMethod(const StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params) override;
private:
	rpc::ServerConnection *m_client;
};
