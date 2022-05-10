#include "hscopeapp.h"
#include "appclioptions.h"

#include <shv/iotqt/rpc/deviceconnection.h>
#include <shv/iotqt/node/shvnodetree.h>
#include <shv/iotqt/node/localfsnode.h>
#include <shv/coreqt/log.h>
#include <shv/chainpack/tunnelctl.h>
#include <shv/chainpack/metamethod.h>
#include <shv/chainpack/cponreader.h>

#include <shv/core/stringview.h>

#include <lua.hpp>

#include <QTimer>

using namespace std;

namespace cp = shv::chainpack;
namespace si = shv::iotqt;

static std::vector<cp::MetaMethod> meta_methods {
	{cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, 0, cp::Rpc::ROLE_BROWSE},
		{cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, 0, cp::Rpc::ROLE_BROWSE},
		{cp::Rpc::METH_APP_NAME, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsGetter, cp::Rpc::ROLE_BROWSE},
		{cp::Rpc::METH_DEVICE_ID, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsGetter, cp::Rpc::ROLE_BROWSE},
		{cp::Rpc::METH_DEVICE_TYPE, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsGetter, cp::Rpc::ROLE_BROWSE},
};

size_t AppRootNode::methodCount(const StringViewList& shv_path)
{
	if (shv_path.empty()) {
		return meta_methods.size();
	}
	return 0;
}

const shv::chainpack::MetaMethod* AppRootNode::metaMethod(const StringViewList& shv_path, size_t ix)
{
	if (shv_path.empty()) {
		if (meta_methods.size() <= ix) {
			SHV_EXCEPTION("Invalid method index: " + std::to_string(ix) + " of: " + std::to_string(meta_methods.size()));
		}
		return &(meta_methods[ix]);
	}
	return nullptr;
}

shv::chainpack::RpcValue AppRootNode::callMethod(const StringViewList& shv_path, const std::string& method, const shv::chainpack::RpcValue& params, const shv::chainpack::RpcValue& user_id)
{
	if (shv_path.empty()) {
		if (method == cp::Rpc::METH_APP_NAME) {
			return QCoreApplication::instance()->applicationName().toStdString();
		}
	}
	return Super::callMethod(shv_path, method, params, user_id);
}

shv::chainpack::RpcValue AppRootNode::callMethodRq(const shv::chainpack::RpcRequest& rq)
{
	if (rq.shvPath().asString().empty()) {
		if (rq.method() == cp::Rpc::METH_DEVICE_ID) {
			HolyScopeApp* app = HolyScopeApp::instance();
			const cp::RpcValue::Map& opts = app->rpcConnection()->connectionOptions().toMap();
			const cp::RpcValue::Map& dev = opts.value(cp::Rpc::KEY_DEVICE).toMap();
			return dev.value(cp::Rpc::KEY_DEVICE_ID).toString();
		}

		if (rq.method() == cp::Rpc::METH_DEVICE_TYPE) {
			return "HolyScope";
		}
	}
	return Super::callMethodRq(rq);
}

#define DUMP_STACK(state) { \
	int top = lua_gettop(state); \
	std::cerr << "Stack:\n"; \
	std::cerr << "------\n"; \
	for (int i = 1; i <= top; i++) { \
		std::cerr << "#" << i << "\ttype: " << luaL_typename(state,i) << "\tvalue: "; \
		switch (lua_type(state, i)) { \
			case LUA_TNUMBER: \
				std::cerr << lua_tonumber(state, i); \
				break; \
			case LUA_TSTRING: \
				std::cerr << lua_tostring(state, i); \
				break; \
			case LUA_TBOOLEAN: \
				std::cerr << (lua_toboolean(state, i) ? "true" : "false"); \
				break; \
			case LUA_TNIL: \
				std::cerr << "nil"; \
				break; \
			default: \
				std::cerr << lua_topointer(state,i);; \
				break; \
		} \
		std::cerr << "\n"; \
	} \
	std::cerr << "------\n"; \
}

