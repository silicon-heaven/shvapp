#include "brokerapp.h"
#include "clientshvnode.h"
#include "brokernode.h"
#include "subscriptionsnode.h"
#include "brokerconfigfilenode.h"
#include "clientconnectionnode.h"
#include "rpc/tcpserver.h"
#include "rpc/clientbrokerconnection.h"
#include "rpc/masterbrokerconnection.h"
#include "utils/network.h"

#ifdef WITH_SHV_WEBSOCKETS
#include "rpc/websocketserver.h"
#endif

#include <shv/iotqt/node/shvnode.h>
#include <shv/iotqt/node/shvnodetree.h>
#include <shv/iotqt/rpc/brokerconnection.h>
#include <shv/core/utils/shvpath.h>

#include <shv/coreqt/log.h>

#include <shv/core/string.h>
#include <shv/core/utils.h>
#include <shv/core/assert.h>
#include <shv/core/stringview.h>
#include <shv/chainpack/chainpackwriter.h>
#include <shv/chainpack/cponreader.h>
#include <shv/chainpack/rpcmessage.h>
#include <shv/chainpack/metamethod.h>
#include <shv/chainpack/cponwriter.h>
#include <shv/chainpack/tunnelctl.h>
#include <shv/chainpack/accessgrant.h>

#include <QFile>
#include <QSocketNotifier>
#include <QTimer>

#include <ctime>
#include <fstream>

#ifdef Q_OS_UNIX
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

int BrokerApp::m_sigTermFd[2];
#endif

#define logTunnelD() nCDebug("Tunnel")
#define logAclD() nCDebug("Acl")
#define logAccessD() nCDebug("Access").color(NecroLog::Color::Green)
#define logSubscriptionsD() nCDebug("Subscr").color(NecroLog::Color::Yellow)
#define logSigResolveD() nCDebug("SigRes").color(NecroLog::Color::LightGreen)

namespace cp = shv::chainpack;

class ClientsNode : public shv::iotqt::node::MethodsTableNode
{
	using Super = shv::iotqt::node::MethodsTableNode;
public:
	ClientsNode(shv::iotqt::node::ShvNode *parent = nullptr)
		: Super(std::string(), &m_metaMethods, parent)
		, m_metaMethods {
				  {cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam},
				  {cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::GRANT_CONFIG},
		}
	{ }
private:
	std::vector<cp::MetaMethod> m_metaMethods;
};

class MasterBrokersNode : public shv::iotqt::node::MethodsTableNode
{
	using Super = shv::iotqt::node::MethodsTableNode;
public:
	MasterBrokersNode(shv::iotqt::node::ShvNode *parent = nullptr)
		: Super(std::string(), &m_metaMethods, parent)
		, m_metaMethods{
				  {cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam},
				  {cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::GRANT_CONFIG},
		}
	{ }
private:
	std::vector<cp::MetaMethod> m_metaMethods;
};

class MountsNode : public shv::iotqt::node::ShvNode
{
	using Super = shv::iotqt::node::ShvNode;
public:
	MountsNode(shv::iotqt::node::ShvNode *parent = nullptr)
		: Super(parent)
	{ }

	size_t methodCount(const StringViewList &shv_path) override
	{
		if(shv_path.empty())
			return m_metaMethods.size() - 1;
		if(shv_path.size() == 1)
			return m_metaMethods.size();
		return Super::methodCount(shv_path);
	}

	const shv::chainpack::MetaMethod *metaMethod(const StringViewList &shv_path, size_t ix) override
	{
		if(methodCount(shv_path) <= ix)
			SHV_EXCEPTION("Invalid method index: " + std::to_string(ix) + " of: " + std::to_string(methodCount(shv_path)));
		if(shv_path.empty())
			return &(m_metaMethods[ix]);
		if(shv_path.size() == 1)
			return &(m_metaMethods[ix]);
		return Super::metaMethod(shv_path, ix);
	}

	StringList childNames(const StringViewList &shv_path) override
	{
		if(shv_path.empty()) {
			BrokerApp *app = BrokerApp::instance();
			StringList lst;
			for(int id : app->clientConnectionIds()) {
				rpc::ClientBrokerConnection *conn = app->clientConnectionById(id);
				for(const std::string &mp : conn->mountPoints()) {
					lst.push_back(shv::core::utils::ShvPath::SHV_PATH_QUOTE + mp + shv::core::utils::ShvPath::SHV_PATH_QUOTE);
				}
			}
			return lst;
		}
		else {
			if(shv_path.size() == 1) {
				return StringList();
			}
		}
		return Super::childNames(shv_path);
	}

	shv::chainpack::RpcValue callMethod(const StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params) override
	{
		if(shv_path.size() == 1) {
			if(method == METH_CLIENT_IDS) {
				BrokerApp *app = BrokerApp::instance();
				ClientShvNode *nd = qobject_cast<ClientShvNode*>(app->m_nodesTree->cd(shv_path.at(0).toString()));
				if(nd == nullptr)
					SHV_EXCEPTION("Cannot find client node on path: " + shv_path.at(0).toString())
				cp::RpcValue::List lst;
				for(rpc::ClientBrokerConnection *conn : nd->connections())
					lst.push_back(conn->connectionId());
				return cp::RpcValue{lst};
			}
		}
		return Super::callMethod(shv_path, method, params);
	}
private:
	static const char *METH_CLIENT_IDS;
	static std::vector<cp::MetaMethod> m_metaMethods;
};

const char *MountsNode::METH_CLIENT_IDS = "clientIds";

std::vector<cp::MetaMethod> MountsNode::m_metaMethods = {
	{cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam},
	{cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::GRANT_CONFIG},
	{METH_CLIENT_IDS, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsGetter, cp::Rpc::GRANT_CONFIG},
};

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

	//cp::RpcMessage::setMetaTypeExplicit(cli_opts->isMetaTypeExplicit());

	//connect(this, &BrokerApp::sqlServerConnected, this, &BrokerApp::onSqlServerConnected);
	/*
	m_sqlConnectionWatchDog = new QTimer(this);
	connect(m_sqlConnectionWatchDog, SIGNAL(timeout()), this, SLOT(reconnectSqlServer()));
	m_sqlConnectionWatchDog->start(SQL_RECONNECT_INTERVAL);
	*/
	m_nodesTree = new shv::iotqt::node::ShvNodeTree(this);
	connect(m_nodesTree->root(), &shv::iotqt::node::ShvRootNode::sendRpcMessage, this, &BrokerApp::onRootNodeSendRpcMesage);
	BrokerNode *bn = new BrokerNode();
	m_nodesTree->mount(cp::Rpc::DIR_BROKER_APP, bn);
	m_nodesTree->mount(std::string(cp::Rpc::DIR_BROKER) + "/clients", new ClientsNode());
	m_nodesTree->mount(std::string(cp::Rpc::DIR_BROKER) + "/masters", new MasterBrokersNode());
	m_nodesTree->mount(std::string(cp::Rpc::DIR_BROKER) + "/mounts", new MountsNode());
	m_nodesTree->mount(std::string(cp::Rpc::DIR_BROKER) + "/etc/acl", new EtcAclNode());

	QTimer::singleShot(0, this, &BrokerApp::lazyInit);
}

