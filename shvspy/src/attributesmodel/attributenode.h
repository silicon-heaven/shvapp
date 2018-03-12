#ifndef ATTRIBUTENODE_H
#define ATTRIBUTENODE_H

#include "attributenodebase.h"

class AttributesModel;

class AttributeNode : public AttributeNodeBase
{
	friend class AttributesModel;
private:
	typedef AttributeNodeBase Super;
public:
	AttributeNode(const std::string &att_id);
	~AttributeNode() Q_DECL_OVERRIDE;

	virtual bool load(bool force);
	QVariant value() const Q_DECL_OVERRIDE;
	const std::string& attributeId() const {return m_attributeId;}
protected:
	AttributesModel* attributesModel();
	//static QString attributeName(const qfopcua::AttributeId::Enum &att_id);
protected:
	std::string m_attributeId;
	//qfopcua::DataValue m_dataValue;
	bool m_loaded = false;
};

#endif // ATTRIBUTENODE_H
