#ifndef DLGSERVERPROPERTIES_H
#define DLGSERVERPROPERTIES_H

#include <QDialog>

namespace Ui {
class DlgServerProperties;
}

class DlgServerProperties : public QDialog
{
	Q_OBJECT
private:
	typedef QDialog Super;
public:
	explicit DlgServerProperties(QWidget *parent = 0);
	~DlgServerProperties();

	QVariantMap serverProperties() const;
	void setServerProperties(const QVariantMap &props);

	void done(int res) Q_DECL_OVERRIDE;
private:
	Ui::DlgServerProperties *ui;
};

#endif // DLGSERVERPROPERTIES_H
