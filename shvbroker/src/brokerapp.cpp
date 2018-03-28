#include "brokerapp.h"
#include "shvclientnode.h"
#include "brokernode.h"
#include "rpc/tcpserver.h"
#include "rpc/serverconnection.h"

#include <shv/iotqt/node/shvnode.h>
#include <shv/iotqt/node/shvnodetree.h>
#include <shv/coreqt/log.h>

#include <shv/core/string.h>
#include <shv/core/utils.h>
#include <shv/core/assert.h>
#include <shv/core/stringview.h>
#include <shv/chainpack/chainpackwriter.h>
#include <shv/chainpack/cponreader.h>
#include <shv/chainpack/rpcmessage.h>

#include <QSocketNotifier>
#include <QTimer>

#include <ctime>
#include <fstream>

#ifdef Q_OS_UNIX
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

static const std::string DIR_BROKER = ".broker";

namespace cp = shv::chainpack;
//#define logOpcuaReceive qfCInfo("OpcuaReceive")

#ifdef Q_OS_UNIX
int BrokerApp::m_sigTermFd[2];
#endif

//static constexpr int SQL_RECONNECT_INTERVAL = 3000;
BrokerApp::BrokerApp(int &argc, char **argv, AppCliOptions *cli_opts)
	: Super(argc, argv)
	, m_cliOptions(cli_opts)
{
	shvInfo() << "creating SHV BROKER application object ver." << versionString();
	std::srand(std::time(nullptr));
#ifdef Q_OS_UNIX
	//syslog (LOG_INFO, "Server started");
	installUnixSignalHandlers();
#endif

	cp::RpcMessage::setMetaTypeExplicit(cli_opts->isMetaTypeExplicit());

	connect(this, &BrokerApp::sqlServerConnected, this, &BrokerApp::onSqlServerConnected);
	/*
	m_sqlConnectionWatchDog = new QTimer(this);
	connect(m_sqlConnectionWatchDog, SIGNAL(timeout()), this, SLOT(reconnectSqlServer()));
	m_sqlConnectionWatchDog->start(SQL_RECONNECT_INTERVAL);
	*/
	m_deviceTree = new shv::iotqt::node::ShvNodeTree(this);
	BrokerNode *bn = new BrokerNode();
	m_deviceTree->mount(DIR_BROKER + "/app", bn);
	//m_deviceTree->mkdir("test");

	QTimer::singleShot(0, this, &BrokerApp::lazyInit);
}

BrokerApp::~BrokerApp()
{
	shvInfo() << "Destroying SHV BROKER application object";
	//QF_SAFE_DELETE(m_tcpServer);
	//QF_SAFE_DELETE(m_sqlConnector);
}

QString BrokerApp::versionString() const
{
	return QCoreApplication::applicationVersion();
}

rpc::TcpServer *BrokerApp::tcpServer()
{
	if(!m_tcpServer)
		SHV_EXCEPTION("TCP server is NULL!");
	return m_tcpServer;
}

#ifdef Q_OS_UNIX
void BrokerApp::installUnixSignalHandlers()
{
	shvInfo() << "installing Unix signals handlers";
	{
		struct sigaction term;

		term.sa_handler = BrokerApp::sigTermHandler;
		sigemptyset(&term.sa_mask);
		term.sa_flags |= SA_RESTART;

		if(sigaction(SIGTERM, &term, 0) > 0)
			qFatal("Couldn't register SIG_TERM handler");
	}
	if(::socketpair(AF_UNIX, SOCK_STREAM, 0, m_sigTermFd))
		qFatal("Couldn't create SIG_TERM socketpair");
	m_snTerm = new QSocketNotifier(m_sigTermFd[1], QSocketNotifier::Read, this);
	connect(m_snTerm, &QSocketNotifier::activated, this, &BrokerApp::handleSigTerm);
	shvInfo() << "SIG_TERM handler installed OK";
}

void BrokerApp::sigTermHandler(int)
{
	shvInfo() << "SIG TERM";
	char a = 1;
	::write(m_sigTermFd[0], &a, sizeof(a));
}

