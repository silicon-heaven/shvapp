#include "lua_utils.h"
#include "hscopenode.h"
#include <lua.hpp>
#include <shv/core/utils/shvpath.h>

namespace cp = shv::chainpack;
namespace {
std::vector<cp::MetaMethod> methods {
	{cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_READ},
	{cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_READ},
};

std::vector<cp::MetaMethod> methodsWithTester {
	{cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_READ},
	{cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_READ},
	{"run", cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_READ},
};

std::vector<cp::MetaMethod> rpc_value_node_methods {
	{cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_READ},
	{cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_READ},
	{cp::Rpc::METH_GET, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsGetter, cp::Rpc::ROLE_READ},
	{cp::Rpc::SIG_VAL_CHANGED, cp::MetaMethod::Signature::VoidParam, cp::MetaMethod::Flag::IsSignal},
};
}

RpcValueNode::RpcValueNode(const std::string& name, shv::iotqt::node::ShvNode *parent)
	: Super(name, &rpc_value_node_methods, parent)
{
}

shv::chainpack::RpcValue RpcValueNode::callMethod(const shv::iotqt::node::ShvNode::StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params, const shv::chainpack::RpcValue &user_id)
{
	if (method == cp::Rpc::METH_GET) {
		return m_value;
	}

	return Super::callMethod(shv_path, method, params, user_id);
}

shv::chainpack::RpcValue RpcValueNode::getValue() const
{
	return m_value;
}

void RpcValueNode::setValue(const shv::chainpack::RpcValue& value)
{
	m_value = value;
	emit valueChanged();

	cp::RpcSignal ntf;
	ntf.setMethod(cp::Rpc::SIG_VAL_CHANGED);
	ntf.setParams((m_value));
	ntf.setShvPath(shvPath());
	emitSendRpcMessage(ntf);
}

HscopeNode::HscopeNode(const std::string& name, shv::iotqt::node::ShvNode* parent)
	: Super(name, parent)
	, m_state(nullptr)
{
}

size_t HscopeNode::methodCount(const StringViewList&)
{
	if (!m_state) {
		return methods.size();
	}

	return methodsWithTester.size();
}

const shv::chainpack::MetaMethod* HscopeNode::metaMethod(const StringViewList&, size_t index)
{
	if (!m_state) {
		return &methods.at(index);
	}

	return &methodsWithTester.at(index);
}

shv::chainpack::RpcValue HscopeNode::callMethod(const shv::iotqt::node::ShvNode::StringViewList& shv_path, const std::string &method, const shv::chainpack::RpcValue& params, const shv::chainpack::RpcValue& user_id)
{
	if (method == "run") {
		lua_getfield(m_state, LUA_REGISTRYINDEX, "testers");
		// -1) registry["tester"]

		lua_getfield(m_state, -1, m_testerLocation.c_str());
		// -2) registry["tester"]
		// -1) registry["tester"][m_testerLocation]

		auto errors = lua_pcall(m_state, 0, 0, 0);
		if (errors) {
			// -2) registry["tester"]
			// -1) error
			auto ret = std::string("Lua error: ") + lua_tostring(m_state, -1);
			lua_pop(m_state, 2);
			// <empty stack>
			return ret;
		}
		// -1) registry["tester"]

		lua_pop(m_state, 1);
		// <empty stack>

		return true;
	}

	return Super::callMethod(shv_path, method, params, user_id);
}

void HscopeNode::setStatus(const std::string& severity, const std::string& message)
{
	if (!m_statusNode) {
		return;
	}

	m_lastRunNode->setValue(shv::chainpack::RpcValue::DateTime::now());

	auto current_status = m_statusNode->getValue();
	if (current_status.asMap().value("message") == message && current_status.asMap().value("severity") == severity) {
		return;
	}

	m_statusNode->setValue(shv::chainpack::RpcValue::Map{
		{"message", message},
		{"severity", severity},
		{"time_changed", shv::chainpack::RpcValue::DateTime::now()}
	});
}

void HscopeNode::attachTester(lua_State* state, const std::string& tester_location)
{
	m_state = state;
	m_testerLocation = tester_location;
	m_statusNode = new RpcValueNode("status", this);
	m_lastRunNode = new RpcValueNode("lastRunTimestamp", this);
}
