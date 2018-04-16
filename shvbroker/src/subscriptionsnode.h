#pragma once

#include <shv/iotqt/node/shvnode.h>

namespace rpc { class ServerConnection; }

class SubscriptionsNode : public shv::iotqt::node::ShvNode
{
	Q_OBJECT

	using Super = shv::iotqt::node::ShvNode;
public:
	SubscriptionsNode(rpc::ServerConnection *conn);

	//StringList childNames(const std::string &shv_path) const override;
	//shv::chainpack::RpcValue dir(const shv::chainpack::RpcValue &methods_params) override;
	//shv::chainpack::RpcValue ls(const shv::chainpack::RpcValue &methods_params) override;
	//shv::chainpack::RpcValue processRpcRequest(const shv::chainpack::RpcRequest &rq) override;
protected:
	//shv::chainpack::RpcValue call(const std::string &node_id, const MethParamsList &mpl) override;
private:
	rpc::ServerConnection *m_client;
};
