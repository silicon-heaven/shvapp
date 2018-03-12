#ifndef THEAPP_H
#define THEAPP_H

#include <shv/core/utils/crypt.h>

#include <QApplication>

class ServerTreeModel;
class AttributesModel;
class SubscriptionsModel;
class AppCliOptions;

class TheApp : public QApplication
{
	Q_OBJECT
private:
	typedef QApplication Super;
public:
	TheApp(int &argc, char **argv, AppCliOptions* cli_opts);
	~TheApp() Q_DECL_OVERRIDE;

	static TheApp* instance() {return qobject_cast<TheApp*>(Super::instance());}
public:
	ServerTreeModel* serverTreeModel() {return m_serverTreeModel;}
	AttributesModel* attributesModel() {return m_attributesModel;}
	SubscriptionsModel* subscriptionsModel() {return m_subscriptionsModel;}
	const shv::core::utils::Crypt& crypt() {return m_crypt;}
private:
	ServerTreeModel *m_serverTreeModel;
	AttributesModel *m_attributesModel;
	SubscriptionsModel *m_subscriptionsModel;
	AppCliOptions* m_cliOptions;
	shv::core::utils::Crypt m_crypt;
};

#endif // THEAPP_H