void BrokerApp::handleSigTerm()
{
	m_snTerm->setEnabled(false);
	char tmp;
	::read(m_sigTermFd[1], &tmp, sizeof(tmp));

	shvInfo() << "SIG TERM catched, stopping server.";
	QMetaObject::invokeMethod(this, "quit", Qt::QueuedConnection);

	m_snTerm->setEnabled(true);
}
#endif

/*
sql::SqlConnector *TheApp::sqlConnector()
{
	if(!m_sqlConnector || !m_sqlConnector->isOpen())
		QF_EXCEPTION("SQL server not connected!");
	return m_sqlConnector;
}

QString TheApp::serverProfile()
{
	return cliOptions()->profile();
}

QVariantMap TheApp::cliOptionsMap()
{
	return cliOptions()->values();
}

void TheApp::reconnectSqlServer()
{
	if(m_sqlConnector && m_sqlConnector->isOpen())
		return;
	QF_SAFE_DELETE(m_sqlConnector);
	m_sqlConnector = new sql::SqlConnector(this);
	connect(m_sqlConnector, SIGNAL(sqlServerError(QString)), this, SLOT(onSqlServerError(QString)), Qt::QueuedConnection);
	//connect(m_sqlConnection, SIGNAL(openChanged(bool)), this, SLOT(onSqlServerConnectedChanged(bool)), Qt::QueuedConnection);

	QString host = cliOptions()->sqlHost();
	int port = cliOptions()->sqlPort();
	m_sqlConnector->open(host, port);
	if (m_sqlConnector->isOpen()) {
		emit sqlServerConnected();
	}
}
*/
void BrokerApp::onSqlServerError(const QString &err_mesg)
{
	Q_UNUSED(err_mesg)
	//SHV_SAFE_DELETE(m_sqlConnector);
	//m_sqlConnectionWatchDog->start(SQL_RECONNECT_INTERVAL);
}

void BrokerApp::onSqlServerConnected()
{
	//connect(depotModel(), &DepotModel::valueChangedWillBeEmitted, m_sqlConnector, &sql::SqlConnector::saveDepotModelJournal, Qt::UniqueConnection);
	//connect(this, &TheApp::opcValueWillBeSet, m_sqlConnector, &sql::SqlConnector::saveDepotModelJournal, Qt::UniqueConnection);
	//m_depotModel->setValue(QStringList() << QStringLiteral("server") << QStringLiteral("startTime"), QVariant::fromValue(QDateTime::currentDateTime()), !shv::core::Exception::Throw);
}

void BrokerApp::startTcpServer()
{
	SHV_SAFE_DELETE(m_tcpServer);
	m_tcpServer = new rpc::TcpServer(this);
	if(!m_tcpServer->start(cliOptions()->serverPort())) {
		SHV_EXCEPTION("Cannot start TCP server!");
	}
}

void BrokerApp::lazyInit()
{
	//reconnectSqlServer();
	startTcpServer();
}

std::string BrokerApp::mountPointForDevice(const shv::chainpack::RpcValue &device_id)
{
	if(!m_fstab.isValid()) {
		std::string fn = m_cliOptions->fstab().toStdString();
		if(!fn.empty()) {
			std::ifstream is(fn, std::ios::binary);
			if (!is.is_open()) {
				shvError() << "Cannot open fstab file" << fn << "for reading";
			}
			else {
				cp::CponReader rd(is);
				std::string err;
				rd.read(m_fstab, err);
				if(!err.empty())
					shvError() << "Error parsing fstab file:" << err;
			}
		}
		if(!m_fstab.isValid())
			m_fstab = cp::RpcValue::Map();
	}
	const std::string dev_id = device_id.toStdString();
	std::string mount_point = m_fstab.toMap().value(dev_id).toString();
	return mount_point;
}

