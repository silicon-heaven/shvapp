#pragma once

#include <shv/iotqt/node/shvnodetree.h>
#include <shv/iotqt/rpc/deviceconnection.h>

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QTimer>

class AppCliOptions;
class RootNode;
class DeviceMonitor;
class DirtyLogManager;
class DiskCleaner;
class LogSanitizer;

class Application : public QCoreApplication
{
	Q_OBJECT
	using Super = QCoreApplication;

public:
	static const int CHUNK_RECORD_COUNT;
	static const char *DIRTY_LOG_NODE;
	static const char *START_TS_NODE;

	Application(int &argc, char **argv, AppCliOptions* cli_opts);
	~Application();

	static Application *instance();
	AppCliOptions *cliOptions() { return m_cliOptions; }
	DeviceMonitor *deviceMonitor() { return m_deviceMonitor; }
	LogSanitizer *logSanitizer() { return m_logSanitizer; }
	shv::iotqt::rpc::DeviceConnection *deviceConnection();
	void registerAlienFile(const QString &filename);
	QString uptime() const;

private:
	void onShvStateChanged();
	void connectToShv();
	void disconnectFromShv();
	void quit();
	void logAlienFiles();

	AppCliOptions *m_cliOptions;
	RootNode *m_root;
	QElapsedTimer m_uptime;
	DeviceMonitor *m_deviceMonitor;
	LogSanitizer *m_logSanitizer;
	DiskCleaner *m_diskCleaner;
	DirtyLogManager *m_dirtyLogManager;
	shv::iotqt::rpc::DeviceConnection *m_rpcConnection;
	shv::iotqt::node::ShvNodeTree *m_shvTree;
	QStringList m_alienFiles;
	QTimer m_logAlienFilesTimer;
};
