#ifndef BRCLABNODE_H
#define BRCLABNODE_H

#include "brclabusersnode.h"
#include <shv/iotqt/node/shvnode.h>

#include <QObject>

class BrclabNode : public shv::iotqt::node::ShvNode
{
	using Super = shv::iotqt::node::ShvNode;
public:
	BrclabNode(shv::iotqt::node::ShvNode *parent);

	size_t methodCount(const StringViewList &shv_path) override;
	const shv::chainpack::MetaMethod* metaMethod(const StringViewList &shv_path, size_t ix) override;
	shv::chainpack::RpcValue callMethod(const StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params) override;

private:
	BrclabUsersNode *m_BrclabUsersNode = nullptr;
};

#endif // BRCLABNODE_H
