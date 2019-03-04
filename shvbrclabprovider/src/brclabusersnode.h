#ifndef BRCLABUSERSNODE_H
#define BRCLABUSERSNODE_H

#include "brclabusers.h"

#include <shv/iotqt/node/shvnode.h>

#include <QObject>

class BrclabUsersNode : public shv::iotqt::node::RpcValueMapNode
{
	using Super = shv::iotqt::node::RpcValueMapNode;
public:
	BrclabUsersNode(const std::string &config_name, shv::iotqt::node::ShvNode *parent);

	size_t methodCount(const StringViewList &shv_path) override;
	const shv::chainpack::MetaMethod* metaMethod(const StringViewList &shv_path, size_t ix) override;
	shv::chainpack::RpcValue callMethod(const StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params) override;
	shv::chainpack::RpcValue loadValues() override;
	bool saveValues(const shv::chainpack::RpcValue &vals) override;

private:
	BrclabUsers m_brclabUsers;
};

#endif // BRCLABUSERSNODE_H