BrokerApp::~BrokerApp()
{
	shvInfo() << "Destroying SHV BROKER application object";
	//QF_SAFE_DELETE(m_tcpServer);
	//QF_SAFE_DELETE(m_sqlConnector);
}

#ifdef Q_OS_UNIX
void BrokerApp::installUnixSignalHandlers()
{
	shvInfo() << "installing Unix signals handlers";
	for(int sig_num : {SIGTERM, SIGHUP}) {
		struct sigaction sa;

		sa.sa_handler = BrokerApp::nativeSigHandler;
		sigemptyset(&sa.sa_mask);
		sa.sa_flags |= SA_RESTART;

		if(sigaction(sig_num, &sa, nullptr) > 0)
			qFatal("Couldn't register posix signal handler");
	}
	if(::socketpair(AF_UNIX, SOCK_STREAM, 0, m_sigTermFd))
		qFatal("Couldn't create SIG_TERM socketpair");
	m_snTerm = new QSocketNotifier(m_sigTermFd[1], QSocketNotifier::Read, this);
	connect(m_snTerm, &QSocketNotifier::activated, this, &BrokerApp::handlePosixSignals);
	shvInfo() << "SIG_TERM handler installed OK";
}

void BrokerApp::nativeSigHandler(int sig_number)
{
	shvInfo() << "SIG:" << sig_number;
	unsigned char a = static_cast<unsigned char>(sig_number);
	::write(m_sigTermFd[0], &a, sizeof(a));
}

void BrokerApp::handlePosixSignals()
{
	m_snTerm->setEnabled(false);
	unsigned char sig_num;
	::read(m_sigTermFd[1], &sig_num, sizeof(sig_num));

	shvInfo() << "SIG" << sig_num << "catched.";
	if(sig_num == SIGTERM) {
		QTimer::singleShot(0, this, &BrokerApp::quit);
	}
	else if(sig_num == SIGHUP) {
		//QMetaObject::invokeMethod(this, &BrokerApp::reloadConfig, Qt::QueuedConnection); since Qt 5.10
		QTimer::singleShot(0, this, &BrokerApp::reloadConfig);
	}

	m_snTerm->setEnabled(true);
}
#endif

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

rpc::ClientBrokerConnection *BrokerApp::clientById(int client_id)
{
	return clientConnectionById(client_id);
}

#ifdef WITH_SHV_WEBSOCKETS
rpc::WebSocketServer *BrokerApp::webSocketServer()
{
	if(!m_webSocketServer)
		SHV_EXCEPTION("WebSocket server is NULL!");
	return m_webSocketServer;
}
#endif

void BrokerApp::reloadConfig()
{
	shvInfo() << "Reloading config";
	reloadAcl();
	remountDevices();
	emit configReloaded();
}

void BrokerApp::clearAccessGrantCache()
{
	m_userFlattenGrantsCache.clear();
#ifdef USE_SHV_PATHS_GRANTS_CACHE
	m_userPathGrantCache.clear();
#endif
}

void BrokerApp::reloadAcl()
{
	m_usersConfig = cp::RpcValue();
	m_grantsConfig = cp::RpcValue();
	m_pathsConfig = cp::RpcValue();
	clearAccessGrantCache();
}

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
*/

void BrokerApp::startTcpServer()
{
	SHV_SAFE_DELETE(m_tcpServer);
	m_tcpServer = new rpc::TcpServer(this);
	if(!m_tcpServer->start(cliOptions()->serverPort())) {
		SHV_EXCEPTION("Cannot start TCP server!");
	}
}

void BrokerApp::startWebSocketServer()
{
#ifdef WITH_SHV_WEBSOCKETS
	SHV_SAFE_DELETE(m_webSocketServer);
	int port = cliOptions()->serverWebsocketPort();
	if(port > 0) {
		m_webSocketServer = new rpc::WebSocketServer(this);
		if(!m_webSocketServer->start(port)) {
			SHV_EXCEPTION("Cannot start WebSocket server!");
		}
	}
	else {
		shvInfo() << "Websocket server port is not set, it will not be started.";
	}
#else
	shvWarning() << "Websocket server is not included in this build";
#endif
}

rpc::ClientBrokerConnection *BrokerApp::clientConnectionById(int connection_id)
{
	rpc::ClientBrokerConnection *conn = tcpServer()->connectionById(connection_id);
#ifdef WITH_SHV_WEBSOCKETS
	if(!conn && m_webSocketServer)
		conn = m_webSocketServer->connectionById(connection_id);
#endif
	return conn;
}

std::vector<int> BrokerApp::clientConnectionIds()
{
	std::vector<int> ids = tcpServer()->connectionIds();
#ifdef WITH_SHV_WEBSOCKETS
	if(m_webSocketServer) {
		std::vector<int> ids2 = m_webSocketServer->connectionIds();
		ids.insert( ids.end(), ids2.begin(), ids2.end() );
	}
#endif
	return ids;
}

void BrokerApp::lazyInit()
{
	startTcpServer();
	startWebSocketServer();
	createMasterBrokerConnections();
}

shv::chainpack::RpcValue BrokerApp::aclConfig(const std::string &config_name, bool throw_exc)
{
	shv::chainpack::RpcValue *config_val = aclConfigVariable(config_name, throw_exc);
	if(config_val) {
		if(!config_val->isValid()) {
			*config_val = loadAclConfig(config_name, throw_exc);
			logAclD() << "ACL config:" << config_name << "loaded:\n" << config_val->toCpon("\t");
		}
		if(!config_val->isValid())
			*config_val = cp::RpcValue::Map{}; /// will not be loaded next time
		return *config_val;
	}
	else {
		if(throw_exc) {
			throw std::runtime_error("Config " + config_name + " does not exist.");
		}
		else {
			//shvError().nospace() << "Config '" << config_name << "' does not exist.";
			return cp::RpcValue();
		}
	}
}

