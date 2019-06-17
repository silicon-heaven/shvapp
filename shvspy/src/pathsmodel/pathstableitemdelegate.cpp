#include "pathstableitemdelegate.h"

#include "pathsmodel.h"

#include <QCheckBox>
#include <QSpinBox>
#include <shv/chainpack/rpcvalue.h>

PathsTableItemDelegate::PathsTableItemDelegate(QObject *parent):
	Super(parent)
{

}

QWidget *PathsTableItemDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	if (index.column() == PathsModel::Columns::ColWeight){
		QSpinBox *editor = new QSpinBox(parent);
		editor->setFrame(false);
		editor->setRange(-1, 100000);
		editor->setSingleStep(1);
		editor->setSpecialValueText("Not used");
		return editor;
	}

	return Super::createEditor(parent, option, index);
}

void PathsTableItemDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
	if (qobject_cast<QSpinBox*>(editor)) {
		qobject_cast<QSpinBox*>(editor)->setValue(index.data(Qt::EditRole).toInt());
	}

	Super::setEditorData(editor, index);
}

void PathsTableItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
	if (qobject_cast<QSpinBox*>(editor)) {
		model->setData(index, qobject_cast<QSpinBox*>(editor)->value(), Qt::EditRole);
	}

	Super::setModelData(editor, model, index);
}

void PathsTableItemDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	Q_UNUSED(index);
	editor->setGeometry(option.rect);
}
