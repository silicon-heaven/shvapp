#include "application.h"
#include "appclioptions.h"
#include "devicemonitor.h"
#include "dirtylogmanager.h"
#include "logsanitizer.h"
#include "rootnode.h"
#include "diskcleaner.h"
#include "shvsubscription.h"

#include <shv/coreqt/log.h>
#include <shv/coreqt/exception.h>
#include <shv/iotqt/rpc/rpccall.h>

#include <QTimer>

namespace cp = shv::chainpack;

const int Application::CHUNK_RECORD_COUNT = 10000;
const char *Application::DIRTY_LOG_NODE = "dirtyLog";
const char *Application::START_TS_NODE = "startTS";

Application::Application(int &argc, char **argv, AppCliOptions* cli_opts)
	: Super(argc, argv)
	, m_cliOptions(cli_opts)
	, m_root(nullptr)
	, m_diskCleaner(nullptr)
	, m_chngSubscription(nullptr)
	, m_cmdLogSubscription(nullptr)
{
	qRegisterMetaType<DateTimeInterval>();
	qRegisterMetaType<QVector<DateTimeInterval>>();

	if (!cli_opts->deviceId_isset()) {
		cli_opts->setDeviceId("shvhistoryprovider");
	}

	m_rpcConnection = new shv::iotqt::rpc::DeviceConnection(this);
	if (!cli_opts->serverHost_isset()) {
        cli_opts->setServerHost("localhost");
    }
    if (!cli_opts->user_isset()) {
        cli_opts->setUser("shvhistoryprovider");
    }
    if (!cli_opts->password_isset()) {
        cli_opts->setPassword("lub42DUB");
		cli_opts->setLoginType("PLAIN");
    }
	m_rpcConnection->setCliOptions(cli_opts);

	connect(m_rpcConnection, &shv::iotqt::rpc::ClientConnection::stateChanged, this, &Application::onShvStateChanged);

	m_uptime.start();
	connectToShv();

	m_deviceMonitor = new DeviceMonitor(this);

	QEventLoop loop(this);
	connect(m_deviceMonitor, &DeviceMonitor::sitesDownloadFinished, &loop, &QEventLoop::quit);
	loop.exec();

	m_logSanitizer = new LogSanitizer(this);
	m_dirtyLogManager = new DirtyLogManager(this);

	if (cli_opts->cacheSizeLimit_isset()) {
		QString limit = QString::fromStdString(cli_opts->cacheSizeLimit());
		char suffix = 0;
		if (limit.endsWith('k') || limit.endsWith('M') || limit.endsWith('G')) {
			suffix = limit[limit.length() - 1].toLatin1();
			limit = limit.left(limit.length() - 1);
		}
		int64_t cache_size_limit = limit.toLongLong();

		if (cache_size_limit > 0LL) {
			switch (suffix) {
			case 'k':
				cache_size_limit *= 1000;
				break;
			case 'M':
				cache_size_limit *= 1000000;
				break;
			case 'G':
				cache_size_limit *= 1000000000;
				break;
			}
			m_diskCleaner = new DiskCleaner(cache_size_limit, this);
		}
	}
}

Application::~Application()
{
	shvInfo() << "destroying" << QCoreApplication::applicationName() << "application";
	if (m_chngSubscription) {
		delete m_chngSubscription;
	}
	if (m_cmdLogSubscription) {
		delete m_cmdLogSubscription;
	}
}

shv::iotqt::rpc::DeviceConnection *Application::deviceConnection()
{
	return m_rpcConnection;
}

void Application::registerAlienFile(const QString &filename)
{
	if (!m_alienFiles.contains(filename)) {
		m_alienFiles << filename;
		shvWarning() << "Alien file in HP cache:" << filename;
	}
}

void Application::connectToShv()
{
	shvInfo() << "Connecting to SHV Broker";
	m_rpcConnection->open();
}

void Application::disconnectFromShv()
{
	if (m_rpcConnection->state() != shv::iotqt::rpc::DeviceConnection::State::BrokerConnected) {
		return;
	}
	m_rpcConnection->close();
}

