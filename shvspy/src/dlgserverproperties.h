#ifndef DLGSERVERPROPERTIES_H
#define DLGSERVERPROPERTIES_H

#include <QDialog>
#include <QVariantMap>

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

public slots:
	void on_subscriptionDialogButton_clicked();

private:
	Ui::DlgServerProperties *ui;
	QVariantList m_subscriptions;
};

#endif // DLGSERVERPROPERTIES_H
