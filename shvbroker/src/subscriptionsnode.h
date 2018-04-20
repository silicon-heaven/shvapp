#pragma once

#include <shv/iotqt/node/shvtreenode.h>

namespace rpc { class ServerConnection; }

class SubscriptionsNode : public shv::iotqt::node::ShvTreeNode
{
	Q_OBJECT

	using Super = shv::iotqt::node::ShvTreeNode;
public:
	SubscriptionsNode(rpc::ServerConnection *conn);

	virtual size_t methodCount2(const std::string &shv_path = std::string());
	virtual const shv::chainpack::MetaMethod* metaMethod2(size_t ix, const std::string &shv_path = std::string());

	virtual StringList childNames2(const std::string &shv_path = std::string());

	virtual shv::chainpack::RpcValue call2(const std::string &method, const shv::chainpack::RpcValue &params, const std::string &shv_path = std::string());
private:
	rpc::ServerConnection *m_client;
};
