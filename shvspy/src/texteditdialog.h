#pragma once

#include <QDialog>

namespace Ui {
class TextEditDialog;
}

class TextEditDialog : public QDialog
{
	Q_OBJECT

	using Super = QDialog;
public:
	explicit TextEditDialog(QWidget *parent = nullptr);
	~TextEditDialog() override;

	void setText(const QString &s);
	QString text() const;

	void setReadOnly(bool ro);
private:
	Ui::TextEditDialog *ui;
};
