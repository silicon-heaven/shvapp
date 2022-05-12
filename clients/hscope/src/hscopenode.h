#ifndef HSCOPENODE_H
#define HSCOPENODE_H

#include <shv/iotqt/node/shvnode.h>
#include <shv/chainpack/datachange.h>

struct lua_State;

class HscopeNode : public shv::iotqt::node::MethodsTableNode
{
	Q_OBJECT

	using Super = shv::iotqt::node::MethodsTableNode;

public:
	HscopeNode(const std::string &name, shv::iotqt::node::ShvNode *parent);

	shv::chainpack::RpcValue callMethod(const StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params, const shv::chainpack::RpcValue &user_id) override;

private:

	std::string m_status;
};
#endif /*HSCOPENODE_H*/
