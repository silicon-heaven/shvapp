#ifndef HSCOPENODE_H
#define HSCOPENODE_H

#include <shv/iotqt/node/shvnode.h>
#include <shv/chainpack/datachange.h>

struct lua_State;

enum class HasTester {
	Yes,
	No
};

class HscopeNode : public shv::iotqt::node::ShvNode
{
	Q_OBJECT

	using Super = shv::iotqt::node::ShvNode;

public:
	HscopeNode(const std::string &name, shv::iotqt::node::ShvNode *parent);

	size_t methodCount(const StringViewList& shv_path) override;
	const shv::chainpack::MetaMethod* metaMethod(const StringViewList& shv_path, size_t ix) override;
	shv::chainpack::RpcValue callMethod(const StringViewList& shv_path, const std::string& method, const shv::chainpack::RpcValue& params, const shv::chainpack::RpcValue& user_id) override;

	void setStatus(const std::string& severity, const std::string& message);
	void attachTester(lua_State* state, const std::string& tester_location);

private:

	lua_State* m_state;
	shv::chainpack::RpcValue m_status;
	std::string m_testerLocation;
};
#endif /*HSCOPENODE_H*/
