#pragma once

#include <QDialog>

namespace shv { namespace chainpack { class RpcValue; }}

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
	void setValidateCpon(bool b);
private slots:
	void on_btCompactCpon_clicked();
	void on_btFormatCpon_clicked();
private:
	void validateContentDeferred();
	shv::chainpack::RpcValue validateContent();
private:
	Ui::TextEditDialog *ui;
	bool m_isValidateContent = false;
	QTimer *m_validateTimer = nullptr;
};
