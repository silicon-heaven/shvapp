#include "sitesnode.h"
#include "shvfileproviderapp.h"

#include <shv/chainpack/metamethod.h>
#include <shv/chainpack/rpcvalue.h>
#include <shv/chainpack/rpc.h>
#include <shv/iotqt/utils/fileshvjournal.h>

namespace cp = shv::chainpack;
namespace iot = shv::iotqt;

static const char M_READ[] = "read";

static std::vector<cp::MetaMethod> meta_methods_sites {
	{cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, false},
	{cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, false},
	{M_READ, cp::MetaMethod::Signature::RetParam, false},
};

SitesNode::SitesNode(shv::iotqt::node::ShvNode *parent)
	: Super("sites", parent)
{
}

size_t SitesNode::methodCount(const StringViewList &shv_path)
{
	return (shv_path.empty()) ? meta_methods_sites.size() : 0;
}

const shv::chainpack::MetaMethod *SitesNode::metaMethod(const StringViewList &shv_path, size_t ix)
{
	return (shv_path.empty()) ? &(meta_methods_sites.at(ix)) : nullptr;
}

shv::chainpack::RpcValue SitesNode::callMethod(const shv::iotqt::node::ShvNode::StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params)
{
	if(shv_path.empty()) {
		if(method == M_READ) {
			return ShvFileProviderApp::instance()->getSites();
		}
	}
	return Super::callMethod(shv_path, method, params);
}
