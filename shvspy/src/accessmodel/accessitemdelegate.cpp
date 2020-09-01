#include "accessitemdelegate.h"
#include "accessmodel.h"

#include <QLineEdit>

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
				emit inputDataError(QString());
			}
			else
				emit inputDataError(tr("Error: column") + " " + AccessModel::columnName(index.column()) + " " + tr("is not valid chainpack.") + " " + tr("For exmaple \"cmd\""));
		}
		else if (index.column() == AccessModel::Columns::ColPath){
			if (!val.empty()){
				model->setData(index, qobject_cast<QLineEdit*>(editor)->text(), Qt::EditRole);
				emit inputDataError(QString());
			}
			else
				emit inputDataError(tr("Error: column") + " " + AccessModel::columnName(index.column()) + " " + tr("is empty."));
		}
		else if (index.column() == AccessModel::Columns::ColMethod){
			model->setData(index, qobject_cast<QLineEdit*>(editor)->text(), Qt::EditRole);
			emit inputDataError(QString());
		}
		else
			Super::setModelData(editor, model, index);
	}
	else
		Super::setModelData(editor, model, index);
}
