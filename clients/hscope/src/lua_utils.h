#include <lua.hpp>
#include <shv/coreqt/log.h>

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

#define DUMP_STACK(state) { \
	int top = lua_gettop(state); \
	shvInfo() << "Stack:"; \
	shvInfo() << "------"; \
	for (int i = 1; i <= top; i++) { \
		shvInfo() << "#" << i << "\ttype: " << luaL_typename(state,i) << "\tvalue: "; \
		switch (lua_type(state, i)) { \
			case LUA_TNUMBER: \
				shvInfo() << lua_tonumber(state, i); \
				break; \
			case LUA_TSTRING: \
				shvInfo() << lua_tostring(state, i); \
				break; \
			case LUA_TBOOLEAN: \
				shvInfo() << (lua_toboolean(state, i) ? "true" : "false"); \
				break; \
			case LUA_TNIL: \
				shvInfo() << "nil"; \
				break; \
			default: \
				shvInfo() << lua_topointer(state,i);; \
				break; \
		} \
	} \
	shvInfo() << "------"; \
}
