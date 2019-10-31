#pragma once

#include <shv/iotqt/node/shvnode.h>
#include <shv/coreqt/utils.h>

#include <QApplication>
#include <QDateTime>

class AppCliOptions;

//namespace shv { namespace chainpack { class RpcMessage; }}

class Ams005YcpApp : public QApplication
{
	Q_OBJECT
private:
	using Super = QApplication;

public:
	Ams005YcpApp(int &argc, char **argv, AppCliOptions* cli_opts);
	~Ams005YcpApp() Q_DECL_OVERRIDE;

	static Ams005YcpApp *instance();
	AppCliOptions* cliOptions() {return m_cliOptions;}

	void loadSettings();

	Q_SIGNAL void opcDeviceConnectedChanged(bool is_connected);

	QVariant reloadOpcValue(const QString &path);
	QVariant opcValue(const QString &path);
	Q_SIGNAL void opcValueChanged(const std::string &path, const QVariant &value);
private:
	AppCliOptions* m_cliOptions;

	//QTimer *m_shvDeviceConnectedCheckTimer = nullptr;
};

