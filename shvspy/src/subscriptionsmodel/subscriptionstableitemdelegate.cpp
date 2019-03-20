#include "subscriptionstableitemdelegate.h"
#include "subscriptionsmodel.h"

#include <QCheckBox>
#include <QApplication>

SubscriptionsTableItemDelegate::SubscriptionsTableItemDelegate(QObject *parent) :
	Super(parent)
{

}

QWidget *SubscriptionsTableItemDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	return Super::createEditor(parent, option, index);
}

void SubscriptionsTableItemDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
	Super::setEditorData(editor, index);
}

void SubscriptionsTableItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
	Super::setModelData(editor, model, index);
}

void SubscriptionsTableItemDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	Q_UNUSED(index);
	editor->setGeometry(option.rect);
}
