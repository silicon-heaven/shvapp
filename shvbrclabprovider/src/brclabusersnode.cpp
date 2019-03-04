#include "brclabusersnode.h"
#include "shv/chainpack/rpcvalue.h"
#include "shvbrclabproviderapp.h"

#include <shv/core/exception.h>

namespace cp = shv::chainpack;

static const char M_ADD_USER[] = "addUser";
static const char M_CHANGE_USER_PASSWORD[] = "changeUserPassword";
static const char M_GET_USER_GRANTS[] = "getUserGrants";

static std::vector<cp::MetaMethod> meta_methods {
	{M_ADD_USER, cp::MetaMethod::Signature::RetParam, 0, cp::Rpc::GRANT_WRITE},
	{M_CHANGE_USER_PASSWORD, cp::MetaMethod::Signature::RetParam, 0, cp::Rpc::GRANT_WRITE},
	{M_GET_USER_GRANTS, cp::MetaMethod::Signature::RetParam, 0, cp::Rpc::GRANT_WRITE}
};

BrclabUsersNode::BrclabUsersNode(const std::string &config_name, ShvNode *parent)
	: Super(config_name, parent),
	  m_brclabUsers(ShvBrclabProviderApp::instance()->brclabUsersFileName(), this)
{

}

size_t BrclabUsersNode::methodCount(const shv::iotqt::node::ShvNode::StringViewList &shv_path)
{
	return (shv_path.empty()) ? meta_methods.size() + Super::methodCount(shv_path) : Super::methodCount(shv_path);
}

const shv::chainpack::MetaMethod *BrclabUsersNode::metaMethod(const shv::iotqt::node::ShvNode::StringViewList &shv_path, size_t ix)
{
	if(shv_path.empty()) {
		size_t all_method_count = meta_methods.size() + Super::methodCount(shv_path);

		if (ix < meta_methods.size())
			return &(meta_methods[ix]);
		else if (ix < all_method_count)
			return Super::metaMethod(shv_path, ix - meta_methods.size());
		else
			SHV_EXCEPTION("Invalid method index: " + std::to_string(ix) + " of: " + std::to_string(all_method_count));
	}

	return Super::metaMethod(shv_path, ix);
}

shv::chainpack::RpcValue BrclabUsersNode::callMethod(const shv::iotqt::node::ShvNode::StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params)
{
	if(shv_path.empty()) {
		if(method == M_ADD_USER) {
			return m_brclabUsers.addUser(params);
		}
		else if(method == M_CHANGE_USER_PASSWORD) {
			return m_brclabUsers.changePassword(params);
		}
		else if(method == M_GET_USER_GRANTS) {
			return m_brclabUsers.getUserGrants(params);
		}
	}

	return Super::callMethod(shv_path, method, params);
}

shv::chainpack::RpcValue BrclabUsersNode::loadValues()
{
	return  m_brclabUsers.loadUsersConfig();
}

bool BrclabUsersNode::saveValues(const shv::chainpack::RpcValue &vals)
{
}

