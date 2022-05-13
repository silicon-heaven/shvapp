#include "lua_utils.h"

[[nodiscard]] StackGuard::StackGuard(lua_State* state, ShouldPopStack shouldPopStack)
    : m_state(state)
    , m_targetSize(shouldPopStack == ShouldPopStack::Yes ? 0 : lua_gettop(m_state))
{
}

StackGuard::~StackGuard()
{
    if (auto stack_size = lua_gettop(m_state); stack_size != m_targetSize) {
        shvError() << "Lua logic error.";
        shvError() << "Target stack size not acheived.";
        shvError() << "Target size: " << m_targetSize << " Actual size: " << stack_size;
    }
}
