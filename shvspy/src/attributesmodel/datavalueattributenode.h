#ifndef DATAVALUEATTRIBUTENODE_H
#define DATAVALUEATTRIBUTENODE_H

#include "attributenode.h"

class DataValueAttributeNode : public AttributeNode
{
private:
	typedef AttributeNode Super;
public:
	DataValueAttributeNode(const std::string &att_id);
private:
	bool load(bool force) Q_DECL_OVERRIDE;
};

#endif // DATAVALUEATTRIBUTENODE_H
