#include "clientconnectionnode.h"
#include "brokerapp.h"
#include "rpc/serverconnection.h"

#include <shv/chainpack/metamethod.h>
#include <shv/chainpack/rpc.h>

namespace cp = shv::chainpack;

static const char M_MOUNT_POINTS[] = "mountPoints";
static const char M_DROP_CLIENT[] = "dropClient";

static std::vector<cp::MetaMethod> meta_methods {
	{cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam},
	{cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, 0, cp::Rpc::GRANT_SERVICE},
	{M_MOUNT_POINTS, cp::MetaMethod::Signature::RetVoid, 0, cp::Rpc::GRANT_SERVICE},
	{M_DROP_CLIENT, cp::MetaMethod::Signature::VoidVoid, 0, cp::Rpc::GRANT_SERVICE},
};

ClientConnectionNode::ClientConnectionNode(int client_id, shv::iotqt::node::ShvNode *parent)
	: Super(std::to_string(client_id), meta_methods, parent)
	, m_clientId(client_id)
{
}

shv::chainpack::RpcValue ClientConnectionNode::callMethod(const shv::iotqt::node::ShvNode::StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params)
{
	if(shv_path.empty()) {
		if(method == M_MOUNT_POINTS) {
			rpc::ServerConnection *cli = BrokerApp::instance()->clientById(m_clientId);
			cp::RpcValue::List ret;
			if(cli) {
				for(auto s : cli->mountPoints())
					ret.push_back(s);
			}
			return ret;
		}
		else if(method == M_DROP_CLIENT) {
			rpc::ServerConnection *cli = BrokerApp::instance()->clientById(m_clientId);
			if(cli) {
				cli->close();
				return true;
			}
			return false;
		}
	}
	return Super::callMethod(shv_path, method, params);
}