namespace {
template <int ArgNum, int ArgType, int... Rest>
void impl_check_lua_args(lua_State* state, const char* lua_fn_name)
{
	if (auto type = lua_type(state, ArgNum); type != ArgType) {
		luaL_error(state, "Argument #%d to `%s` must be of type `%s` (got `%s`)", ArgNum, lua_fn_name, lua_typename(state, ArgType), luaL_typename(state, ArgNum));
	}

	if constexpr (sizeof...(Rest) != 0)  {
		impl_check_lua_args<ArgNum + 1, Rest...>(state, lua_fn_name);
	}

}

template <int... ExpectedArgTypes>
void check_lua_args(lua_State* state, const char* lua_fn_name)
{
	if (auto nargs = lua_gettop(state); nargs != sizeof...(ExpectedArgTypes)) {
		luaL_error(state, "%s expects 2 arguments (got %d)", lua_fn_name, nargs);
	}

	if constexpr (sizeof...(ExpectedArgTypes) != 0) {
		impl_check_lua_args<1, ExpectedArgTypes...>(state, lua_fn_name);
	}
}
}


extern "C" {
static int on_broker_connected(lua_State* state)
{
	check_lua_args<LUA_TFUNCTION>(state, "on_broker_connected");
	// Stack:
	// 1) function

	lua_getfield(state, LUA_REGISTRYINDEX, "on_broker_connected_handlers");
	// 1) function
	// 2) registry["on_broker_connected_handlers"]

	lua_insert(state, 1);
	// 1) registry["on_broker_connected_handlers"]
	// 2) function

	lua_seti(state, 1, lua_rawlen(state, 1) + 1);
	// 1) registry["on_broker_connected_handlers"]

	lua_pop(state, 1);
	// <empty stack>
	return 0;
}

static int rpc_call(lua_State* state)
{
	check_lua_args<LUA_TSTRING, LUA_TSTRING, LUA_TSTRING, LUA_TFUNCTION>(state, "rpc_call");
	auto hscope = static_cast<HolyScopeApp*>(lua_touserdata(state, lua_upvalueindex(1)));

	// Stack:
	// 1) path
	// 2) method
	// 3) params
	// 4) callback

	auto id = hscope->callShvMethod(lua_tostring(state, 1), lua_tostring(state, 2), lua_tostring(state, 3));
	lua_getfield(state, LUA_REGISTRYINDEX, "rpc_call_handlers");
	// Stack:
	// 1) path
	// 2) method
	// 3) params
	// 4) callback
	// 5) registry["rpc_call_handlers"]

	lua_insert(state, 4);
	// Stack:
	// 1) path
	// 2) method
	// 3) params
	// 4) registry["rpc_call_handlers"]
	// 5) callback

	lua_seti(state, 4, id);
	// Stack:
	// 1) path
	// 2) method
	// 3) params
	// 4) registry["rpc_call_handlers"]

	lua_pop(state, 4);
	return 0;
}

static int subscribe_change(lua_State* state)
{
	check_lua_args<LUA_TSTRING, LUA_TFUNCTION>(state, "subscribe_change");

	// Stack:
	// 1) path
	// 2) callback

	auto hscope = static_cast<HolyScopeApp*>(lua_touserdata(state, lua_upvalueindex(1)));
	if (!hscope->rpcConnection()->isBrokerConnected()) {
		luaL_error(state, "Broker is not connected!");
	}

	auto path = std::string{lua_tostring(state, 1)};
	hscope->subscribeLua(path);

	lua_newtable(state);
	// 1) path
	// 2) callback
	// 3) the newly created table

	lua_insert(state, 1);
	// 1) the newly created table
	// 2) path
	// 3) callback

	lua_setfield(state, 1, "callback");
	// 1) the newly created table
	// 2) path

	lua_setfield(state, 1, "path");
	// 1) the newly created table

	lua_getfield(state, LUA_REGISTRYINDEX, "callbacks");
	// 1) the newly created table
	// 2) registry["callbacks"]

	lua_insert(state, 1);
	// 1) registry["callbacks"]
	// 2) the newly created table

	lua_seti(state, 1, lua_rawlen(state, 1) + 1);
	// 1) registry["callbacks"]

	lua_pop(state, 1);
	// <empty stack>
	return 0;
}
}

