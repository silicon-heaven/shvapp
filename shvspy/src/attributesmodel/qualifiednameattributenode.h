#ifndef QUALIFIEDNAMEATTRIBUTENODE_H
#define QUALIFIEDNAMEATTRIBUTENODE_H

#include "attributenode.h"

class QualifiedNameAttributeNode : public AttributeNode
{
private:
	typedef AttributeNode Super;
public:
	QualifiedNameAttributeNode(const std::string &att_id);
private:
	bool load(bool force) Q_DECL_OVERRIDE;
};

#endif // QUALIFIEDNAMEATTRIBUTENODE_H