bool BrokerApp::setAclConfig(const std::string &config_name, const shv::chainpack::RpcValue &config, bool throw_exc)
{
	shv::chainpack::RpcValue *config_val = aclConfigVariable(config_name, throw_exc);
	if(config_val) {
		if(saveAclConfig(config_name, config, throw_exc)) {
			*config_val = config;
			clearAccessGrantCache();
			return true;
		}
		return false;
	}
	else {
		if(throw_exc)
			throw std::runtime_error("Config " + config_name + " does not exist.");
		else
			return false;
	}
}

bool BrokerApp::checkTunnelSecret(const std::string &s)
{
	return m_tunnelSecretList.checkSecret(s);
}

std::string BrokerApp::dataToCpon(shv::chainpack::Rpc::ProtocolType protocol_type, const shv::chainpack::RpcValue::MetaData &md, const std::string &data, size_t start_pos, size_t data_len)
{
	shv::chainpack::RpcValue rpc_val;
	if(data_len == 0)
		data_len = data.size() - start_pos;
	if(data_len < 256) {
		rpc_val = shv::chainpack::RpcDriver::decodeData(protocol_type, data, start_pos);
	}
	else {
		rpc_val = " ... " + std::to_string(data_len) + " bytes of data ... ";
	}
	rpc_val.setMetaData(shv::chainpack::RpcValue::MetaData(md));
	return rpc_val.toPrettyString();
}

void BrokerApp::remountDevices()
{
	shvInfo() << "Remounting devices by dropping their connection";
	m_fstabConfig = cp::RpcValue();
	for(int conn_id : clientConnectionIds()) {
		rpc::ClientBrokerConnection *conn = clientConnectionById(conn_id);
		if(conn && !conn->mountPoints().empty()) {
			shvInfo() << "Dropping connection ID:" << conn_id << "mounts:" << shv::core::String::join(conn->mountPoints(), ' ');
			conn->close();
		}
	}
}

shv::chainpack::RpcValue *BrokerApp::aclConfigVariable(const std::string &config_name, bool throw_exc)
{
	shv::chainpack::RpcValue *config_val = nullptr;
	if(config_name == "fstab")
		config_val = &m_fstabConfig;
	else if(config_name == "users")
		config_val = &m_usersConfig;
	else if(config_name == "grants")
		config_val = &m_grantsConfig;
	else if(config_name == "paths")
		config_val = &m_pathsConfig;
	if(config_val) {
		return config_val;
	}
	else {
		if(throw_exc)
			throw std::runtime_error("Config " + config_name + " does not exist.");
		else
			return nullptr;
	}
}

shv::chainpack::RpcValue BrokerApp::loadAclConfig(const std::string &config_name, bool throw_exc)
{
	std::string fn = config_name;
	fn = cliOptions()->value("etc.acl." + fn).toString();
	if(fn.empty()) {
		if(throw_exc)
			throw std::runtime_error("config file name is empty.");
		else
			return cp::RpcValue();
	}
	if(fn[0] != '/')
		fn = cliOptions()->configDir() + '/' + fn;
	std::ifstream fis(fn);
	QFile f(QString::fromStdString(fn));
	if (!fis.good()) {
		if(throw_exc)
			throw std::runtime_error("Cannot open config file " + fn + " for reading");
		else
			return cp::RpcValue();
	}
	shv::chainpack::CponReader rd(fis);
	shv::chainpack::RpcValue rv;
	std::string err;
	rv = rd.read(throw_exc? nullptr: &err);
	return rv;
}

bool BrokerApp::saveAclConfig(const std::string &config_name, const shv::chainpack::RpcValue &config, bool throw_exc)
{
	logAclD() << "saveAclConfig" << config_name << "config type:" << config.typeToName(config.type());
	//logAclD() << "config:" << config.toCpon();
	std::string fn = config_name;
	fn = cliOptions()->value("etc.acl." + fn).toString();
	if(fn.empty()) {
		if(throw_exc)
			throw std::runtime_error("config file name is empty.");
		else
			return false;
	}
	if(fn[0] != '/')
		fn = cliOptions()->configDir() + '/' + fn;

	if(config.isMap()) {
		std::ofstream fos(fn, std::ios::binary | std::ios::trunc);
		if (!fos) {
			if(throw_exc)
				throw std::runtime_error("Cannot open config file " + fn + " for writing");
			else
				return false;
		}
		shv::chainpack::CponWriterOptions opts;
		opts.setIndent("  ");
		shv::chainpack::CponWriter wr(fos, opts);
		wr << config;
		return true;
	}
	else {
		if(throw_exc)
			throw std::runtime_error("Config must be RpcValue::Map type, config name: " + config_name);
		else
			return false;
	}
}

std::string BrokerApp::resolveMountPoint(const shv::chainpack::RpcValue::Map &device_opts)
{
	std::string mount_point;
	shv::chainpack::RpcValue device_id = device_opts.value(cp::Rpc::KEY_DEVICE_ID);
	if(device_id.isValid())
		mount_point = mountPointForDevice(device_id);
	if(mount_point.empty()) {
		mount_point = device_opts.value(cp::Rpc::KEY_MOUT_POINT).toString();
		std::vector<shv::core::StringView> path = shv::core::utils::ShvPath::split(mount_point);
		if(path.size() && !(path[0] == "test")) {
			shvWarning() << "Mount point can be explicitly specified to test/ dir only, dev id:" << device_id.toCpon();
			mount_point.clear();
		}
	}
	if(mount_point.empty()) {
		shvWarning() << "cannot find mount point for device id:" << device_id.toCpon();// << "connection id:" << connection_id;
	}
	return mount_point;
}

std::string BrokerApp::mountPointForDevice(const shv::chainpack::RpcValue &device_id)
{
	shv::chainpack::RpcValue fstab = fstabConfig();
	const std::string dev_id = device_id.toString();
	shv::chainpack::RpcValue mp_record = m_fstabConfig.toMap().value(dev_id);
	std::string mount_point;
	if(mp_record.isString())
		mount_point = mp_record.toString();
	else
		mount_point = mp_record.toMap().value(cp::Rpc::KEY_MOUT_POINT).toString();
	return mount_point;
}

