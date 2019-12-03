#include "theapp.h"
#include "servertreemodel/servertreemodel.h"
#include "attributesmodel/attributesmodel.h"
#include "log/rpcnotificationsmodel.h"
#include "log/errorlogmodel.h"

#include <shv/coreqt/log.h>

#include <QSettings>

TheApp::TheApp(int &argc, char **argv, AppCliOptions *cli_opts)
	: Super(argc, argv)
	, m_cliOptions(cli_opts)
	, m_crypt(shv::core::utils::Crypt::createGenerator(17456, 3148, 2147483647))
{
#ifdef Q_OS_WIN
	// set default style to fusion to overcome ugly look on some Windows installations
	for(auto key : QStyleFactory::keys()) {
		if(key == QLatin1String("Fusion")) {
			setStyle(QStyleFactory::create(key));
			break;
		}
	}
#endif
	m_serverTreeModel = new ServerTreeModel(this);
	m_attributesModel = new AttributesModel(this);
	m_rpcNotificationsModel = new RpcNotificationsModel(this);
	m_errorLogModel = new ErrorLogModel(this);
}

TheApp::~TheApp()
{
	//shvWarning() << __FUNCTION__;
}

void TheApp::addLastUsedParam(const QString &shv_path, const QString &method, const QString &cpon)
{
	auto &vector = m_lastUsedParams[shv_path][method];
	if (vector.count() == 0 || vector.last().param != cpon) {
		while (vector.size() >= 5) {
			vector.removeAt(0);
		}
		vector << CallParam{ QDateTime::currentDateTime(), cpon };
	}
	else {
		vector.last().time = QDateTime::currentDateTime();
	}
}

const TheApp::ParamMap &TheApp::lastUsedParams() const
{
	return m_lastUsedParams;
}

void TheApp::loadSettings(QSettings &settings)
{
	m_serverTreeModel->loadSettings(settings);
	settings.beginGroup("params");
	QStringList paths = settings.childGroups();
	for (QString path : paths) {
		settings.beginGroup(path);
		QStringList methods = settings.childGroups();
		for (const QString &method : methods) {
			settings.beginGroup(method);
			QStringList dates = settings.childKeys();
			for (const QString &date : dates) {
				m_lastUsedParams[path.replace("%", "/")][method] << CallParam { QDateTime::fromString(date), settings.value(date).toString() };
			}
			settings.endGroup();
		}
		settings.endGroup();
	}
	settings.endGroup();
}

void TheApp::saveSettings(QSettings &settings)
{
	m_serverTreeModel->saveSettings(settings);
	settings.beginGroup("params");
	QStringList paths = m_lastUsedParams.keys();
	for (QString path : paths) {
		auto &path_params = m_lastUsedParams[path];
		settings.beginGroup(path.replace("/", "%"));
		QStringList methods = path_params.keys();
		for (const QString &method : methods) {
			auto &method_params = path_params[method];
			settings.beginGroup(method);
			for (int i = 0; i < method_params.count(); ++i) {
				settings.setValue(method_params[i].time.toString(), method_params[i].param);
			}
			settings.endGroup();
		}
		settings.endGroup();
	}
	settings.endGroup();
}
