#include "hscopeapp.h"
#include "hscopenode.h"
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
#include "lua_utils.h"

#include <QDirIterator>
#include <QTimer>

using namespace std;

#define logLuaD() shvCDebug("Lua")
#define logLuaI() shvCInfo("Lua")
#define logLuaW() shvCWarning("Lua")
#define logLuaE() shvCError("Lua")

#define LUA_LOG_FN(name, logger) \
static int name(lua_State* state) \
{ \
	auto sg = StackGuard(state, 0); \
 \
	for (auto i = 0; i < lua_gettop(state); i++) { \
		logger() << lua_tostring(state, i + 1); \
	} \
	lua_pop(state, lua_gettop(state)); \
 \
	return 0; \
}

extern "C" {
LUA_LOG_FN(log_debug, logLuaD);
LUA_LOG_FN(log_info, logLuaI);
LUA_LOG_FN(log_warning, logLuaW);
LUA_LOG_FN(log_error, logLuaE);
}

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

extern "C" {
static int on_broker_connected(lua_State* state)
{
	auto sg = StackGuard(state, 0);
	check_lua_args<LUA_TFUNCTION>(state, "on_broker_connected");
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
	auto sg = StackGuard(state, 0);
	check_lua_args<LUA_TSTRING, LUA_TSTRING, LUA_TSTRING, LUA_TFUNCTION>(state, "rpc_call");
	// 1) path
	// 2) method
	// 3) params
	// 4) callback

	auto hscope = static_cast<HolyScopeApp*>(lua_touserdata(state, lua_upvalueindex(1)));

	auto id = hscope->callShvMethod(lua_tostring(state, 1), lua_tostring(state, 2), lua_tostring(state, 3));
	lua_getfield(state, LUA_REGISTRYINDEX, "rpc_call_handlers");
	// 1) path
	// 2) method
	// 3) params
	// 4) callback
	// 5) registry["rpc_call_handlers"]

	lua_insert(state, 4);
	// 1) path
	// 2) method
	// 3) params
	// 4) registry["rpc_call_handlers"]
	// 5) callback

	lua_seti(state, 4, id);
	// 1) path
	// 2) method
	// 3) params
	// 4) registry["rpc_call_handlers"]

	lua_pop(state, 4);
	return 0;
}

static int subscribe(lua_State* state)
{
	auto sg = StackGuard(state, 0);
	check_lua_args<LUA_TSTRING, LUA_TSTRING, LUA_TFUNCTION>(state, "subscribe");
	// 1) path
	// 2) type
	// 3) callback

	auto hscope = static_cast<HolyScopeApp*>(lua_touserdata(state, lua_upvalueindex(1)));
	if (!hscope->rpcConnection()->isBrokerConnected()) {
		luaL_error(state, "Broker is not connected!");
	}

	auto path = std::string{lua_tostring(state, 1)};
	auto type = std::string{lua_tostring(state, 2)};
	hscope->subscribeLua(path, type);

	lua_newtable(state);
	// 1) path
	// 2) type
	// 3) callback
	// 4) the newly created table

	lua_insert(state, 1);
	// 1) the newly created table
	// 2) path
	// 3) type
	// 4) callback

	lua_setfield(state, 1, "callback");
	// 1) the newly created table
	// 2) path
	// 3) type

	lua_setfield(state, 1, "type");
	// 1) the newly created table
	// 2) path

	lua_setfield(state, 1, "path");
	// 1) the newly created table

	lua_getfield(state, LUA_REGISTRYINDEX, "subscribe_callbacks");
	// 1) the newly created table
	// 2) registry["subscribe_callbacks"]

	lua_insert(state, 1);
	// 1) registry["subscribe_callbacks"]
	// 2) the newly created table

	lua_seti(state, 1, lua_rawlen(state, 1) + 1);
	// 1) registry["subscribe_callbacks"]

	lua_pop(state, 1);
	// <empty stack>
	return 0;
}
}

namespace {
void new_empty_registry_table(lua_State* state, const char* name)
{
	auto sg = StackGuard(state);
	lua_newtable(state);
	// 1) new table

	lua_setfield(state, LUA_REGISTRYINDEX, name);
	// <empty stack>
}

void handle_lua_error(lua_State* state, const std::string& where)
{
	shvError() << "Error in" << where;
	shvError() << lua_tostring(state, -1);
	lua_pop(state, 1);
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
	auto sg = StackGuard(m_state);
	luaL_openlibs(m_state);

	lua_getglobal(m_state, "package");
	// 1) global: package

	lua_getfield(m_state, 1, "path");
	// 1) global: package
	// 2) package.path

	auto new_path = std::string{lua_tostring(m_state, 2)} + ";" + m_cliOptions->configDir() + "/lua-lib/?;" + m_cliOptions->configDir() + "/lua-lib/?.lua";
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
		{"subscribe", subscribe},
		{"rpc_call", rpc_call},
		{"on_broker_connected", on_broker_connected},
		{"log_debug", log_debug},
		{"log_info", log_info},
		{"log_warning", log_warning},
		{"log_error", log_error},
		{NULL, NULL}
	};