void Application::quit()
{
	if (m_rpcConnection->state() == shv::iotqt::rpc::DeviceConnection::State::BrokerConnected) {
		connect(m_rpcConnection, &shv::iotqt::rpc::DeviceConnection::stateChanged, []() {
			Super::quit();
		});
		m_rpcConnection->close();
	}
	else {
		Super::quit();
	}
}

Application *Application::instance()
{
	return qobject_cast<Application *>(QCoreApplication::instance());
}

QString Application::uptime() const
{
	qint64 elapsed = m_uptime.elapsed();
	int ms = static_cast<int>(elapsed) % 1000;
	elapsed /= 1000;
	int sec = static_cast<int>(elapsed) % 60;
	elapsed /= 60;
	int min = static_cast<int>(elapsed) % 60;
	elapsed /= 60;
	int hour = static_cast<int>(elapsed) % 24;
	int day = static_cast<int>(elapsed) / 24;
	return QString("%1 day(s) %2:%3:%4.%5")
			.arg(day)
			.arg(hour, 2, 10, QChar('0'))
			.arg(min, 2, 10, QChar('0'))
			.arg(sec, 2, 10, QChar('0'))
			.arg(ms, 3, 10, QChar('0'));
}

void Application::onShvStateChanged()
{
	if (m_rpcConnection->state() == shv::iotqt::rpc::ClientConnection::State::BrokerConnected) {
		shvInfo() << "SHV Broker connected";
		shv::iotqt::rpc::RpcCall *call = shv::iotqt::rpc::RpcCall::create(m_rpcConnection)
										 ->setShvPath(".broker/app")
										 ->setMethod("brokerId");
		connect(call, &shv::iotqt::rpc::RpcCall::error, this, [](const QString &error) {
			shvError() << "Cannot get broker ID:" << error;
		});
		connect(call, &shv::iotqt::rpc::RpcCall::result, this, [this](const cp::RpcValue &result) {
			m_brokerId = QString::fromStdString(result.toString());
		});
		call->start();
		if (!m_root) {
			m_root = new RootNode();
			m_shvTree = new shv::iotqt::node::ShvNodeTree(m_root, this);
			connect(m_shvTree->root(), &shv::iotqt::node::ShvRootNode::sendRpcMessage, m_rpcConnection, &shv::iotqt::rpc::ClientConnection::sendMessage);
			connect(m_rpcConnection, &shv::iotqt::rpc::DeviceConnection::rpcMessageReceived, m_root, &RootNode::onRpcMessageReceived);
		}

		QString shv_sites_path = QString::fromStdString(m_cliOptions->sitesRootPath());
		QString path = "shv";
		if (!shv_sites_path.isEmpty()) {
			path += '/' + shv_sites_path;
		}
		m_chngSubscription = new ShvSubscription(m_rpcConnection, path, shv::chainpack::Rpc::SIG_VAL_CHANGED, this);
		connect(m_chngSubscription, &ShvSubscription::notificationReceived, this, &Application::onDataChanged);
		m_cmdLogSubscription = new ShvSubscription(m_rpcConnection, path, shv::chainpack::Rpc::SIG_COMMAND_LOGGED, this);
		connect(m_cmdLogSubscription, &ShvSubscription::notificationReceived, this, &Application::onDataChanged);
	}
	else if (m_rpcConnection->state() == shv::iotqt::rpc::ClientConnection::State::NotConnected) {
		shvInfo() << "SHV Broker disconnected";

		if (m_chngSubscription) {
			delete m_chngSubscription;
			m_chngSubscription = nullptr;
		}
		if (m_cmdLogSubscription) {
			delete m_cmdLogSubscription;
			m_cmdLogSubscription = nullptr;
		}
	}
}

void Application::onDataChanged(const QString &shv_path, const QString &method, const shv::chainpack::RpcValue &data)
{
	QString p = shv_path.mid(4);
	QString site_path;
	QString property;
	for (const QString &device : m_deviceMonitor->monitoredDevices()) {
		if (p.startsWith(device)) {
			site_path = device;
			property = p.mid(device.length() + 1);
			break;
		}
	}
	if (!site_path.isEmpty()) {
		Q_EMIT deviceDataChanged(site_path, property, method, data);
	}
}