static std::set<std::string> flatten_grant(const std::string &grant_name, const cp::RpcValue::Map &defined_grants)
{
	std::set<std::string> ret;
	cp::RpcValue gdef = defined_grants.value(grant_name);
	if(gdef.isValid()) {
		ret.insert(grant_name);
		const shv::chainpack::RpcValue::Map &gdefm = gdef.toMap();
		cp::RpcValue gg = gdefm.value("grants");
		for(auto g : gg.toList()) {
			const std::string &child_grant_name = g.toString();
			if(ret.count(child_grant_name)) {
				shvWarning() << "Cyclic reference in grants detected for grant name:" << child_grant_name;
			}
			else {
				std::set<std::string> gg = flatten_grant(child_grant_name, defined_grants);
				ret.insert(gg.begin(), gg.end());
			}
		}
	}
	return ret;
}

const std::set<std::string> &BrokerApp::userFlattenGrants(const std::string &user_name)
{
	if(m_userFlattenGrantsCache.count(user_name))
		return m_userFlattenGrantsCache.at(user_name);
	cp::RpcValue user_def = usersConfig().toMap().value(user_name);
	if(!user_def.isMap()) {
		static std::set<std::string> empty;
		return empty;
	}
	std::set<std::string> ret;
	for(auto gv : user_def.toMap().value("grants").toList()) {
		const std::string &gn = gv.toString();
		std::set<std::string> gg = flatten_grant(gn, grantsConfig().toMap());
		ret.insert(gg.begin(), gg.end());
	}
	m_userFlattenGrantsCache[user_name] = ret;
	return m_userFlattenGrantsCache.at(user_name);
}

std::string BrokerApp::primaryIPAddress(bool &is_public)
{
	if(cliOptions()->publicIP_isset())
		return cliOptions()->publicIP();
	QHostAddress ha = utils::Network::primaryPublicIPv4Address();
	if(!ha.isNull()) {
		is_public = true;
		return ha.toString().toStdString();
	}
	is_public = false;
	ha = utils::Network::primaryIPv4Address();
	if(!ha.isNull())
		return ha.toString().toStdString();
	return std::string();
}

void BrokerApp::propagateSubscriptionsToMasterBroker()
{
	logSubscriptionsD() << "Connected to main master broker, propagating client subscriptions.";
	rpc::MasterBrokerConnection *mbrconn = mainMasterBrokerConnection();
	if(!mbrconn)
		return;
	for(int id : clientConnectionIds()) {
		rpc::ClientBrokerConnection *conn = clientConnectionById(id);
		for (size_t i = 0; i < conn->subscriptionCount(); ++i) {
			const rpc::CommonRpcClientHandle::Subscription &subs = conn->subscriptionAt(i);
			if(shv::core::utils::ShvPath::isRelativePath(subs.absolutePath)) {
				logSubscriptionsD() << "client id:" << id << "propagating subscription for path:" << subs.absolutePath << "method:" << subs.method;
				mbrconn->callMethodSubscribe(subs.absolutePath, subs.method);
			}
		}
	}
}

static std::string join_string_set(const std::set<std::string> &ss, char sep)
{
	std::string ret;
	for(const std::string &s : ss) {
		if(ret.empty())
			ret = s;
		else
			ret = ret + sep + s;
	}
	return ret;
}

cp::Rpc::AccessGrant BrokerApp::accessGrantForRequest(rpc::CommonRpcClientHandle *conn, const std::string &rq_shv_path, const std::string &rq_grant)
{
	logAclD() << "accessGrantForShvPath user:" << conn->loggedUserName() << "shv path:" << rq_shv_path << "request grant:" << rq_grant;
#ifdef USE_SHV_PATHS_GRANTS_CACHE
	PathGrantCache *user_path_grants = m_userPathGrantCache.object(user_name);
	if(user_path_grants) {
		cp::Rpc::AccessGrant *pg = user_path_grants->object(shv_path);
		if(pg) {
			logAclD() << "\t cache hit:" << pg->grant << "weight:" << pg->weight;
			return *pg;
		}
	}
#endif
	cp::Rpc::AccessGrant ret;
	//shv::chainpack::RpcValue user = usersConfig().toMap().value(user_name);
	shv::iotqt::node::ShvNode::StringViewList shv_path_lst = shv::core::utils::ShvPath::split(rq_shv_path);
	const std::set<std::string> &user_flattent_grants = conn->isMasterBrokerConnection()
			? (rq_grant.empty()? std::set<std::string>{cp::Rpc::GRANT_MASTER_BROKER}: std::set<std::string>{rq_grant, cp::Rpc::GRANT_MASTER_BROKER}) // master broker has allways grant masterBroker
			: userFlattenGrants(conn->loggedUserName());
	for(const std::string &grant : user_flattent_grants) {
		logAclD() << "cheking grant:" << grant;
		cp::RpcValue grant_paths = pathsConfig().toMap().value(grant);
		for(const auto &kv : grant_paths.toMap()) {
			const std::string &p = kv.first;
			logAclD().nospace() << "\t checking if path: '" << rq_shv_path << "' match granted path: '" << p << "'";
			//logAclD().nospace() << "\t cheking if path: '" << shv_path << "' starts with granted path: '" << p << "' ,result:" << shv::core::String::startsWith(shv_path, p);
			shv::iotqt::node::ShvNode::StringViewList grant_path_pattern = shv::core::utils::ShvPath::split(p);
			/*
			for(size_t i=0; i<grant_path_pattern.size(); i++) {
				if(grant_path_pattern[i] == "{{rq.shvPath}}")
					grant_path_pattern[i] = rq_shv_path;
				else if(grant_path_pattern[i] == "{{rq.grant}}")
					grant_path_pattern[i] = rq_grant;
			}
			*/
			do {
				if(!shv::core::utils::ShvPath::matchWild(shv_path_lst, grant_path_pattern))
					break;
				const cp::RpcValue::Map &path_grant = kv.second.toMap();
				std::string gn = path_grant.value("grant").toString();
				int weight = 0;
				if(gn.empty()) {
					if(path_grant.value("forwardGrant").toBool()) {
						weight = path_grant.value("weight").toInt();
						gn = rq_grant;
					}
				}
				else {
					cp::RpcValue grant_def = grantsConfig().toMap().value(gn);
					weight = grant_def.toMap().value("weight").toInt();
				}
				logAclD() << "\t\t match, granted:" << gn << "weight:" << weight;
				if(weight > ret.weight) {
					logAclD() << "\t\t\t greatest weight!";
					ret = cp::Rpc::AccessGrant(gn, weight);
				}
			} while(false);
		}
	}
#ifdef USE_SHV_PATHS_GRANTS_CACHE
	if(!user_path_grants) {
		user_path_grants = new PathGrantCache();
		if(!m_userPathGrantCache.insert(user_name, user_path_grants))
			shvError() << "m_userPathGrantCache::insert() error";
	}
	if(user_path_grants)
		user_path_grants->insert(shv_path, new cp::Rpc::AccessGrant(ret));
#endif
	logAclD() << "\t resolved:" << ret.grant << "weight:" << ret.weight;
	logAccessD() << "access user:" << conn->loggedUserName()
				 << "shv_path:" << rq_shv_path
				 << "rq_grant:" << (rq_grant.empty()? "<none>": rq_grant)
				 << "grants:" << join_string_set(user_flattent_grants, ',')
				 << " => " << ret.grant << "weight:" << ret.weight;
	return ret;
}

