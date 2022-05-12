#include "lua_utils.h"
#include "hscopenode.h"
#include <lua.hpp>

namespace cp = shv::chainpack;
namespace {
std::vector<cp::MetaMethod> methods {
	{cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_READ},
	{cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_READ},
};

std::vector<cp::MetaMethod> methodsWithTester {
	{cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_READ},
	{cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_READ},
	{"status", cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsGetter, cp::Rpc::ROLE_READ},
	{"run", cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_READ},
};
}

HscopeNode::HscopeNode(const std::string& name, shv::iotqt::node::ShvNode* parent)
	: Super(name, &methods, parent)
{
}

HscopeNode::HscopeNode(const std::string &name, lua_State* state, const std::string& testerLocation, shv::iotqt::node::ShvNode *parent)
	: Super(name, &methodsWithTester, parent)
	, m_state(state)
	, m_testerLocation(testerLocation)
{
}

extern "C" {
static int set_status(lua_State* state)
{
	check_lua_args<LUA_TSTRING>(state, "set_status");
	// Stack:
	// 1) string
	auto node = static_cast<HscopeNode*>(lua_touserdata(state, lua_upvalueindex(1)));
	node->setStatus(lua_tostring(state, 1));

	lua_pop(state, 1);
	// <empty stack>
	return 0;
}
}

shv::chainpack::RpcValue HscopeNode::callMethod(const shv::iotqt::node::ShvNode::StringViewList& shv_path, const std::string &method, const shv::chainpack::RpcValue& params, const shv::chainpack::RpcValue& user_id)
{
	if (method == "status") {
		return m_status;
	}

	if (method == "run") {
		lua_getfield(m_state, LUA_REGISTRYINDEX, "testers");
		// -1) registry["tester"]

		lua_getfield(m_state, -1, m_testerLocation.c_str());
		// -2) registry["tester"]
		// -1) registry["tester"][m_testerLocation]

		lua_pushlightuserdata(m_state, this);
		// -3) registry["tester"]
		// -2) registry["tester"][m_testerLocation]
		// -1) this

		lua_pushcclosure(m_state, set_status, 1);
		// -3) registry["tester"]
		// -2) registry["tester"][m_testerLocation]
		// -1) set_status

		auto errors = lua_pcall(m_state, 1, 0, 0);
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

void HscopeNode::setStatus(const std::string& status)
{
	m_status = status;
}