	lua_pushlightuserdata(m_state, this);
	// 1) our library table
	// 2) `this`

	luaL_setfuncs(m_state, functions_to_register, 1);
	// 1) our library table

	lua_setglobal(m_state, "shv");
	// <empty stack>


	new_empty_registry_table(m_state, "subscribe_callbacks");
	new_empty_registry_table(m_state, "rpc_call_handlers");
	new_empty_registry_table(m_state, "on_broker_connected_handlers");
	new_empty_registry_table(m_state, "testers");

	if (auto conf_dir = QDir{QString::fromStdString(m_cliOptions->configDir())}; conf_dir.exists()) {
		if (conf_dir.cd("instances")) {
			// resolveLua expects an environment on the top of the stack. The first environment is empty.
			lua_newtable(m_state);
			// 1) new environment

			lua_newtable(m_state);
			// 1) new environment
			// 2) metatable for the new environment

			lua_pushglobaltable(m_state);
			// 1) new environment
			// 2) metatable for the new environment
			// 3) the global environment

			lua_setfield(m_state, 2, "__index");
			// 1) new environment
			// 2) metatable for the new environment

			lua_setmetatable(m_state, 1);
			// 1) new environment

			resolveConfTree(conf_dir, root);
			lua_pop(m_state, 1);
			// <empty stack>
		}
	}

	if (lua_gettop(m_state) != 0) {
		DUMP_STACK(m_state);
		throw std::runtime_error("Something went wrong, Lua stack's equilibrium wasn't maintained.");
	}
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

	auto sg = StackGuard(m_state);

	lua_getfield(m_state, LUA_REGISTRYINDEX, "on_broker_connected_handlers");
	// 1) registry["on_broker_connected_handlers"]

	lua_pushnil(m_state);
	// 1) registry["on_broker_connected_handlers"]
	// 2) nil

	while (lua_next(m_state, 1)) {
		// 1) registry["on_broker_connected_handlers"]
		// 2) key
		// 3) on_broker_connected_handler

		auto errors = lua_pcall(m_state, 0, 0, 0);
		// 1) registry["on_broker_connected_handlers"]
		// 2) key

		if (errors) {
			handle_lua_error(m_state, "on_broker_connected_handler");
		}
	}

	// 1) registry["on_broker_connected_handlers"]
	lua_pop(m_state, 1);
	// <empty stack>
}