void BrokerApp::onClientLogin(int connection_id)
{
	rpc::ClientBrokerConnection *conn = clientConnectionById(connection_id);
	if(!conn)
		SHV_EXCEPTION("Cannot find connection for ID: " + std::to_string(connection_id));
	//const shv::chainpack::RpcValue::Map &opts = conn->connectionOptions();

	shvInfo() << "Client login connection id:" << connection_id;// << "connection type:" << conn->connectionType();
	{
		std::string dir_mount_point = brokerClientDirPath(connection_id);
		{
			shv::iotqt::node::ShvNode *dir = m_nodesTree->cd(dir_mount_point);
			if(dir) {
				shvError() << "Client dir" << dir_mount_point << "exists already and will be deleted, this should never happen!";
				dir->setParentNode(nullptr);
				delete dir;
			}
		}
		shv::iotqt::node::ShvNode *clients_nd = m_nodesTree->mkdir(std::string(cp::Rpc::DIR_BROKER) + "/clients/");
		if(!clients_nd)
			SHV_EXCEPTION("Cannot create parent for ClientDirNode id: " + std::to_string(connection_id));
		ClientConnectionNode *client_dir_node = new ClientConnectionNode(connection_id, clients_nd);
		//shvWarning() << "path1:" << client_dir_node->shvPath();
		ClientShvNode *client_app_node = new ClientShvNode(conn, client_dir_node);
		client_app_node->setNodeId("app");
		//shvWarning() << "path2:" << client_app_node->shvPath();
		/*
		std::string app_mount_point = brokerClientAppPath(connection_id);
		shv::iotqt::node::ShvNode *curr_nd = m_deviceTree->cd(app_mount_point);
		ShvClientNode *curr_cli_nd = qobject_cast<ShvClientNode*>(curr_nd);
		if(curr_cli_nd) {
			shvWarning() << "The SHV client on" << app_mount_point << "exists already, this should never happen!";
			curr_cli_nd->connection()->abort();
			delete curr_cli_nd;
		}
		if(!m_deviceTree->mount(app_mount_point, cli_nd))
			SHV_EXCEPTION("Cannot mount connection to device tree, connection id: " + std::to_string(connection_id)
						  + " shv path: " + app_mount_point);
		*/
		// delete whole client tree, when client is destroyed
		connect(conn, &rpc::ClientBrokerConnection::destroyed, client_app_node->parentNode(), &ClientShvNode::deleteLater);

		conn->setParent(client_app_node);
		{
			std::string mount_point = client_dir_node->shvPath() + "/subscriptions";
			SubscriptionsNode *nd = new SubscriptionsNode(conn);
			if(!m_nodesTree->mount(mount_point, nd))
				SHV_EXCEPTION("Cannot mount connection subscription list to device tree, connection id: " + std::to_string(connection_id)
							  + " shv path: " + mount_point);
		}
	}

	if(conn->deviceOptions().isMap()) {
		const shv::chainpack::RpcValue::Map &device_opts = conn->deviceOptions().toMap();
		std::string mount_point = resolveMountPoint(device_opts);
		if(!mount_point.empty()) {
			ClientShvNode *cli_nd = qobject_cast<ClientShvNode*>(m_nodesTree->cd(mount_point));
			if(cli_nd) {
				/*
				shvWarning() << "The mount point" << mount_point << "exists already";
				if(cli_nd->connection()->deviceId() == device_id) {
					shvWarning() << "The same device ID will be remounted:" << device_id.toCpon();
					delete cli_nd;
				}
				*/
				cli_nd->addConnection(conn);
			}
			else {
				cli_nd = new ClientShvNode(conn);
				if(!m_nodesTree->mount(mount_point, cli_nd))
					SHV_EXCEPTION("Cannot mount connection to device tree, connection id: " + std::to_string(connection_id));
			}
			mount_point = cli_nd->shvPath();
			shvInfo() << "client connection id:" << conn->connectionId() << "device id:" << conn->deviceId().toCpon() << " mounted on:" << mount_point;
			/// overwrite client default mount point
			conn->addMountPoint(mount_point);
			connect(conn, &rpc::ClientBrokerConnection::destroyed, this, [this, connection_id, mount_point]() {
				shvInfo() << "server connection destroyed";
				this->onClientMountedChanged(connection_id, mount_point, false);
			});
			connect(cli_nd, &ClientShvNode::destroyed, cli_nd->parentNode(), &shv::iotqt::node::ShvNode::deleteIfEmptyWithParents, static_cast<Qt::ConnectionType>(Qt::UniqueConnection | Qt::QueuedConnection));
			QTimer::singleShot(0, [this, connection_id, mount_point]() {
				onClientMountedChanged(connection_id, mount_point, true);
			});
		}
	}
	//shvInfo() << m_deviceTree->dumpTree();
}

