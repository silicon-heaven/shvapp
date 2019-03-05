#ifndef BRCLABNODE_H
#define BRCLABNODE_H

#include "brclabusersnode.h"
#include <shv/iotqt/node/shvnode.h>

#include <QObject>

class BrclabNode : public shv::iotqt::node::ShvNode
{
	using Super = shv::iotqt::node::ShvNode;
public:
	BrclabNode(const std::string &node_id, shv::iotqt::node::ShvNode *parent);

private:
	BrclabUsersNode *m_BrclabUsersNode = nullptr;
};

#endif // BRCLABNODE_H
