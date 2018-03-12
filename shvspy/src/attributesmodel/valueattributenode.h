#ifndef VALUEATTRIBUTENODE_H
#define VALUEATTRIBUTENODE_H

#include "attributenodebase.h"

class ValueAttributeNode : public AttributeNodeBase
{
private:
	typedef AttributeNodeBase Super;
public:
	ValueAttributeNode();

	void setValue(const QVariant &val) Q_DECL_OVERRIDE;

	QVariant toEditorValue(const QVariant &val) const Q_DECL_OVERRIDE;
	QVariant fromEditorValue(const QVariant &val) const Q_DECL_OVERRIDE;
};

#endif // VALUEATTRIBUTENODE_H