void BrokerApp::onConnectedToMasterBrokerChanged(int connection_id, bool is_connected)
{
	rpc::MasterBrokerConnection *conn = masterBrokerConnectionById(connection_id);
	if(!conn) {
		shvError() << "Cannot find master broker connection for ID:" << connection_id;
		return;
	}
	std::string masters_path = std::string(cp::Rpc::DIR_BROKER) + "/" + cp::Rpc::DIR_MASTERS;
	shv::iotqt::node::ShvNode *masters_nd = m_nodesTree->cd(masters_path);
	if(!masters_nd) {
		shvError() << ".broker/masters shv directory does not exist, this should never happen";
		return;
	}
	std::string connection_path = masters_path + '/' + conn->objectName().toStdString();
	shv::iotqt::node::ShvNode *node = m_nodesTree->cd(connection_path);
	if(is_connected) {
		shvInfo() << "Logged-in to master broker, connection id:" << connection_id;
		{
			if(node) {
				shvError() << "Master broker connection dir" << connection_path << "exists already and will be deleted, this should never happen!";
				node->setParentNode(nullptr);
				delete node;
			}
			MasterBrokerShvNode *mbnd = new MasterBrokerShvNode(masters_nd);
			mbnd->setNodeId(conn->objectName().toStdString());
			/*shv::iotqt::node::RpcValueMapNode *config_nd = */
			new shv::iotqt::node::RpcValueMapNode("config", conn->options(), mbnd);
		}
		if(conn == mainMasterBrokerConnection()) {
			/// propagate relative subscriptions
			propagateSubscriptionsToMasterBroker();
		}
	}
	else {
		shvInfo() << "Connection to master broker lost, connection id:" << connection_id;
		if(node) {
			node->setParentNode(nullptr);
			delete node;
		}
	}
}

void BrokerApp::onRpcDataReceived(int connection_id, shv::chainpack::Rpc::ProtocolType protocol_type, cp::RpcValue::MetaData &&meta, std::string &&data)
{
	cp::RpcMessage::setProtocolType(meta, protocol_type);
	if(cp::RpcMessage::isRegisterRevCallerIds(meta))
		cp::RpcMessage::pushRevCallerId(meta, connection_id);
	if(cp::RpcMessage::isRequest(meta)) {
		// prepare response for catch block
		// it cannot be constructed from meta, since meta is moved in the try block
		shv::chainpack::RpcResponse rsp = cp::RpcResponse::forRequest(meta);
		try {
			rpc::ClientBrokerConnection *client_connection = clientConnectionById(connection_id);
			rpc::MasterBrokerConnection *broker_connection = masterBrokerConnectionById(connection_id);
			rpc::CommonRpcClientHandle *connection_handle = client_connection;
			if(connection_handle == nullptr)
				connection_handle = broker_connection;
			std::string shv_path = cp::RpcMessage::shvPath(meta).toString();
			if(connection_handle) {
				if(shv::core::utils::ShvPath::isRelativePath(shv_path)) {
					rpc::MasterBrokerConnection *master_broker_conn = mainMasterBrokerConnection();
					if(client_connection) {
						shv_path = client_connection->resolveLocalPath(shv_path);
					}
					if(shv::core::utils::ShvPath::isRelativePath(shv_path)) {
						/// still relative path, it should be forwarded to mater broker
						if(master_broker_conn == nullptr)
							SHV_EXCEPTION("Cannot resolve relative path " + cp::RpcMessage::shvPath(meta).toString() + ", there is no master broker to forward the request.");
						cp::RpcMessage::setShvPath(meta, shv_path);
						cp::RpcMessage::pushCallerId(meta, connection_id);
						master_broker_conn->sendRawData(std::move(meta), std::move(data));
						return;
					}
					cp::RpcMessage::setShvPath(meta, shv_path);
				}
				if(client_connection) {
					// erase grant from client connections
					cp::AccessGrant ag = cp::RpcMessage::accessGrant(meta);
					if(ag.isValid() && !ag.isLogin()) {
						shvWarning() << "Client request with access grant specified not allowed, erasing:" << ag.toPrettyString();
						cp::RpcMessage::setAccessGrant(meta, cp::RpcValue());
					}
				}
				cp::Rpc::AccessGrant acg = accessGrantForRequest(connection_handle, shv_path, cp::RpcMessage::accessGrant(meta).toString());
				if(!acg.isValid())
					SHV_EXCEPTION("Acces to shv path '" + shv_path + "' not granted for user '" + connection_handle->loggedUserName() + "'");
				cp::RpcMessage::setAccessGrant(meta, acg.grant);
				cp::RpcMessage::pushCallerId(meta, connection_id);
				if(m_nodesTree->root()) {
					shvDebug() << "REQUEST conn id:" << connection_id << meta.toPrettyString();
					m_nodesTree->root()->handleRawRpcRequest(std::move(meta), std::move(data));
				}
				else {
					SHV_EXCEPTION("Device tree root node is NULL");
				}
			}
			else {
				SHV_EXCEPTION("Connection ID: " + std::to_string(connection_id) + " doesn't exist.");
			}
		}
		catch (std::exception &e) {
			rpc::ClientBrokerConnection *conn = clientConnectionById(connection_id);
			if(conn) {
				rsp.setError(cp::RpcResponse::Error::create(
								 cp::RpcResponse::Error::MethodCallException
								 , e.what()));
				conn->sendMessage(rsp);
			}
		}
	}
	else if(cp::RpcMessage::isResponse(meta)) {
		shvDebug() << "RESPONSE conn id:" << connection_id << meta.toPrettyString();
		shv::chainpack::RpcValue orig_caller_ids = cp::RpcMessage::callerIds(meta);
		cp::RpcValue::Int caller_id = cp::RpcMessage::popCallerId(meta);
		shvDebug() << "top caller id:" << caller_id;
		if(caller_id > 0) {
			cp::TunnelCtl tctl = cp::RpcMessage::tunnelCtl(meta);
			if(tctl.state() == cp::TunnelCtl::State::FindTunnelRequest) {
				logTunnelD() << "FindTunnelRequest received:" << meta.toPrettyString();
				cp::FindTunnelReqCtl find_tunnel_request(tctl);
				//uint32_t tctl_ipv4_addr = utils::Network::toIntIPv4Address(find_tunnel_request.host());
				//bool tctl_ipv4_addr_is_public = utils::Network::isPublicIPv4Address(tctl_ipv4_addr);
				bool last_broker = cp::RpcValueGenList(cp::RpcMessage::callerIds(meta)).empty();
				bool is_public_node;
				std::string server_ip = primaryIPAddress(is_public_node);
				if(is_public_node || (last_broker && find_tunnel_request.host().empty())) {
					find_tunnel_request.setHost(server_ip);
					find_tunnel_request.setPort(tcpServer()->serverPort());
					find_tunnel_request.setCallerIds(orig_caller_ids);
					find_tunnel_request.setSecret(m_tunnelSecretList.createSecret());
				}
				if(last_broker) {
					cp::FindTunnelRespCtl find_tunnel_response = cp::FindTunnelRespCtl::fromFindTunnelRequest(find_tunnel_request);
					cp::RpcMessage msg;
					msg.setRequestId(cp::RpcMessage::requestId(meta));
					// send response to FindTunnelRequest, remove top client id, since it is this connection id
					msg.setCallerIds(cp::RpcMessage::revCallerIds(meta));
					int top_connection_id = msg.popCallerId();
					if(top_connection_id != connection_id) {
						shvError() << "(top_connection_id != connection_id) this should never happen";
						return;
					}
					msg.setTunnelCtl(find_tunnel_response);
					rpc::CommonRpcClientHandle *cch = commonClientConnectionById(connection_id);
					if(cch) {
						logTunnelD() << "Sending FindTunnelResponse:" << msg.toPrettyString();
						cch->sendMessage(msg);
					}
					return;
				}
				cp::RpcMessage::setTunnelCtl(meta, find_tunnel_request);
				logTunnelD() << "Forwarding FindTunnelRequest:" << meta.toPrettyString();
			}
			rpc::CommonRpcClientHandle *cch = commonClientConnectionById(caller_id);
			if(cch) {
				cch->sendRawData(std::move(meta), std::move(data));
			}
			else {
				shvWarning() << "Got RPC response for not-exists connection, may be it was closed meanwhile. Connection id:" << caller_id;
			}
		}
		else {
			// broker messages like create master broker subscription
			shvDebug() << "Got RPC response without src connection specified, it should be this broker call like create master broker subscription, throwing message away." << meta.toPrettyString();
		}
	}
	else if(cp::RpcMessage::isSignal(meta)) {
		logSigResolveD() << "SIGNAL:" << meta.toPrettyString() << "from:" << connection_id;

		/// if signal arrives from client, its path must be prepended by client mount points
		/// if signal arrives from master broker, it can happen in case of relative subscription only,
		///   then its path must be left untouched
		std::vector<std::string> mount_points;

		rpc::ClientBrokerConnection *client_connection = clientConnectionById(connection_id);
		//rpc::MasterBrokerConnection *broker_connection = masterBrokerConnectionById(connection_id);
		//rpc::CommonRpcClientHandle *connection_handle = nullptr;
		if(client_connection) {
			//connection_handle = client_connection;
			mount_points = client_connection->mountPoints();
		}
		else {
			//connection_handle = broker_connection;
			mount_points = {std::string()};
		}
		const std::string sig_shv_path = cp::RpcMessage::shvPath(meta).toString();
		for(const std::string &mp : mount_points) {
			std::string full_shv_path = shv::core::utils::ShvPath::join(mp, sig_shv_path);
			if(full_shv_path.empty()) {
				shvError() << "SIGNAL with empty shv path received from master broker connection.";
			}
			else {
				//logSigResolveD() << client_connection->connectionId() << "forwarding signal to client on mount point:" << mp << "as:" << full_shv_path;
				cp::RpcMessage::setShvPath(meta, full_shv_path);
				bool sig_sent = sendNotifyToSubscribers(meta, data);
				if(!sig_sent && client_connection->isSlaveBrokerConnection()) {
					logSubscriptionsD() << "Rejecting unsubscribed signal, shv_path:" << full_shv_path << "method:" << cp::RpcMessage::method(meta).toString();
					cp::RpcRequest rq;
					rq.setRequestId(0);
					rq.setMethod(cp::Rpc::METH_REJECT_NOT_SUBSCRIBED)
							.setParams(cp::RpcValue::Map{
										   { cp::Rpc::PAR_PATH, sig_shv_path},
										   { cp::Rpc::PAR_METHOD, cp::RpcMessage::method(meta).toString()}})
							.setShvPath(cp::Rpc::DIR_BROKER_APP);
					client_connection->sendMessage(rq);
				}
			}
		}
	}
}

