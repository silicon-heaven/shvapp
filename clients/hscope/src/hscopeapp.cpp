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

extern "C" {
static int subscribe_change(lua_State* state)
{
	if (auto nargs = lua_gettop(state); nargs != 2) {
		luaL_error(state, "register_callback expects 2 arguments (got %d)", nargs);
	}

	if (lua_type(state, 1) != LUA_TSTRING) {
		luaL_error(state, "arg 1 must be of type `string` (got %s)", luaL_typename(state, 1));
	}

	if (!lua_isfunction(state, 2)) {
		luaL_error(state, "arg 2 must be of type `function` (got %s)", luaL_typename(state, 2));
	}

	// Stack:
	// 1) path
	// 2) callback

	auto hscope = static_cast<HolyScopeApp*>(lua_touserdata(state, lua_upvalueindex(1)));
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

	lua_pushlightuserdata(m_state, this);
	// 1) our library table
	// 2) `this`

	lua_pushcclosure(m_state, subscribe_change, 1);
	// 1) our library table
	// 2) `this`
	// 3) subscribe_change

	lua_setfield(m_state, 1, "subscribe_change");
	// 1) our library table

	lua_setglobal(m_state, "shv");
	// <empty stack>

	lua_newtable(m_state);
	// 1) new table

	lua_setfield(m_state, LUA_REGISTRYINDEX, "callbacks");
	// <empty stack>

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

	for (const auto& path : m_luaSubscriptions) {
		m_rpcConnection->callMethodSubscribe(path, "chng");
	}
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
	m_luaSubscriptions.emplace_back(path);

}
