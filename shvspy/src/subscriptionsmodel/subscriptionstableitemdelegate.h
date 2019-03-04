#ifndef SUBSCRIPTIONSTABLEITEMDELEGATE_H
#define SUBSCRIPTIONSTABLEITEMDELEGATE_H

#include <QObject>
#include <QStyledItemDelegate>

class SubscriptionsTableItemDelegate :  public QStyledItemDelegate
{
	Q_OBJECT
	using Super = QStyledItemDelegate;
public:
	explicit SubscriptionsTableItemDelegate(QObject *parent = nullptr);
	QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const Q_DECL_OVERRIDE;
	void setEditorData(QWidget *editor, const QModelIndex &index) const Q_DECL_OVERRIDE;
	void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const Q_DECL_OVERRIDE;
	void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const Q_DECL_OVERRIDE;
};

#endif // SUBSCRIPTIONSTABLEITEMDELEGATE_H
