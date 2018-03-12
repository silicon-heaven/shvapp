#ifndef NODEIDATTRIBUTENODE_H
#define NODEIDATTRIBUTENODE_H

#include "attributenode.h"

class NodeIdAttributeNode : public AttributeNode
{
private:
	typedef AttributeNode Super;
public:
	NodeIdAttributeNode(const std::string &att_id);
private:
	bool load(bool force) Q_DECL_OVERRIDE;
private:
	std::string m_nodeId;
};

#endif // NODEIDATTRIBUTENODE_H