void BrokerApp::onRootNodeSendRpcMesage(const shv::chainpack::RpcMessage &msg)
{
	if(msg.isResponse()) {
		cp::RpcResponse resp(msg);
		shv::chainpack::RpcValue::Int connection_id = resp.popCallerId();
		rpc::CommonRpcClientHandle *conn = commonClientConnectionById(connection_id);
		if(conn)
			conn->sendMessage(resp);
		else
			shvError() << "Cannot find connection for ID:" << connection_id;
		return;
	}
	shvError() << "Send message not implemented.";// << msg.toCpon();
}

void BrokerApp::onClientMountedChanged(int client_id, const std::string &mount_point, bool is_mounted)
{
	sendNotifyToSubscribers(mount_point, cp::Rpc::SIG_MOUNTED_CHANGED, is_mounted);
	if(is_mounted) {
		//sendNotifyToSubscribers(connection_id, mount_point, cp::Rpc::NTF_CONNECTED, cp::RpcValue());
		rpc::ClientBrokerConnection *cc = clientConnectionById(client_id);
		if(cc && cc->isSlaveBrokerConnection()) {
			/// if slave broker is connected, forward subscriptions of connected clients
			for(rpc::CommonRpcClientHandle *ch : allClientConnections()) {
				if(ch->connectionId() == client_id)
					continue;
				for(size_t i=0; i<ch->subscriptionCount(); i++) {
					const rpc::CommonRpcClientHandle::Subscription &subs = ch->subscriptionAt(i);
					cc->propagateSubscriptionToSlaveBroker(subs);
				}
			}
		}
	}
	else {
		//this->sendNotifyToSubscribers(connection_id, mount_point, cp::Rpc::NTF_DISCONNECTED, cp::RpcValue());
	}
}

std::string BrokerApp::brokerClientDirPath(int client_id)
{
	return std::string(cp::Rpc::DIR_BROKER) + "/clients/" + std::to_string(client_id);
}

std::string BrokerApp::brokerClientAppPath(int client_id)
{
	return brokerClientDirPath(client_id) + "/app";
}

