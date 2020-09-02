#ifndef ACCESSITEMDELEGATE_H
#define ACCESSITEMDELEGATE_H

#include <QStyledItemDelegate>

class AccessItemDelegate : public QStyledItemDelegate
{
	Q_OBJECT
private:
	using Super = QStyledItemDelegate;
public:
	AccessItemDelegate(QObject *parent = nullptr);

	QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const Q_DECL_OVERRIDE;
	void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const Q_DECL_OVERRIDE;
};

#endif // ACCESSITEMDELEGATE_H
