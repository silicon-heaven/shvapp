#ifndef THEAPP_H
#define THEAPP_H

#include <shv/core/utils/crypt.h>

#include <QApplication>

class ServerTreeModel;
class AttributesModel;
class RpcNotificationsModel;
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
	AppCliOptions* cliOptions() {return m_cliOptions;}

	ServerTreeModel* serverTreeModel() {return m_serverTreeModel;}
	AttributesModel* attributesModel() {return m_attributesModel;}
	RpcNotificationsModel* rpcNotificationsModel() {return m_rpcNotificationsModel;}
	const shv::core::utils::Crypt& crypt() {return m_crypt;}
private:
	ServerTreeModel *m_serverTreeModel = nullptr;
	AttributesModel *m_attributesModel = nullptr;
	RpcNotificationsModel *m_rpcNotificationsModel = nullptr;
	AppCliOptions* m_cliOptions = nullptr;
	shv::core::utils::Crypt m_crypt;
};

#endif // THEAPP_H
