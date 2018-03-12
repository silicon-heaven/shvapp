#ifndef ATTRIBUTENODEBASE_H
#define ATTRIBUTENODEBASE_H

#include <QStandardItem>

class AttributeNodeBase : public QStandardItem
{
private:
	typedef QStandardItem Super;
public:
	AttributeNodeBase();

	virtual QString name() const {return text();}
	virtual void setName(const QString &n);
	virtual QVariant value() const;
	virtual void setValue(const QVariant &val);
	virtual QVariant displayValue() const;
	virtual void setDisplayValue(const QVariant &val);

	virtual QVariant toEditorValue(const QVariant &val) const {return val;}
	virtual QVariant fromEditorValue(const QVariant &val) const {return val;}
protected:
	void appendNode(const QString &name, const QVariant &val);
	void appendNode(AttributeNodeBase *bnd, const QString &name, const QVariant &val);
};

#endif // ATTRIBUTENODEBASE_H
