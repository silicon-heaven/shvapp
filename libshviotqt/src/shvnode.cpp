#include "shvnode.h"

#include <shv/chainpack/rpcvalue.h>

namespace shv {
namespace iotqt {

ShvNode::ShvNode(QObject *parent)
	: QObject(parent)
{

}

ShvNode *ShvNode::childNode(const String &name) const
{
	ShvNode *nd = findChild<ShvNode*>(QString::fromStdString(name), Qt::FindDirectChildrenOnly);
	return nd;
}

void ShvNode::setParentNode(ShvNode *parent)
{
	setParent(parent);
}

ShvNode::StringList ShvNode::propertyNames() const
{
	StringList ret;
	for(ShvNode *nd : findChildren<ShvNode*>(QString(), Qt::FindDirectChildrenOnly))
		ret.push_back(nd->nodeName());
	return ret;
}

shv::chainpack::RpcValue ShvNode::shvValue() const
{
	shv::chainpack::RpcValue::List lst;
	for(const String &s : propertyNames())
		lst.push_back(s);
	return lst;
}

shv::chainpack::RpcValue ShvNode::propertyValue(const String &property_name) const
{
	ShvNode *nd = childNode(property_name);
	if(nd)
		return nd->shvValue();
	return shv::chainpack::RpcValue();
}

bool ShvNode::setPropertyValue(const ShvNode::String &property_name, const shv::chainpack::RpcValue &val)
{
	if(setPropertyValue_helper(property_name, val)) {
		emit propertyValueChanged(property_name, val);
		return true;
	}
	return false;
}

bool ShvNode::setPropertyValue_helper(const ShvNode::String &property_name, const shv::chainpack::RpcValue &val)
{
	Q_UNUSED(property_name)
	Q_UNUSED(val)
	return false;
}

}}