void BrokerApp::onClientLogin(int connection_id)
{
	rpc::ServerConnection *conn = tcpServer()->connectionById(connection_id);
	if(!conn)
		SHV_EXCEPTION("Cannot find connection for ID: " + std::to_string(connection_id));
	const shv::chainpack::RpcValue::Map &opts = conn->connectionOptions();
	if(conn->connectionType() == cp::Rpc::TYPE_DEVICE) do {
		shv::chainpack::RpcValue device_id = conn->deviceId();
		std::string mount_point = mountPointForDevice(device_id);
		if(mount_point.empty()) {
			const shv::chainpack::RpcValue::Map &device = opts.value(cp::Rpc::TYPE_DEVICE).toMap();
			mount_point = device.value("mount").toString();
			std::vector<shv::core::StringView> path = shv::core::StringView(mount_point).split('/');
			//if(path.empty())
			//	SHV_EXCEPTION("Cannot find mount point for device: " + device_id.toCpon());
			if(path.empty() || !(path[0] == "test")) {
				shvWarning() << "Mount point can be explicitly specified to test/ dir only, dev id:" << device_id.toCpon();
				mount_point.clear();
			}
		}
		if(mount_point.empty()) {
			//SHV_EXCEPTION("Mount point is empty.");
			mount_point = DIR_BROKER + "/mountError/" + std::to_string(connection_id);
			shvWarning() << "Device will be mounted to" << mount_point << ", dev id:" << device_id.toCpon();
		}
		ShvClientNode *nd = new ShvClientNode(conn);
		shvInfo() << "Client node:" << nd << "connection id:" << connection_id << "mounting device on path:" << mount_point;
		ShvClientNode *curr_cli_nd = qobject_cast<ShvClientNode*>(m_deviceTree->cd(mount_point));
		if(curr_cli_nd) {
			shvWarning() << "The mount point" << mount_point << "exists already";
			if(curr_cli_nd->connection()->deviceId() == device_id) {
				shvWarning() << "The same device ID will be remounted:" << device_id.toCpon();
				curr_cli_nd->connection()->abort();
				delete curr_cli_nd;
			}
		}
		if(!m_deviceTree->mount(mount_point, nd))
			SHV_EXCEPTION("Cannot mount connection to device tree, connection id: " + std::to_string(connection_id));
		conn->setMountPoint(nd->shvPath());
		connect(conn, &rpc::ServerConnection::destroyed, nd, &ShvClientNode::deleteLater);
		connect(conn, &rpc::ServerConnection::destroyed, [this, connection_id, mount_point]() {
			shvInfo() << "server connection destroyed";
			sendNotifyToSubscribers(connection_id, mount_point, cp::Rpc::NTF_DISCONNECTED, cp::RpcValue());
		});
		sendNotifyToSubscribers(connection_id, mount_point, cp::Rpc::NTF_CONNECTED, cp::RpcValue());
	} while(false);
	//shvInfo() << m_deviceTree->dumpTree();
}

