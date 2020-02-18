#include "accesstableitemdelegate.h"

#include "accessmodel.h"

#include <QSpinBox>
#include <shv/chainpack/rpcvalue.h>

AccessTableItemDelegate::AccessTableItemDelegate(QObject *parent):
	Super(parent)
{

}

QWidget *AccessTableItemDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	return Super::createEditor(parent, option, index);
}

void AccessTableItemDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
	if (qobject_cast<QSpinBox*>(editor)) {
		qobject_cast<QSpinBox*>(editor)->setValue(index.data(Qt::EditRole).toInt());
	}

	Super::setEditorData(editor, index);
}

void AccessTableItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
	if (qobject_cast<QSpinBox*>(editor)) {
		model->setData(index, qobject_cast<QSpinBox*>(editor)->value(), Qt::EditRole);
	}

	Super::setModelData(editor, model, index);
}

void AccessTableItemDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	Q_UNUSED(index);
	editor->setGeometry(option.rect);
}
