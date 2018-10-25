#include "brokerapp.h"
#include "fstabnode.h"

#include <shv/chainpack/metamethod.h>
#include <shv/chainpack/rpc.h>

namespace cp = shv::chainpack;

//static const char M_SIZE[] = "size";
static const char M_READ[] = "read";
static const char M_WRITE[] = "write";
//static const char M_RELOAD[] = "serverReload";

static std::vector<cp::MetaMethod> meta_methods {
	{cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, 0, cp::MetaMethod::AccessLevel::Read},
	{cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, 0, cp::MetaMethod::AccessLevel::Read},
	//{M_SIZE, cp::MetaMethod::Signature::RetVoid, 0, cp::MetaMethod::AccessLevel::Read},
	{M_READ, cp::MetaMethod::Signature::RetVoid, 0, cp::MetaMethod::AccessLevel::Read},
	{M_WRITE, cp::MetaMethod::Signature::RetParam, 0, cp::MetaMethod::AccessLevel::Service},
	//{M_RELOAD, cp::MetaMethod::Signature::VoidVoid, 0, cp::MetaMethod::AccessLevel::Service},
};

FSTabNode::FSTabNode(shv::iotqt::node::ShvNode *parent)
: Super("fstab", meta_methods, parent)
{
}

shv::chainpack::RpcValue FSTabNode::callMethod(const shv::iotqt::node::ShvNode::StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params)
{
	if(shv_path.empty()) {
		if(method == M_READ) {
			return BrokerApp::instance()->fstabCpon();
		}
		else if(method == M_WRITE) {
			std::string cpon = params.toString();
			BrokerApp::instance()->saveFstabCpon(cpon);
			return true;
		}
	}
	return Super::callMethod(shv_path, method, params);
}
