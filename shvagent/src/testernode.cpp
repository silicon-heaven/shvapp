#include "testernode.h"

#include <shv/core/utils/shvpath.h>

namespace cp = shv::chainpack;

//=====================================================================
// TesterPropertyNode
//=====================================================================
static std::vector<cp::MetaMethod> meta_methods_tester {
	{cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_READ},
	{cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_READ},
};

TesterNode::TesterNode(ShvNode *parent)
	: Super("tester", &meta_methods_tester, parent)
{
	TesterPropertyNode *nd1 = new TesterPropertyNode("prop1", this);
	new TesterPropertyNode("propA", nd1);

	new TesterPropertyNode("prop2", this);
	new TesterPropertyNode("prop3", this);
	new TesterPropertyNode("prop4", this);
}

//=====================================================================
// TesterPropertyNode
//=====================================================================
static std::vector<cp::MetaMethod> meta_methods_property {
	{cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_READ},
	{cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_READ},
	{cp::Rpc::METH_GET, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsGetter, cp::Rpc::ROLE_READ},
	{cp::Rpc::METH_SET, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsSetter, cp::Rpc::ROLE_WRITE},
	{cp::Rpc::SIG_VAL_CHANGED, cp::MetaMethod::Signature::VoidParam, cp::MetaMethod::Flag::IsSignal},
};

TesterPropertyNode::TesterPropertyNode(const std::string &name, shv::iotqt::node::ShvNode *parent)
	: Super(name, &meta_methods_property, parent)
{
}

shv::chainpack::RpcValue TesterPropertyNode::callMethod(const shv::iotqt::node::ShvNode::StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params, const shv::chainpack::RpcValue &user_id)
{
	if(shv_path.empty()) {
		if(method == cp::Rpc::METH_GET) {
			int max_age = cp::DataChange::DONT_CARE_TS;
			if(params.isInt())
				max_age = params.toInt();
			if(max_age == cp::DataChange::DONT_CARE_TS)
				return m_dataChange.value().isValid()? m_dataChange.value(): cp::RpcValue(nullptr);
			return m_dataChange.value().isValid()? m_dataChange.toRpcValue(): cp::RpcValue(nullptr);
		}
		if(method == cp::Rpc::METH_SET) {
			cp::RpcValue old_val = m_dataChange.value();
			m_dataChange = cp::DataChange(params, cp::RpcValue::DateTime::now());
			if(params != old_val) {
				cp::RpcSignal ntf;
				ntf.setMethod(cp::Rpc::SIG_VAL_CHANGED);
				ntf.setParams(m_dataChange.toRpcValue());
				ntf.setShvPath(shvPath());
				//shvInfo() << "LublicatorPropertyValueChanged:" << ntf.toCpon();
				rootNode()->emitSendRpcMessage(ntf);
			}
			return true;
		}
	}
	return Super::callMethod(shv_path, method, params, user_id);
}
