#include "nodeidattributenode.h"

NodeIdAttributeNode::NodeIdAttributeNode(const std::string &att_id)
	: Super(att_id)
{

}

bool NodeIdAttributeNode::load(bool force)
{
	if(Super::load(force)) {
		//m_nodeId = m_dataValue.value().value<qfopcua::NodeId>();
		//appendNode(QStringLiteral("NameSpaceIndex"), m_nodeId.nameSpaceIndex());
		//appendNode(QStringLiteral("IdentifierType"), qfopcua::NodeIdEncoding::toString(m_nodeId.identifierType()));
		//appendNode(QStringLiteral("Identifier"), m_nodeId.identifier());
		return true;
	}
	return false;
}

