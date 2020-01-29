#include "historynode.h"
#include "revitestapp.h"

#include <shv/chainpack/metamethod.h>
#include <shv/chainpack/rpcvalue.h>
#include <shv/chainpack/rpc.h>
#include <shv/core/utils/fileshvjournal.h>

namespace cp = shv::chainpack;
namespace iot = shv::iotqt;

static std::vector<cp::MetaMethod> meta_methods_history {
	{cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, false},
	{cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, false},
	{cp::Rpc::METH_GET_LOG, cp::MetaMethod::Signature::RetParam, false},
};

HistoryNode::HistoryNode(shv::iotqt::node::ShvNode *parent)
	: Super(".history", parent)
{
}

size_t HistoryNode::methodCount(const StringViewList &shv_path)
{
	if(shv_path.empty())
		return meta_methods_history.size();
	if(shv_path.size() == 1)
		return meta_methods_history.size();
	return 0;
}

const shv::chainpack::MetaMethod *HistoryNode::metaMethod(const StringViewList &shv_path, size_t ix)
{
	if(shv_path.empty())
		return &(meta_methods_history.at(ix));
	if(shv_path.size() == 1)
		return &(meta_methods_history.at(ix));
	return nullptr;
}

shv::chainpack::RpcValue HistoryNode::callMethod(const shv::iotqt::node::ShvNode::StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params)
{
	if(shv_path.empty()) {
		if(method == cp::Rpc::METH_GET_LOG) {
			return RevitestApp::instance()->shvJournal()->getLog(shv::core::utils::ShvGetLogParams(params));
		}
	}
	return Super::callMethod(shv_path, method, params);
}