namespace {
void push_rpc_value(lua_State* state, const shv::chainpack::RpcValue& value)
{
	auto sg = StackGuard(state, lua_gettop(state) + 1);
	lua_newtable(state);
	// -1) new table

	switch (value.type()) {
	case shv::chainpack::RpcValue::Type::String: {
		auto str_val = value.toString();
		lua_pushlstring(state, str_val.c_str(), str_val.size());
		// -2) new table
		// -1) str_val
		break;
	}
	case shv::chainpack::RpcValue::Type::Int:
		lua_pushinteger(state, value.toInt());
		// -2) new table
		// -1) int
		break;
	case shv::chainpack::RpcValue::Type::UInt:
		lua_pushinteger(state, value.toUInt());
		// -2) new table
		// -1) int
		break;
	case shv::chainpack::RpcValue::Type::Bool:
		lua_pushboolean(state, value.toBool());
		// -2) new table
		// -1) bool
		break;
	case shv::chainpack::RpcValue::Type::Double:
		lua_pushnumber(state, value.toDouble());
		// -2) new table
		// -1) double
		break;
	case shv::chainpack::RpcValue::Type::Null:
		lua_pushnil(state);
		// -2) new table
		// -1) nil
		break;
	case shv::chainpack::RpcValue::Type::List:
		lua_newtable(state);
		// -2) new table
		// -1) table for list

		for (const auto& value : value.asList()) {
			push_rpc_value(state, value);
			// -3) new table
			// -2) table for list
			// -1) new value for list

			lua_seti(state, -1, lua_rawlen(state, 1) + 1);
			// -2) new table
			// -1) table for list
		}
		break;
	case shv::chainpack::RpcValue::Type::IMap:
		lua_newtable(state);
		// -2) new table
		// -1) table for IMap
		for (const auto& [k, v] : value.asIMap()) {
			push_rpc_value(state, v);
			// -3) new table
			// -2) table for map
			// -1) value for map table

			lua_seti(state, -2, k + 1/* lua arrays are indexed from 1 */);
			// -2) new table
			// -1) table for map
		}
		break;
	case shv::chainpack::RpcValue::Type::Map:
		lua_newtable(state);
		// -2) new table
		// -1) table for Map

		for (const auto& [k, v] : value.asMap()) {
			push_rpc_value(state, v);
			// -3) new table
			// -2) table for map
			// -1) value for map table

			lua_setfield(state, -2, k.c_str());
			// -2) new table
			// -1) table for map
		}
		break;
	case shv::chainpack::RpcValue::Type::DateTime:
		lua_newtable(state);
		// -2) new table
		// -1) table for DateTime

		lua_pushinteger(state, value.toDateTime().msecsSinceEpoch());
		// -3) new table
		// -2) table for DateTime
		// -1) msecsSinceEpoch

		lua_setfield(state, -2, "msecsSinceEpoch");
		// -2) new table
		// -1) table for DateTime
		break;
	default:
		shvWarning() << "Can't convert RpcValue to lua value: unsupported type" << value.typeName();
		lua_pushnil(state);
	}
	// -2) new table
	// -1) value

	lua_setfield(state, -2, "value");
	// -1) new table

	lua_newtable(state);
	// -2) new table
	// -1) table for metadata

	for (const auto& [k, v] : value.metaData().sValues()) {
		push_rpc_value(state, v);
		// -3) new table
		// -2) table for metadata
		// -1) i meta value

		lua_setfield(state, -2, k.c_str());
		// -2) new table
		// -1) table for metadata
	}

	for (const auto& [i, v] : value.metaData().iValues()) {
		push_rpc_value(state, v);
		// -3) new table
		// -2) table for metadata
		// -1) i meta value

		lua_seti(state, -2, i + 1/* lua arrays are indexed from 1 */);
		// -2) new table
		// -1) table for metadata
	}
	// -2) new table
	// -1) table for metadata

	lua_setfield(state, -2, "meta");
}
}

void HolyScopeApp::onRpcMessageReceived(const shv::chainpack::RpcMessage& msg)
{
	auto sg = StackGuard(m_state);
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
		// 1) registry["rpc_call_handlers"]

		lua_geti(m_state, 1, rp.requestId().toInt());
		// 1) registry["rpc_call_handlers"]
		// 2) registry["rpc_call_handlers"][req_id]

		if (!lua_isnil(m_state, 2)) {
			// 1) registry["rpc_call_handlers"]
			// 2) function

			// A handler for this req_id exists, so it's from Lua.
			if (rp.isSuccess()) {
				push_rpc_value(m_state, rp.result());
				lua_pushnil(m_state);
			} else {
				lua_pushnil(m_state);
				push_rpc_value(m_state, rp.error());
			}

			// 1) registry["rpc_call_handlers"]
			// 2) registry["rpc_call_handlers"][req_id]
			// 3) result/nil
			// 3) error/nil

			auto errors = lua_pcall(m_state, 2, 0, 0);
			// 1) registry["rpc_call_handlers"]

			if (errors) {
				handle_lua_error(m_state, "rpc_call_handler");
			}
		} else {
			// 1) registry["rpc_call_handlers"]
			// 2) nil

			lua_pop(m_state, 1);
			// 1) registry["rpc_call_handlers"]
		}

		lua_pop(m_state, 1);
		// <empty stack>

	} else if (msg.isSignal()) {
		cp::RpcSignal nt(msg);
		shvDebug() << "RPC notify received:" << nt.toPrettyString();
		lua_getfield(m_state, LUA_REGISTRYINDEX, "subscribe_callbacks");
		// 1) registry["subscribe_callbacks"]

		lua_pushnil(m_state);
		// 1) registry["subscribe_callbacks"]
		// 2) nil

		while (lua_next(m_state, 1)) {
			// 1) registry["subscribe_callbacks"]
			// 2) key
			// 3) {callback: function, path: string, type: string}

			lua_getfield(m_state, 3, "path");
			// 1) registry["subscribe_callbacks"]
			// 2) key
			// 3) {callback: function, path: string, type: string}
			// 4) .path

			lua_getfield(m_state, 3, "type");
			// 1) registry["subscribe_callbacks"]
			// 2) key
			// 3) {callback: function, path: string, type: string}
			// 4) .path
			// 5) .type

			auto path = lua_tostring(m_state, 4);
			auto type = lua_tostring(m_state, 5);
			lua_pop(m_state, 2);
			// 1) registry["subscribe_callbacks"]
			// 2) key
			// 3) {callback: function, path: string, type: string}

			if (nt.shvPath().asString().find(path) == 0 && nt.method().asString() == type) {
				// 1) registry["subscribe_callbacks"]
				// 2) key
				// 3) {callback: function, path: string, type: string}

				lua_getfield(m_state, 3, "callback");
				// 1) registry["subscribe_callbacks"]
				// 2) key
				// 3) {callback: function, path: string, type: string}
				// 4) .callback

				lua_pushstring(m_state, nt.shvPath().asString().c_str());
				// 1) registry["subscribe_callbacks"]
				// 2) key
				// 3) {callback: function, path: string, type: string}
				// 4) .callback
				// 5) arg1

				push_rpc_value(m_state, nt.params());
				// 1) registry["subscribe_callbacks"]
				// 2) key
				// 3) {callback: function, path: string, type: string}
				// 4) .callback
				// 5) arg1
				// 6) arg2

				auto errors = lua_pcall(m_state, 2, 0, 0);
				// 1) registry["subscribe_callbacks"]
				// 2) key
				// 3) {callback: function, path: string, type: string}

				if (errors) {
					handle_lua_error(m_state, "subscribe_callbacks");
				}
			}

			lua_pop(m_state, 1);
			// 1) registry["change_callbacks"]
			// 2) key
		}
		// 1) registry["subscribe_callbacks"]

		lua_pop(m_state, 1);
		// <empty stack>
	}
}

