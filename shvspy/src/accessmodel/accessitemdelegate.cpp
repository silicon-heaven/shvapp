#include "accessitemdelegate.h"
#include "accessmodel.h"

#include <QLineEdit>
#include <shv/core/log.h>


AccessItemDelegate::AccessItemDelegate(QObject *parent)
	: QStyledItemDelegate(parent)
{

}

QWidget *AccessItemDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	QLineEdit *editor = new QLineEdit(parent);
	return editor;
}

void AccessItemDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
	Super::setEditorData(editor, index);
}

void AccessItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
	if (index.isValid()){
		QLineEdit *e = qobject_cast<QLineEdit*>(editor);
		if (e){
			std::string val = e->text().trimmed().toStdString();

			if (index.column() == AccessModel::Columns::ColGrant){
				std::string err;
				shv::chainpack::RpcValue rv = shv::chainpack::RpcValue::fromCpon(val, &err);

				if (err.empty()){
					model->setData(index, qobject_cast<QLineEdit*>(editor)->text(), Qt::EditRole);
					emit inputDataError(QString());
				}
				else
					emit inputDataError(tr("Invalid data for column") + " " + AccessModel::columnName(index.column()) + " " + tr("It must be a valid chainpack. For exmaple \"cmd\""));
			}
			else if (index.column() == AccessModel::Columns::ColPath){
				if (!val.empty()){
					model->setData(index, qobject_cast<QLineEdit*>(editor)->text(), Qt::EditRole);
					emit inputDataError(QString());
				}
				else
					emit inputDataError(tr("Invalid data for column") + " " + AccessModel::columnName(index.column()));
			}
		}
	}
	else {
		Super::setModelData(editor, model, index);
	}
}
