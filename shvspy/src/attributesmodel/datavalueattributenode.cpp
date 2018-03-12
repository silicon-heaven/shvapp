#include "datavalueattributenode.h"
#include "valueattributenode.h"

DataValueAttributeNode::DataValueAttributeNode(const std::string &att_id)
	: Super(att_id)
{
	setDisplayValue(QString());
}

bool DataValueAttributeNode::load(bool force)
{
	if(Super::load(force)) {
		/*
		appendNode(QStringLiteral("SourceTimestamp"), m_dataValue.sourceTimestamp());
		appendNode(QStringLiteral("ServerTimestamp"), m_dataValue.serverTimestamp());
		{
			AttributeNodeBase *nd = new AttributeNodeBase();
			nd->setDisplayValue(qfopcua::Client::dataValueStatusCodeToString(m_dataValue.statusCode()));
			appendNode(nd, QStringLiteral("Status"), m_dataValue.statusCode());
		}
		appendNode(new ValueAttributeNode(), QStringLiteral("Value"), m_dataValue.value());
		*/
		return true;
	}
	return false;
}