bool BrokerApp::sendNotifyToSubscribers(const shv::chainpack::RpcValue::MetaData &meta_data, const std::string &data)
{
	// send it to all clients for now
	bool subs_sent = false;
	for(rpc::CommonRpcClientHandle *conn : allClientConnections()) {
		if(conn->isConnectedAndLoggedIn()) {
			const cp::RpcValue shv_path = cp::RpcMessage::shvPath(meta_data);
			const cp::RpcValue method = cp::RpcMessage::method(meta_data);
			int subs_ix = conn->isSubscribed(shv_path.toString(), method.toString());
			if(subs_ix >= 0) {
				//shvDebug() << "\t broadcasting to connection id:" << id;
				const rpc::ClientBrokerConnection::Subscription &subs = conn->subscriptionAt((size_t)subs_ix);
				std::string new_path = conn->toSubscribedPath(subs, shv_path.toString());
				if(new_path == shv_path.toString()) {
					conn->sendRawData(meta_data, std::string(data));
				}
				else {
					shv::chainpack::RpcValue::MetaData md2(meta_data);
					cp::RpcMessage::setShvPath(md2, new_path);
					conn->sendRawData(md2, std::string(data));
				}
				subs_sent = true;
			}
		}
	}
	return subs_sent;
}

void BrokerApp::sendNotifyToSubscribers(const std::string &shv_path, const std::string &method, const shv::chainpack::RpcValue &params)
{
	//shvWarning() << shv_path << method << params.toPrettyString();
	cp::RpcSignal ntf;
	ntf.setShvPath(shv_path);
	ntf.setMethod(method);
	ntf.setParams(params);
	// send it to all clients for now
	for(rpc::CommonRpcClientHandle *conn : allClientConnections()) {
		if(conn->isConnectedAndLoggedIn()) {
			int subs_ix = conn->isSubscribed(shv_path, method);
			if(subs_ix >= 0) {
				//shvDebug() << "\t broadcasting to connection id:" << id;
				const rpc::ClientBrokerConnection::Subscription &subs = conn->subscriptionAt((size_t)subs_ix);
				std::string new_path = conn->toSubscribedPath(subs, shv_path);
				if(new_path != shv_path)
					ntf.setShvPath(new_path);
				conn->sendMessage(ntf);
			}
		}
	}
}

void BrokerApp::addSubscription(int client_id, const std::string &shv_path, const std::string &method)
{
	rpc::CommonRpcClientHandle *conn = commonClientConnectionById(client_id);
	if(!conn)
		SHV_EXCEPTION("Connot create subscription, client doesn't exist.");
	//logSubscriptionsD() << "addSubscription connection id:" << client_id << "path:" << path << "method:" << method;
	auto subs_ix = conn->addSubscription(shv_path, method);
	const rpc::CommonRpcClientHandle::Subscription &subs = conn->subscriptionAt(subs_ix);
	if(shv::core::utils::ShvPath::isRelativePath(subs.absolutePath)) {
		/// still relative path, should be propagated to the master broker
		rpc::MasterBrokerConnection *mbrconn = mainMasterBrokerConnection();
		if(mbrconn) {
			mbrconn->callMethodSubscribe(subs.absolutePath, subs.method);
		}
		else {
			shvError() << "Cannot propagate relative path subscription, without master broker connected:" << subs.absolutePath;
		}
	}
	else {
		/// check slave broker connections
		/// whether this subsciption should be propagated to them
		for (int connection_id : clientConnectionIds()) {
			rpc::ClientBrokerConnection *conn = clientConnectionById(connection_id);
			if(conn->isSlaveBrokerConnection()) {
				conn->propagateSubscriptionToSlaveBroker(subs);
			}
		}
	}
}

bool BrokerApp::removeSubscription(int client_id, const std::string &shv_path, const std::string &method)
{
	rpc::CommonRpcClientHandle *conn = commonClientConnectionById(client_id);
	if(!conn)
		SHV_EXCEPTION("Connot remove subscription, client doesn't exist.");
	//logSubscriptionsD() << "addSubscription connection id:" << client_id << "path:" << path << "method:" << method;
	return conn->removeSubscription(shv_path, method);
}

bool BrokerApp::rejectNotSubscribedSignal(int client_id, const std::string &path, const std::string &method)
{
	logSubscriptionsD() << "signal rejected, shv_path:" << path << "method:" << method;
	rpc::MasterBrokerConnection *conn = masterBrokerConnectionById(client_id);
	if(conn) {
		return conn->rejectNotSubscribedSignal(conn->masterExportedToLocalPath(path), method);
	}
	return false;
}

void BrokerApp::createMasterBrokerConnections()
{
	if(!cliOptions()->isMasterBrokersEnabled())
		return;
	shvInfo() << "Creating master broker connections";
	shv::chainpack::RpcValue masters = cliOptions()->masterBrokersConnections();
	for(const auto &kv : masters.toMap()) {
		const cp::RpcValue::Map &opts = kv.second.toMap();
		if(opts.value("enabled").toBool() == false)
			continue;
		shvInfo() << "creating master broker connection:" << kv.first;
		rpc::MasterBrokerConnection *bc = new rpc::MasterBrokerConnection(this);
		bc->setObjectName(QString::fromStdString(kv.first));
		int id = bc->connectionId();
		connect(bc, &rpc::MasterBrokerConnection::brokerConnectedChanged, this, [id, this](bool is_connected) {
			this->onConnectedToMasterBrokerChanged(id, is_connected);
		});
		bc->setOptions(opts);
		bc->open();
	}
}

QList<rpc::MasterBrokerConnection *> BrokerApp::masterBrokerConnections() const
{
	return findChildren<rpc::MasterBrokerConnection*>(QString(), Qt::FindDirectChildrenOnly);
}

rpc::MasterBrokerConnection *BrokerApp::masterBrokerConnectionById(int connection_id)
{
	for(rpc::MasterBrokerConnection *bc : masterBrokerConnections()) {
		if(bc->connectionId() == connection_id)
			return bc;
	}
	return nullptr;
}

std::vector<rpc::CommonRpcClientHandle *> BrokerApp::allClientConnections()
{
	std::vector<rpc::CommonRpcClientHandle *> ret;
	for (int i : clientConnectionIds())
		ret.push_back(clientConnectionById(i));
	QList<rpc::MasterBrokerConnection *> mbc = masterBrokerConnections();
	std::copy(mbc.begin(), mbc.end(), std::back_inserter(ret));
	return ret;
}

rpc::CommonRpcClientHandle *BrokerApp::commonClientConnectionById(int connection_id)
{
	rpc::CommonRpcClientHandle *ret = clientConnectionById(connection_id);
	if(ret)
		return ret;
	ret = masterBrokerConnectionById(connection_id);
	return ret;
}