namespace {
void new_empty_registry_table(lua_State* state, const char* name)
{
	lua_newtable(state);
	// 1) new table

	lua_setfield(state, LUA_REGISTRYINDEX, name);
	// <empty stack>
}
}

HolyScopeApp::HolyScopeApp(int& argc, char** argv, AppCliOptions* cli_opts)
	: Super(argc, argv)
	  , m_cliOptions(cli_opts)
{
	m_rpcConnection = new si::rpc::DeviceConnection(this);

	if (!cli_opts->user_isset()) {
		cli_opts->setUser("iot");
	}

	if (!cli_opts->password_isset()) {
		cli_opts->setPassword("lub42DUB");
	}

	m_rpcConnection->setCliOptions(cli_opts);

	connect(m_rpcConnection, &si::rpc::ClientConnection::brokerConnectedChanged, this, &HolyScopeApp::onBrokerConnectedChanged);
	connect(m_rpcConnection, &si::rpc::ClientConnection::rpcMessageReceived, this, &HolyScopeApp::onRpcMessageReceived);

	AppRootNode* root = new AppRootNode();
	m_shvTree = new si::node::ShvNodeTree(root, this);
	connect(m_shvTree->root(), &si::node::ShvRootNode::sendRpcMessage, m_rpcConnection, &si::rpc::ClientConnection::sendMessage);

	QTimer::singleShot(0, m_rpcConnection, &si::rpc::ClientConnection::open);

	m_state = luaL_newstate();
	luaL_openlibs(m_state);

	lua_getglobal(m_state, "package");
	// Stack:
	// 1) global: package

	lua_getfield(m_state, 1, "path");
	// 1) global: package
	// 2) package.path

	auto new_path = std::string{lua_tostring(m_state, 2)} + ";" + m_cliOptions->configDir() + "/?;" + m_cliOptions->configDir() + "?.lua";
	lua_pushstring(m_state, new_path.c_str());
	// 1) global: package
	// 2) package.path
	// 3) the new path

	lua_setfield(m_state, 1, "path");
	// 1) global: package
	// 2) package.path

	lua_pop(m_state, 2);
	// <empty stack>

	lua_newtable(m_state);
	// 1) our library table

	luaL_Reg functions_to_register[] = {
		{"subscribe_change", subscribe_change},
		{"rpc_call", rpc_call},
		{"on_broker_connected", on_broker_connected},
		{NULL, NULL}
	};

	lua_pushlightuserdata(m_state, this);
	// 1) our library table
	// 2) `this`

	luaL_setfuncs(m_state, functions_to_register, 1);
	// 1) our library table

	lua_setglobal(m_state, "shv");
	// <empty stack>


	new_empty_registry_table(m_state, "callbacks");
	new_empty_registry_table(m_state, "rpc_call_handlers");
	new_empty_registry_table(m_state, "on_broker_connected_handlers");

	evalLuaFile(m_cliOptions->luaFile());
}

HolyScopeApp::~HolyScopeApp()
{
	shvInfo() << "destroying holy scope application";
}

HolyScopeApp* HolyScopeApp::instance()
{
	return qobject_cast<HolyScopeApp*>(QCoreApplication::instance());
}

void HolyScopeApp::onBrokerConnectedChanged(bool is_connected)
{
	m_isBrokerConnected = is_connected;

	lua_getfield(m_state, LUA_REGISTRYINDEX, "on_broker_connected_handlers");
	// Stack:
	// 1) registry["on_broker_connected_handlers"]

	lua_pushnil(m_state);
	// 1) registry["on_broker_connected_handlers"]
	// 2) nil

	while (lua_next(m_state, 1)) {
		// 1) registry["callbacks"]
		// 2) key
		// 3) on_broker_connected_handler

		lua_call(m_state, 0, 0);
		// 1) registry["callbacks"]
		// 2) key
	}

	// 1) registry["callbacks"]
	lua_pop(m_state, 1);
}

