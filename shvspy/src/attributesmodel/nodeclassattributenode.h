#ifndef NODECLASSATTRIBUTENODE_H
#define NODECLASSATTRIBUTENODE_H

#include "attributenode.h"

class NodeClassAttributeNode : public AttributeNode
{
private:
	typedef AttributeNode Super;
public:
	NodeClassAttributeNode(const std::string &att_id);
protected:
	bool load(bool force) Q_DECL_OVERRIDE;
};

#endif // NODECLASSATTRIBUTENODE_H
