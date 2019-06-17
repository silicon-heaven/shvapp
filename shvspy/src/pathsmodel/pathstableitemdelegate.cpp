#include "pathstableitemdelegate.h"

#include "pathsmodel.h"

#include <QCheckBox>
#include <QComboBox>
#include <QSpinBox>
#include <shv/chainpack/rpcvalue.h>
#include <shv/core/log.h>

PathsTableItemDelegate::PathsTableItemDelegate(QObject *parent):
	Super(parent)
{

}

QWidget *PathsTableItemDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	shvInfo() << "create editor";
	if (index.column() == PathsModel::Columns::ColWeight){
		QSpinBox *editor = new QSpinBox(parent);
		editor->setFrame(false);
		editor->setRange(-1, 100000);
		editor->setSingleStep(1);
		editor->setSpecialValueText("Not used");
		return editor;
	}
	/*
	else if (index.column() == PathsModel::Columns::ColForwardGrant){
		QCheckBox *editor = new QCheckBox(parent);
		editor->setTristate();
		return editor;
	}*/

	return Super::createEditor(parent, option, index);
}

void PathsTableItemDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
	Super::setEditorData(editor, index);
}

void PathsTableItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
	Super::setModelData(editor, model, index);
}

void PathsTableItemDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	Q_UNUSED(index);
	editor->setGeometry(option.rect);
}
