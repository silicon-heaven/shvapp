#ifndef PATHSTABLEITEMDELEGATE_H
#define PATHSTABLEITEMDELEGATE_H

#include <QObject>
#include <QStyledItemDelegate>

class PathsTableItemDelegate : public QStyledItemDelegate
{
	Q_OBJECT
private:
	using Super = QStyledItemDelegate;
public:
	PathsTableItemDelegate(QObject *parent = nullptr);

	QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const Q_DECL_OVERRIDE;
	void setEditorData(QWidget *editor, const QModelIndex &index) const Q_DECL_OVERRIDE;
	void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const Q_DECL_OVERRIDE;
	void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const Q_DECL_OVERRIDE;
};

#endif // PATHSTABLEITEMDELEGATE_H
