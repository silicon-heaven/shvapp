#pragma once

#include <shv/iotqt/node/shvnode.h>

namespace rpc { class ServerConnection; }

class SubscriptionsNode : public shv::iotqt::node::ShvNode
{
	Q_OBJECT

	using Super = shv::iotqt::node::ShvNode;
public:
	SubscriptionsNode(rpc::ServerConnection *conn);

	virtual size_t methodCount(const std::string &shv_path = std::string());
	virtual const shv::chainpack::MetaMethod* metaMethod(size_t ix, const std::string &shv_path = std::string());

	virtual StringList childNames(const std::string &shv_path = std::string());

	virtual shv::chainpack::RpcValue call(const std::string &method, const shv::chainpack::RpcValue &params, const std::string &shv_path = std::string());
private:
	rpc::ServerConnection *m_client;
};
