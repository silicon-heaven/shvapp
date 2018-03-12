#include "attributenode.h"
#include "attributesmodel.h"

AttributeNode::AttributeNode(const std::string &att_id)
	: Super(), m_attributeId(att_id)
{
	setText(QString::fromStdString(m_attributeId));
}

AttributeNode::~AttributeNode()
{
}

QVariant AttributeNode::value() const
{
	QVariant ret;// = m_dataValue.value();
	return ret;
}

bool AttributeNode::load(bool force)
{
	if(!m_loaded || force) {
		removeRows(0, rowCount());
		m_loaded = true;
		//m_dataValue = attributesModel()->attribute(m_attributeId);
		return true;
	}
	return false;
}

AttributesModel *AttributeNode::attributesModel()
{
	return qobject_cast<AttributesModel*>(model());
}
/*
QString AttributeNode::attributeName(const qfopcua::AttributeId::Enum &att_id)
{
	return qfopcua::AttributeId::toString(att_id);
}
*/



