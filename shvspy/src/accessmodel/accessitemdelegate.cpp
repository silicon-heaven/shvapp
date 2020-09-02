#include "accessitemdelegate.h"
#include "accessmodel.h"

#include <QLineEdit>
#include <QMessageBox>

AccessItemDelegate::AccessItemDelegate(QObject *parent)
	: QStyledItemDelegate(parent)
{

}

QWidget *AccessItemDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	Q_UNUSED(option);
	Q_UNUSED(index);

	QLineEdit *editor = new QLineEdit(parent);
	return editor;
}

void AccessItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
	QLineEdit *e = qobject_cast<QLineEdit*>(editor);
	if (index.isValid() && e){
		std::string val = e->text().trimmed().toStdString();

		if (index.column() == AccessModel::Columns::ColGrant){
			std::string err;
			shv::chainpack::RpcValue rv = shv::chainpack::RpcValue::fromCpon(val, &err);

			if (err.empty()){
				model->setData(index, qobject_cast<QLineEdit*>(editor)->text(), Qt::EditRole);
			}
			else{
				QString warn =tr("In column") + " " + AccessModel::columnName(index.column()) + " " + tr("is not valid chainpack.") + " " + tr("For exmaple \"cmd\"");
				QMessageBox::warning(editor, tr("Invalid data"), warn);
			}
		}
		else if (index.column() == AccessModel::Columns::ColPath){
			if (!val.empty()){
				model->setData(index, qobject_cast<QLineEdit*>(editor)->text(), Qt::EditRole);
			}
			else{
				QString warn =tr("Column") + " " + AccessModel::columnName(index.column()) + " " + tr("is empty.");
				QMessageBox::warning(editor, tr("Invalid data"), warn);
			}
		}
		else if (index.column() == AccessModel::Columns::ColMethod){
			model->setData(index, qobject_cast<QLineEdit*>(editor)->text(), Qt::EditRole);
		}
		else
			Super::setModelData(editor, model, index);
	}
	else
		Super::setModelData(editor, model, index);
}