void BrokerApp::onRpcDataReceived(unsigned connection_id, cp::RpcValue::MetaData &&meta, std::string &&data)
{
	if(cp::RpcMessage::isRequest(meta)) {
		shvDebug() << "REQUEST conn id:" << connection_id << meta.toStdString();
		cp::RpcMessage::pushCallerId(meta, connection_id);
		const std::string shv_path = cp::RpcMessage::shvPath(meta);
		std::string path_rest;
		shv::iotqt::node::ShvNode *nd = m_deviceTree->cd(shv_path, &path_rest);
		ShvClientNode *client_nd = qobject_cast<ShvClientNode *>(nd);
		//shvWarning() << nd << client_nd << "shv path:" << shv_path << "rest:" << path_rest;// << meta.toStdString();
		if(client_nd) {
			cp::RpcMessage::setShvPath(meta, path_rest);
			rpc::ServerConnection *conn2 = client_nd->connection();
			conn2->sendRawData(std::move(meta), std::move(data));
		}
		else if(nd) {
			cp::RpcMessage rpc_msg = cp::RpcDriver::composeRpcMessage(std::move(meta), data, cp::Exception::Throw);
			rpc_msg.setShvPath(path_rest);
			cp::RpcRequest rq(rpc_msg);
			cp::RpcResponse resp = cp::RpcResponse::forRequest(rq);
			try {
				cp::RpcValue result = nd->processRpcRequest(rpc_msg);
				resp.setResult(result);
			}
			catch (shv::core::Exception &e) {
				shvError() << e.message();
				resp.setError(cp::RpcResponse::Error::create(cp::RpcResponse::Error::MethodInvocationException, e.message()));
			}
			rpc::ServerConnection *conn = tcpServer()->connectionById(connection_id);
			if(conn)
				conn->sendMessage(resp);
			else
				shvError() << "Cannot find connection for ID:" << connection_id;
		}
		else {
			std::string err_msg = "Device tree path not found: " + shv_path;
			shvWarning() << err_msg;
			cp::RpcMessage rpc_msg = cp::RpcDriver::composeRpcMessage(std::move(meta), data, !cp::Exception::Throw);
			if(rpc_msg.isRequest()) {
				rpc::ServerConnection *conn = tcpServer()->connectionById(connection_id);
				if(conn) {
					conn->sendError(rpc_msg.requestId(), cp::RpcResponse::Error::create(
										cp::RpcResponse::Error::MethodInvocationException
										, err_msg));
				}
			}
		}
	}
	else if(cp::RpcMessage::isResponse(meta)) {
		shvDebug() << "RESPONSE conn id:" << connection_id << meta.toStdString();
		cp::RpcValue::UInt caller_id = cp::RpcMessage::popCallerId(meta);
		if(caller_id > 0) {
			rpc::ServerConnection *conn = tcpServer()->connectionById(caller_id);
			if(conn)
				conn->sendRawData(std::move(meta), std::move(data));
			else
				shvWarning() << "Got RPC response for not-exists connection, may be it was closed meanwhile. Connection id:" << caller_id;
		}
		else {
			shvError() << "Got RPC response without src connection specified, throwing message away.";
		}
	}
	else if(cp::RpcMessage::isNotify(meta)) {
		shvDebug() << "NOTIFY:" << meta.toStdString() << "from:" << connection_id;
		std::string full_shv_path;
		{
			rpc::ServerConnection *conn = tcpServer()->connectionById(connection_id);
			if(conn) {
				full_shv_path = conn->mountPoint();
				if(!full_shv_path.empty())
					full_shv_path += cp::RpcMessage::shvPath(meta);
			}
		}
		if(!full_shv_path.empty()) {
			cp::RpcMessage::setShvPath(meta, full_shv_path);
			sendNotifyToSubscribers(connection_id, meta, data);
		}
	}
}

void BrokerApp::sendNotifyToSubscribers(unsigned sender_connection_id, const shv::chainpack::RpcValue::MetaData &meta_data, const std::string &data)
{
	// send it to all clients for now
	for(unsigned id : tcpServer()->connectionIds()) {
		if(id == sender_connection_id)
			continue;
		rpc::ServerConnection *conn = tcpServer()->connectionById(id);
		if(conn && conn->isBrokerConnected()) {
			shvDebug() << "\t broadcasting to connection id:" << id;
			conn->sendRawData(meta_data, std::string(data));
		}
	}
}

void BrokerApp::sendNotifyToSubscribers(unsigned sender_connection_id, const std::string &shv_path, std::string method, const shv::chainpack::RpcValue &params)
{
	cp::RpcNotify ntf;
	ntf.setShvPath(shv_path);
	ntf.setMethod(std::move(method));
	ntf.setParams(params);
	// send it to all clients for now
	for(unsigned id : tcpServer()->connectionIds()) {
		if(id == sender_connection_id)
			continue;
		rpc::ServerConnection *conn = tcpServer()->connectionById(id);
		if(conn && conn->isBrokerConnected()) {
			shvDebug() << "\t broadcasting to connection id:" << id;
			conn->sendMessage(ntf);
		}
	}
}


