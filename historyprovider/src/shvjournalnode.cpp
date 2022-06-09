#include "shvjournalnode.h"
#include <shv/coreqt/log.h>

namespace cp = shv::chainpack;
std::vector<cp::MetaMethod> methods {
	{cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_READ},
	{cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_READ},
};

ShvJournalNode::ShvJournalNode()
	: Super("shvjournal")
{
}

size_t ShvJournalNode::methodCount(const StringViewList&)
{
	return methods.size();
}

const cp::MetaMethod* ShvJournalNode::metaMethod(const StringViewList&, size_t index)
{
	return &methods.at(index);
}

cp::RpcValue ShvJournalNode::callMethod(const shv::iotqt::node::ShvNode::StringViewList& shv_path, const std::string &method, const cp::RpcValue& params, const cp::RpcValue& user_id)
{
	return Super::callMethod(shv_path, method, params, user_id);
}
