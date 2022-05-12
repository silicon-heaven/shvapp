#ifndef HSCOPENODE_H
#define HSCOPENODE_H

#include <shv/iotqt/node/shvnode.h>
#include <shv/chainpack/datachange.h>

struct lua_State;

enum class HasTester {
	Yes,
	No
};

class HscopeNode : public shv::iotqt::node::MethodsTableNode
{
	Q_OBJECT

	using Super = shv::iotqt::node::MethodsTableNode;

public:
	HscopeNode(const std::string &name, shv::iotqt::node::ShvNode *parent);
	HscopeNode(const std::string &name, lua_State* state, const std::string& testerLocation, shv::iotqt::node::ShvNode *parent);

	shv::chainpack::RpcValue callMethod(const StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params, const shv::chainpack::RpcValue &user_id) override;

	void setStatus(const std::string& status);

private:
	lua_State* m_state;
	std::string m_status;
	std::string m_testerLocation;
};
#endif /*HSCOPENODE_H*/
