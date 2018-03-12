#include "qualifiednameattributenode.h"

QualifiedNameAttributeNode::QualifiedNameAttributeNode(const std::string &att_id)
	: Super(att_id)
{

}

bool QualifiedNameAttributeNode::load(bool force)
{
	if(Super::load(force)) {
		//qfopcua::QualifiedName qn = m_dataValue.value().value<qfopcua::QualifiedName>();
		//setDisplayValue(qn.toString());
		return true;
	}
	return false;
}

