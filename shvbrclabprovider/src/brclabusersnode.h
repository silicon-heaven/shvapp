#ifndef BRCLABUSERSNODE_H
#define BRCLABUSERSNODE_H

#include <shv/iotqt/node/shvnode.h>

#include <QObject>

class BrclabUsersNode : public shv::iotqt::node::RpcValueMapNode
{
	using Super = shv::iotqt::node::RpcValueMapNode;
public:
	BrclabUsersNode(const std::string &node_id, const std::string &fn, shv::iotqt::node::ShvNode *parent);

	size_t methodCount(const StringViewList &shv_path) override;
	const shv::chainpack::MetaMethod* metaMethod(const StringViewList &shv_path, size_t ix) override;
	shv::chainpack::RpcValue callMethod(const StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params, const shv::chainpack::RpcValue &user_id) override;
	void loadValues() override;
	void saveValues(void) override;

	bool addUser(const shv::chainpack::RpcValue &params);
	bool delUser(const shv::chainpack::RpcValue &params);
	bool changePassword(const shv::chainpack::RpcValue &params);
	shv::chainpack::RpcValue getUserGrants(const shv::chainpack::RpcValue &params);

private:
	void setUsersConfig(const shv::chainpack::RpcValue &data);
	const shv::chainpack::RpcValue &usersConfig();

	std::string m_usersConfigFileName;
};

#endif // BRCLABUSERSNODE_H
