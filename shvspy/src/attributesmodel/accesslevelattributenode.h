#ifndef ACCESSLEVELATTRIBUTENODE_H
#define ACCESSLEVELATTRIBUTENODE_H

#include "attributenode.h"

class AccessLevelAttributeNode : public AttributeNode
{
private:
	typedef AttributeNode Super;
public:
	AccessLevelAttributeNode(const std::string &att_id);
protected:
	bool load(bool force) Q_DECL_OVERRIDE;
};

#endif // ACCESSLEVELATTRIBUTENODE_H
