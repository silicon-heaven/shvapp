#pragma once

#include <QCoreApplication>
#include <QElapsedTimer>

#include <shv/iotqt/node/shvnodetree.h>
#include <shv/iotqt/rpc/deviceconnection.h>
#include <shv/iotqt/rpc/rpcresponsecallback.h>

class AppCliOptions;
class RootNode;
class DeviceMonitor;
class DirtyLogManager;
class LogSanitizer;

class Application : public QCoreApplication
{
	Q_OBJECT
	using Super = QCoreApplication;

public:
	static const int SINGLE_FILE_RECORD_COUNT;

	Application(int &argc, char **argv, AppCliOptions* cli_opts);
	~Application();

	static Application *instance();

	AppCliOptions *cliOptions() { return m_cliOptions; }

	QString elesysPath() const;
	QString sitesPath() const;
	QString shvSitesPath() const;
	QString masterBrokerPath() const;

	QString uptime() const;

	DeviceMonitor *deviceMonitor() { return m_deviceMonitor; }

	Q_SIGNAL void shvStateChanged(shv::iotqt::rpc::ClientConnection::State);

	void shvCall(const QString &shv_path, const QString &method, shv::iotqt::rpc::RpcResponseCallBack::CallBackFunction callback);
	void shvCall(const QString &shv_path, const QString &method, const shv::chainpack::RpcValue &params, shv::iotqt::rpc::RpcResponseCallBack::CallBackFunction callback);

	shv::iotqt::rpc::DeviceConnection *deviceConnection();

private:
	void onShvStateChanged();
	void connectToShv();
	void disconnectFromShv();
	void quit();
    void createNodes();

	AppCliOptions *m_cliOptions;
	RootNode *m_root;
	QElapsedTimer m_uptime;
	DeviceMonitor *m_deviceMonitor;
	LogSanitizer *m_logSanitizer;
	DirtyLogManager *m_dirtyLogManager;
	shv::iotqt::rpc::DeviceConnection *m_rpcConnection;
	shv::iotqt::node::ShvNodeTree *m_shvTree;
};