void HolyScopeApp::onRpcMessageReceived(const shv::chainpack::RpcMessage& msg)
{
	shvLogFuncFrame() << msg.toCpon();
	if (msg.isRequest()) {
		cp::RpcRequest rq(msg);
		shvDebug() << "RPC request received:" << rq.toPrettyString();
		if (m_shvTree->root()) {
			m_shvTree->root()->handleRpcRequest(rq);
		}
	} else if (msg.isResponse()) {
		cp::RpcResponse rp(msg);
		shvDebug() << "RPC response received:" << rp.toPrettyString();
		lua_getfield(m_state, LUA_REGISTRYINDEX, "rpc_call_handlers");
		// Stack:
		// 1) registry["rpc_call_handlers"]

		lua_geti(m_state, 1, rp.requestId().toInt());
		// 1) registry["rpc_call_handlers"]
		// 2) registry["rpc_call_handlers"][req_id]

		if (!lua_isnil(m_state, 2)) {
			// A handler for this req_id exists, so it's from Lua.
			auto resultStr = rp.result().toCpon();
			lua_pushlstring(m_state, resultStr.c_str(), resultStr.size());
			// 1) registry["rpc_call_handlers"]
			// 2) registry["rpc_call_handlers"][req_id]
			// 3) resultStr

			lua_call(m_state, 1, 0);
			// 1) registry["rpc_call_handlers"]

		} else {
			lua_pop(m_state, 1);
			// 1) registry["rpc_call_handlers"]
		}

		lua_pop(m_state, 1);
		// <empty stack>

	} else if (msg.isSignal()) {
		cp::RpcSignal nt(msg);
		shvDebug() << "RPC notify received:" << nt.toPrettyString();
		if (nt.method() == cp::Rpc::SIG_VAL_CHANGED) {
			lua_getfield(m_state, LUA_REGISTRYINDEX, "callbacks");
			// Stack:
			// 1) registry["callbacks"]

			lua_pushnil(m_state);
			// 1) registry["callbacks"]
			// 2) nil

			while (lua_next(m_state, 1)) {
				// 1) registry["callbacks"]
				// 2) key
				// 3) {callback: function, path: string}

				lua_getfield(m_state, 3, "path");
				// 1) registry["callbacks"]
				// 2) key
				// 3) {callback: function, path: string}
				// 4) .path

				if (nt.shvPath().asString().find(lua_tostring(m_state, 4)) == 0) {
					lua_pop(m_state, 1);
					// 1) registry["callbacks"]
					// 2) key
					// 3) {callback: function, path: string}

					lua_getfield(m_state, 3, "callback");
					// 1) registry["callbacks"]
					// 2) key
					// 3) {callback: function, path: string}
					// 4) .callback

					lua_pushstring(m_state, nt.shvPath().asString().c_str());
					// 1) registry["callbacks"]
					// 2) key
					// 3) {callback: function, path: string}
					// 4) .callback
					// 5) arg1

					auto new_value = nt.params().toStdString();
					lua_pushlstring(m_state, new_value.c_str(), new_value.size());
					// 1) registry["callbacks"]
					// 2) key
					// 3) {callback: function, path: string}
					// 4) .callback
					// 5) arg1
					// 6) arg2

					lua_call(m_state, 2, 0);
					// 1) registry["callbacks"]
					// 2) key
					// 3) {callback: function, path: string}

					lua_pop(m_state, 1);
					// 1) registry["callbacks"]
					// 2) key
				}
			}
			// 1) registry["callbacks"]

			lua_pop(m_state, 1);
			// <empty stack>
		}
	}
}

void HolyScopeApp::evalLua(const std::string& code)
{
	luaL_dostring(m_state, code.c_str());
}

void HolyScopeApp::evalLuaFile(const std::string& fileName)
{
	auto errors = luaL_dofile(m_state, fileName.c_str());
	if (errors) {
		throw std::runtime_error(lua_tostring(m_state, 1));
	}

}

void HolyScopeApp::subscribeLua(const std::string& path)
{
	m_rpcConnection->callMethodSubscribe(path, "chng");
}

int HolyScopeApp::callShvMethod(const std::string& path, const std::string& method, const shv::chainpack::RpcValue& params)
{
	return m_rpcConnection->callShvMethod(path, method, params);
}
