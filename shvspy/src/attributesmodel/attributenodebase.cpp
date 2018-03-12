#include "attributenodebase.h"

#include <shv/coreqt/log.h>

enum ExtraRoles {ValueRole = Qt::UserRole + 1, DisplayValueRole};

AttributeNodeBase::AttributeNodeBase()
{

}

void AttributeNodeBase::setName(const QString &n)
{
	setText(n);
}

QVariant AttributeNodeBase::value() const
{
	QVariant val = data(ValueRole);
	return val;
}

void AttributeNodeBase::setValue(const QVariant &val)
{
	setData(val, ValueRole);
}

QVariant AttributeNodeBase::displayValue() const
{
	QVariant val = data(DisplayValueRole);
	if(!val.isValid())
		val = value();
	return val;
}

void AttributeNodeBase::setDisplayValue(const QVariant &val)
{
	setData(val, DisplayValueRole);
}

void AttributeNodeBase::appendNode(const QString &name, const QVariant &val)
{
	appendNode(new AttributeNodeBase(), name, val);
}

void AttributeNodeBase::appendNode(AttributeNodeBase *bnd, const QString &name, const QVariant &val)
{
	QList<QStandardItem*>lst;
	bnd->setName(name);
	bnd->setValue(val);
	lst << bnd;
	lst << new QStandardItem();
	appendRow(lst);
}