HasTester HolyScopeApp::evalLuaFile(const QFileInfo& file)
{
	auto sg = StackGuard(m_state);
	auto path = file.filePath().toStdString();
	auto errors = luaL_loadfile(m_state, path.c_str());
	if (errors) {
		// -3) parent environment
		// -2) new environment
		// -1) error
		handle_lua_error(m_state, path);
		// -2) parent environment
		// -1) new environment
		return HasTester::No;
	}
	// -3) parent environment
	// -2) new environment
	// -1) hscope.lua

	// lua_setupvalue pops from the stack, so before setting it as an upvalue for the lua file, we need to copy the
	// reference (for the upvalue)
	lua_pushvalue(m_state, -2);
	// -4) parent environment
	// -3) new environment
	// -2) hscope.lua
	// -1) new environment

	lua_setupvalue(m_state, -2, 1);
	// -3) parent environment
	// -2) new environment
	// -1) hscope.lua

	auto stack_size = lua_gettop(m_state);
	errors = lua_pcall(m_state, 0, LUA_MULTRET, 0);
	// -2) parent environment
	// -1) new environment

	if (errors) {
		// -3) parent environment
		// -2) new environment
		// -1) error
		handle_lua_error(m_state, path);
		// -2) parent environment
		// -1) new environment

		return HasTester::No;
	}

	auto nresults = lua_gettop(m_state) - stack_size + 1 /* the function gets popped */;
	if (!nresults) {
		// -2) parent environment
		// -1) new environment
		return HasTester::No;
	}

	if (nresults != 1) {
		shvError() << "Error in" << path;
		shvError() << "Lua function returned multiple results";
		shvError() << "One value of type `function` expected";
		lua_pop(m_state, nresults);
		// -2) parent environment
		// -1) new environment
		return HasTester::No;
	}

	if (!lua_isfunction(m_state, -1)) {
		// -3) parent environment
		// -2) new environment
		// -1) some invalid value

		shvError() << "Error in " << path;
		shvError() << "Lua function returned a result of type " << luaL_typename(m_state, -1);
		shvError() << "A value of type `function` expected";

		lua_pop(m_state, -1);
		// -2) parent environment
		// -1) new environment
		return HasTester::No;
	}
	// -3) parent environment
	// -2) new environment
	// -1) tester function

	lua_getfield(m_state, LUA_REGISTRYINDEX, "testers");
	// -4) parent environment
	// -3) new environment
	// -2) tester function
	// -1) registry["testers"]

	lua_insert(m_state, -2);
	// -4) parent environment
	// -3) new environment
	// -2) registry["testers"]
	// -1) tester function

	lua_setfield(m_state, -2, file.path().toStdString().c_str());
	// -3) parent environment
	// -2) new environment
	// -1) registry["testers"]

	lua_pop(m_state, 1);
	// -2) parent environment
	// -1) new environment
	return HasTester::Yes;
}

