#include "hscopenode.h"

namespace cp = shv::chainpack;
namespace {
std::vector<cp::MetaMethod> methods {
	{cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_READ},
	{cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_READ},
	{"status", cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsGetter, cp::Rpc::ROLE_READ},
};
}

HscopeNode::HscopeNode(const std::string& name, shv::iotqt::node::ShvNode* parent)
	: Super(name, &methods, parent)
	, m_status(name + ": dummy status")
{
}

shv::chainpack::RpcValue HscopeNode::callMethod(const shv::iotqt::node::ShvNode::StringViewList& shv_path, const std::string &method, const shv::chainpack::RpcValue& params, const shv::chainpack::RpcValue& user_id)
{
	if (method == "status") {
		return m_status;
	}

	return Super::callMethod(shv_path, method, params, user_id);
}
