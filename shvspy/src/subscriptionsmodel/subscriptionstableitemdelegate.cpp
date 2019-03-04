#include "subscriptionstableitemdelegate.h"
#include "subscriptionsmodel.h"

#include <QComboBox>

SubscriptionsTableItemDelegate::SubscriptionsTableItemDelegate(QObject *parent) :
	Super(parent)
{

}

QWidget *SubscriptionsTableItemDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	int col = index.column();
	if ((col == SubscriptionsModel::Columns::ColPermanent) || (col == SubscriptionsModel::Columns::ColSubscribeAfterConnect) ||
		(col == SubscriptionsModel::Columns::ColEnabled)){
		QComboBox *editor = new QComboBox(parent);
		editor->setInsertPolicy(QComboBox::NoInsert);
		editor->addItem(tr("yes"), true);
		editor->addItem(tr("no"), false);
		return editor;
	}
	else {
		return Super::createEditor(parent, option, index);
	}
}

void SubscriptionsTableItemDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
	int col = index.column();
	if ((col == SubscriptionsModel::Columns::ColPermanent) || (col == SubscriptionsModel::Columns::ColSubscribeAfterConnect) ||
		(col == SubscriptionsModel::Columns::ColEnabled)){
		QComboBox *cb = static_cast<QComboBox*>(editor);
		for(int i = 0; i < cb->count(); ++i){
			if (cb->itemData(i, Qt::UserRole).toBool() == index.data(Qt::UserRole).toBool()){
				cb->setCurrentIndex(i);
			}
		}
	}
	else {
		Super::setEditorData(editor, index);
	}
}

void SubscriptionsTableItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
	int col = index.column();
	if ((col == SubscriptionsModel::Columns::ColPermanent) || (col == SubscriptionsModel::Columns::ColSubscribeAfterConnect) ||
		(col == SubscriptionsModel::Columns::ColEnabled)){
		QComboBox *cb = static_cast<QComboBox*>(editor);
		model->setData(index, cb->currentData(Qt::UserRole), Qt::UserRole);
	}
	else{
		Super::setModelData(editor, model, index);
	}
}

void SubscriptionsTableItemDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	Q_UNUSED(index);
	editor->setGeometry(option.rect);
}