extern "C" {
static int set_status(lua_State* state)
{
	auto sg = StackGuard(state, 0);
	check_lua_args<LUA_TTABLE>(state, "set_status");
	// Stack:
	// 1) table
	auto node = static_cast<HscopeNode*>(lua_touserdata(state, lua_upvalueindex(1)));
	lua_getfield(state, 1, "severity");
	// 1) table
	// 2) table["severity"]

	if (lua_type(state, -1) != LUA_TSTRING) {
		// 1) table
		// 2) nil
		luaL_error(state, "set_status: severity must be of type string");
	}

	auto severity = std::string{lua_tostring(state, -1)};
	lua_getfield(state, 1, "message");

	// 1) table
	// 2) table["severity"]
	// 3) table["message"]
	std::string message;

	if (auto type = lua_type(state, -1); type != LUA_TNIL) {
		// 1) table
		// 2) table["severity"]
		// 3) table["message"]
		if (type == LUA_TSTRING) {
			message = lua_tostring(state, -1);
		} else {
			luaL_error(state, "set_status: message must be of type string");
		}
	}

	lua_pop(state, 3);
	// <empty stack>

	if (severity != "good" && severity != "warn" && severity != "error") {
		luaL_error(state, "Invalid severity value: %s Allowed values for severity are 'good', 'warn', or 'error'", severity.c_str());
	}

	node->setStatus(severity, message);
	return 0;
}
}

void HolyScopeApp::resolveConfTree(const QDir& dir, shv::iotqt::node::ShvNode* parent)
{
	auto sg = StackGuard(m_state);
	auto path = dir.path().toStdString();
	shvInfo() << "Entering " << path;

	// The parent environment is expected on the top of the lua stack.
	// -1) parent environment

	// Clone a new environment for this level:
	lua_newtable(m_state);
	// -2) parent environment
	// -1) new environment

	lua_pushnil(m_state);
	// -3) parent environment
	// -2) new environment
	// -1) nil
	while (lua_next(m_state, -3)) {
		// -4) parent environment
		// -3) new environment
		// -2) key
		// -1) value

		lua_setfield(m_state, -3, lua_tostring(m_state, -2));
		// -3) parent environment
		// -2) new environment
		// -1) key
	}
	// -2) parent environment
	// -1) new environment

	// Even though we copied all the fields from the parent environment, we still need to set the new environment's
	// metatable to the global environment, so that it can still access libraries.
	lua_newtable(m_state);
	// -3) parent environment
	// -2) new environment
	// -1) metatable for the new environment

	lua_pushglobaltable(m_state);
	// -4) parent environment
	// -3) new environment
	// -2) metatable for the new environment
	// -1) the global environment

	lua_setfield(m_state, -2, "__index");
	// -3) parent environment
	// -2) new environment
	// -1) metatable for the new environment

	lua_setmetatable(m_state, -2);
	// -2) parent environment
	// -1) new environment

	lua_pushlstring(m_state, path.c_str(), path.size());
	// -3) parent environment
	// -2) new environment
	// -1) path

	lua_setfield(m_state, -2, "cur_dir");
	// -2) parent environment
	// -1) new environment

	auto new_node = new HscopeNode(dir.dirName().toStdString(), parent);

	lua_pushlightuserdata(m_state, new_node);
	// -3) parent environment
	// -2) new environment
	// -1) new_node

	lua_pushcclosure(m_state, set_status, 1);
	// -3) parent environment
	// -2) new environment
	// -1) set_status

	lua_setfield(m_state, -2, "set_status");
	// -2) parent environment
	// -1) new environment

	if (dir.exists("hscope.lua")) {
		shvInfo() << "Lua file found" << dir.filePath("hscope.lua");
		auto has_tester = evalLuaFile(QFileInfo(dir.filePath("hscope.lua")));
		if (has_tester == HasTester::Yes) {
			new_node->attachTester(m_state, path);
		}
	}

	QDirIterator it(dir);
	while (it.hasNext()) {
		it.next();
		if (it.fileInfo().isDir() && it.fileName() != "." && it.fileName() != "..") {
			resolveConfTree(it.filePath(), new_node);
		}
	}

	lua_pop(m_state, 1);
	// -1) parent environment
}

void HolyScopeApp::subscribeLua(const std::string& path, const std::string& type)
{
	m_rpcConnection->callMethodSubscribe(path, type);
}

int HolyScopeApp::callShvMethod(const std::string& path, const std::string& method, const shv::chainpack::RpcValue& params)
{
	return m_rpcConnection->callShvMethod(path, method, params);
}
