#include "valueattributenode.h"

#include <shv/coreqt/log.h>

#include <QMetaType>

ValueAttributeNode::ValueAttributeNode()
{

}

void ValueAttributeNode::setValue(const QVariant &val)
{
	Super::setValue(val);
	Super::setDisplayValue(toEditorValue(val));
	removeRows(0, rowCount());
	const QVariantList lst = val.toList();
	if(!lst.isEmpty()) {
		int i = 0;
		for(const QVariant &v : lst) {
			appendNode(new ValueAttributeNode(), QString("[%1]").arg(i++), v);
		}
	}
}

QVariant ValueAttributeNode::toEditorValue(const QVariant &val) const
{
	QVariant ret = val;
	/*
	switch(val.userType()) {
	case qMetaTypeId<qfopcua::OpcUa_SByte>():
	case qMetaTypeId<qfopcua::OpcUa_Byte>():
		ret = val.toInt();
		break;
	default:
		break;
	}
	*/
	return ret;
}

QVariant ValueAttributeNode::fromEditorValue(const QVariant &val) const
{
	QVariant ret = val;
	QVariant orig_val = value();
	//shvInfo() << "val:" << val << "orig val:" << orig_val << orig_val.userType() << QMetaType::UChar << qMetaTypeId<qfopcua::OpcUa_Byte>();
	/*
	switch(orig_val.userType()) {
	//case QMetaType::Char:
	case qMetaTypeId<qfopcua::OpcUa_SByte>():
		ret = QVariant::fromValue((qfopcua::OpcUa_SByte)val.toInt());
		break;
	//case QMetaType::UChar:
	case qMetaTypeId<qfopcua::OpcUa_Byte>():
		ret = QVariant::fromValue((qfopcua::OpcUa_Byte)val.toInt());
		break;
	default:
		break;
	}
	*/
	return ret;
}
