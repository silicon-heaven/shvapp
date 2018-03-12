#include "nodeclassattributenode.h"

NodeClassAttributeNode::NodeClassAttributeNode(const std::string &att_id)
	: Super(att_id)
{

}

bool NodeClassAttributeNode::load(bool force)
{
	if(Super::load(force)) {
		//setDisplayValue(qfopcua::NodeClass::toString(value().toInt()));
		return true;
	}
	return false;
}


