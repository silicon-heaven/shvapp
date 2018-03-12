#include "accesslevelattributenode.h"

AccessLevelAttributeNode::AccessLevelAttributeNode(const std::string &att_id)
	: Super(att_id)
{

}

bool AccessLevelAttributeNode::load(bool force)
{
	if(Super::load(force)) {
		//setDisplayValue(qfopcua::AccessLevel::toString(value().toInt()));
		return true;
	}
	return false;
}


