#include "brokerapp.h"
#include "clientshvnode.h"
#include "brokernode.h"
#include "subscriptionsnode.h"
#include "brokerconfigfilenode.h"
#include "clientconnectionnode.h"
#include "rpc/tcpserver.h"
#include "rpc/serverconnection.h"
#include "rpc/masterbrokerconnection.h"

#include <shv/iotqt/node/shvnode.h>
#include <shv/iotqt/node/shvnodetree.h>
#include <shv/iotqt/rpc/brokerconnection.h>
#include <shv/iotqt/utils/shvpath.h>

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

#define logAclD() nCDebug("Acl")
#define logSubscriptionsD() nCDebug("Subs")

int BrokerApp::m_sigTermFd[2];
#endif

namespace cp = shv::chainpack;

class ClientsNode : public shv::iotqt::node::MethodsTableNode
{
	using Super = shv::iotqt::node::MethodsTableNode;
public:
	ClientsNode(shv::iotqt::node::ShvNode *parent = nullptr)
		: Super("clients", m_metaMethods, parent)
		, m_metaMethods{
				  {cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam},
				  {cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, 0, cp::Rpc::GRANT_CONFIG},
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
		: Super("masters", m_metaMethods, parent)
		, m_metaMethods{
				  {cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam},
				  {cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, 0, cp::Rpc::GRANT_CONFIG},
		}
	{ }
private:
	std::vector<cp::MetaMethod> m_metaMethods;
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

	cp::RpcMessage::setMetaTypeExplicit(cli_opts->isMetaTypeExplicit());

	//connect(this, &BrokerApp::sqlServerConnected, this, &BrokerApp::onSqlServerConnected);
	/*
	m_sqlConnectionWatchDog = new QTimer(this);
	connect(m_sqlConnectionWatchDog, SIGNAL(timeout()), this, SLOT(reconnectSqlServer()));
	m_sqlConnectionWatchDog->start(SQL_RECONNECT_INTERVAL);
	*/
	m_deviceTree = new shv::iotqt::node::ShvNodeTree(this);
	connect(m_deviceTree->root(), &shv::iotqt::node::ShvRootNode::sendRpcMesage, this, &BrokerApp::onRootNodeSendRpcMesage);
	BrokerNode *bn = new BrokerNode();
	m_deviceTree->mount(cp::Rpc::DIR_BROKER_APP, bn);
	m_deviceTree->mount(std::string(cp::Rpc::DIR_BROKER) + "/clients", new ClientsNode());
	m_deviceTree->mount(std::string(cp::Rpc::DIR_BROKER) + "/masters", new MasterBrokersNode());
	m_deviceTree->mount(std::string(cp::Rpc::DIR_BROKER) + "/etc/acl", new EtcAclNode());

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

		if(sigaction(sig_num, &sa, 0) > 0)
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
	unsigned char a = sig_number;
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

rpc::ServerConnection *BrokerApp::clientById(int client_id)
{
	return tcpServer()->connectionById(client_id);
}

void BrokerApp::reloadConfig()
{
	shvInfo() << "Reloading config";
	reloadAcl();
	remountDevices();
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

void BrokerApp::lazyInit()
{
	//reconnectSqlServer();
	startTcpServer();
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
	std::ostringstream out;
	{
		cp::CponWriterOptions opts;
		opts.setTranslateIds(cliOptions()->isMetaTypeExplicit());
		cp::CponWriter wr(out, opts);
		wr << rpc_val;
	}
	return out.str();
}

void BrokerApp::remountDevices()
{
	shvInfo() << "Remounting devices by dropping their connection";
	m_fstabConfig = cp::RpcValue();
	for(int conn_id : tcpServer()->connectionIds()) {
		rpc::ServerConnection *conn = tcpServer()->connectionById(conn_id);
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
	rd.read(rv, throw_exc? nullptr: &err);
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
		std::vector<shv::core::StringView> path = shv::iotqt::node::ShvNode::splitShvPath(mount_point);
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
	std::string mount_point = m_fstabConfig.toMap().value(dev_id).toString();
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

cp::Rpc::AccessGrant BrokerApp::accessGrantForRequest(const std::string &user_name, const std::string &rq_shv_path, const std::string &rq_grant)
{
	logAclD() << "accessGrantForShvPath user:" << user_name << "shv path:" << rq_shv_path << "request grant:" << rq_grant;
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
	shv::iotqt::node::ShvNode::StringViewList shv_path_lst = shv::iotqt::node::ShvNode::splitShvPath(rq_shv_path);
	for(const std::string &grant : userFlattenGrants(user_name)) {
		logAclD() << "cheking grant:" << grant;
		cp::RpcValue grant_paths = pathsConfig().toMap().value(grant);
		for(const auto &kv : grant_paths.toMap()) {
			const std::string &p = kv.first;
			logAclD().nospace() << "\t checking if path: '" << rq_shv_path << "' starts with granted path: '" << p;
			//logAclD().nospace() << "\t cheking if path: '" << shv_path << "' starts with granted path: '" << p << "' ,result:" << shv::core::String::startsWith(shv_path, p);
			shv::iotqt::node::ShvNode::StringViewList grant_path_pattern = shv::iotqt::node::ShvNode::splitShvPath(p);
			for(size_t i=0; i<grant_path_pattern.size(); i++) {
				if(grant_path_pattern[i] == "{{rq.shvPath}}")
					grant_path_pattern[i] = rq_shv_path;
				else if(grant_path_pattern[i] == "{{rq.grant}}")
					grant_path_pattern[i] = rq_grant;
			}
			do {
				if(!shv::iotqt::utils::ShvPath::matchWild(shv_path_lst, grant_path_pattern))
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
	return ret;
}

void BrokerApp::onClientLogin(int connection_id)
{
	rpc::ServerConnection *conn = tcpServer()->connectionById(connection_id);
	if(!conn)
		SHV_EXCEPTION("Cannot find connection for ID: " + std::to_string(connection_id));
	//const shv::chainpack::RpcValue::Map &opts = conn->connectionOptions();

	shvInfo() << "Client login connection id:" << connection_id;// << "connection type:" << conn->connectionType();
	{
		std::string dir_mount_point = brokerClientDirPath(connection_id);
		{
			shv::iotqt::node::ShvNode *dir = m_deviceTree->cd(dir_mount_point);
			if(dir) {
				shvError() << "Client dir" << dir_mount_point << "exists already and will be deleted, this should never happen!";
				dir->setParentNode(nullptr);
				delete dir;
			}
		}
		shv::iotqt::node::ShvNode *clients_nd = m_deviceTree->mkdir(std::string(cp::Rpc::DIR_BROKER) + "/clients/");
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
		connect(conn, &rpc::ServerConnection::destroyed, client_app_node->parentNode(), &ClientShvNode::deleteLater);
		//this->sendNotifyToSubscribers(connection_id, mount_point, cp::Rpc::NTF_CONNECTED_CHANGED, true);
		//connect(conn, &rpc::ServerConnection::destroyed, this, [this, connection_id, mount_point]() {
		//	this->sendNotifyToSubscribers(connection_id, mount_point, cp::Rpc::NTF_CONNECTED_CHANGED, false);
		//});
		conn->setParent(client_app_node);
		{
			std::string mount_point = client_dir_node->shvPath() + "/subscriptions";
			SubscriptionsNode *nd = new SubscriptionsNode(conn);
			if(!m_deviceTree->mount(mount_point, nd))
				SHV_EXCEPTION("Cannot mount connection subscription list to device tree, connection id: " + std::to_string(connection_id)
							  + " shv path: " + mount_point);
		}
	}

	if(!conn->deviceOptions().empty()) {
		const shv::chainpack::RpcValue::Map &device_opts = conn->deviceOptions();
		std::string mount_point = resolveMountPoint(device_opts);
		if(!mount_point.empty()) {
			ClientShvNode *cli_nd = qobject_cast<ClientShvNode*>(m_deviceTree->cd(mount_point));
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
				if(!m_deviceTree->mount(mount_point, cli_nd))
					SHV_EXCEPTION("Cannot mount connection to device tree, connection id: " + std::to_string(connection_id));
			}
			mount_point = cli_nd->shvPath();
			shvInfo() << "client connection id:" << conn->connectionId() << "device id:" << conn->deviceId().toCpon() << " mounted on:" << mount_point;
			/// overwrite client default mount point
			conn->addMountPoint(mount_point);
			connect(conn, &rpc::ServerConnection::destroyed, this, [this, connection_id, mount_point]() {
				shvInfo() << "server connection destroyed";
				//this->sendNotifyToSubscribers(connection_id, mount_point, cp::Rpc::NTF_DISCONNECTED, cp::RpcValue());
				this->sendNotifyToSubscribers(connection_id, mount_point, cp::Rpc::SIG_MOUNTED_CHANGED, false);
			});
			connect(cli_nd, &ClientShvNode::destroyed, cli_nd->parentNode(), &shv::iotqt::node::ShvNode::deleteIfEmptyWithParents, static_cast<Qt::ConnectionType>(Qt::UniqueConnection | Qt::QueuedConnection));
			//sendNotifyToSubscribers(connection_id, mount_point, cp::Rpc::NTF_CONNECTED, cp::RpcValue());
			sendNotifyToSubscribers(connection_id, mount_point, cp::Rpc::SIG_MOUNTED_CHANGED, true);
		}
	}
	//shvInfo() << m_deviceTree->dumpTree();
}

void BrokerApp::onLoggedinToMasterBroker(int connection_id)
{
	rpc::MasterBrokerConnection *conn = masterBrokerConnectionById(connection_id);
	if(!conn) {
		shvError() << "Cannot find master broker connection for ID:" << connection_id;
		return;
	}

	shvInfo() << "Logged-in to master broker, connection id:" << connection_id;
	{
		std::string dir_mount_point = std::string(cp::Rpc::DIR_BROKER) + "/masters/" + std::to_string(connection_id);
		{
			shv::iotqt::node::ShvNode *dir = m_deviceTree->cd(dir_mount_point);
			if(dir) {
				shvError() << "Master broker connection dir" << dir_mount_point << "exists already and will be deleted, this should never happen!";
				dir->setParentNode(nullptr);
				delete dir;
			}
		}
		shv::iotqt::node::ShvNode *masters_nd = m_deviceTree->mkdir(std::string(cp::Rpc::DIR_BROKER) + "/masters/");
		if(!masters_nd) {
			shvError() << "Cannot create .broker/masters shv directory";
			return;
		}
		MasterBrokerShvNode *mb_node = new MasterBrokerShvNode(conn, masters_nd);
		conn->setParent(mb_node);
	}

	if(!conn->deviceOptions().empty()) {
		const shv::chainpack::RpcValue::Map &device_opts = conn->deviceOptions();
		std::string mount_point = resolveMountPoint(device_opts);
		if(!mount_point.empty()) {
			MasterBrokerShvNode *mb_nd = qobject_cast<MasterBrokerShvNode*>(m_deviceTree->cd(mount_point));
			if(mb_nd) {
				mb_nd->addConnection(conn);
			}
			else {
				mb_nd = new MasterBrokerShvNode(conn);
				if(!m_deviceTree->mount(mount_point, mb_nd))
					SHV_EXCEPTION("Cannot mount connection to device tree, connection id: " + std::to_string(connection_id));
			}
			mount_point = mb_nd->shvPath();
			shvInfo() << "master broker connection id:" << conn->connectionId() << "device id:" << conn->deviceId().toCpon() << " mounted on:" << mount_point;
			/// overwrite client default mount point
			conn->addMountPoint(mount_point);
			connect(conn, &rpc::MasterBrokerConnection::destroyed, this, [this, connection_id, mount_point]() {
				shvInfo() << "master broker connection destroyed, id:" << connection_id;
				this->sendNotifyToSubscribers(connection_id, mount_point, cp::Rpc::SIG_MOUNTED_CHANGED, false);
			});
			connect(mb_nd, &MasterBrokerShvNode::destroyed, mb_nd->parentNode(), &shv::iotqt::node::ShvNode::deleteIfEmptyWithParents, static_cast<Qt::ConnectionType>(Qt::UniqueConnection | Qt::QueuedConnection));
			//sendNotifyToSubscribers(connection_id, mount_point, cp::Rpc::NTF_CONNECTED, cp::RpcValue());
			sendNotifyToSubscribers(connection_id, mount_point, cp::Rpc::SIG_MOUNTED_CHANGED, true);
		}
	}
}

void BrokerApp::onRpcDataReceived(int connection_id, shv::chainpack::Rpc::ProtocolType protocol_type, cp::RpcValue::MetaData &&meta, std::string &&data)
{
	cp::RpcMessage::setProtocolType(meta, protocol_type);
	if(cp::RpcMessage::isRegisterRevCallerIds(meta))
		cp::RpcMessage::pushRevCallerId(meta, connection_id);
	if(cp::RpcMessage::isRequest(meta)) {
		try {
			rpc::CommonRpcClientHandle *cch = commonClientConnectionById(connection_id);
			std::string shv_path = cp::RpcMessage::shvPath(meta).toString();
			if(cch) {
				if(rpc::ServerConnection::Subscription::isRelativePath(shv_path)) {
					const std::vector<std::string> &mps = cch->mountPoints();
					if(mps.empty())
						SHV_EXCEPTION("Cannot call method on relative path for unmounted device.");
					if(mps.size() > 1)
						SHV_EXCEPTION("Cannot call method on relative path for device mounted to more than single node.");
					shv_path = rpc::ServerConnection::Subscription::toAbsolutePath(mps[0], shv_path);
					cp::RpcMessage::setShvPath(meta, shv_path);
				}
				cp::Rpc::AccessGrant acg = accessGrantForRequest(cch->userName(), shv_path, cp::RpcMessage::accessGrant(meta).toString());
				if(!acg.isValid())
					SHV_EXCEPTION("Acces to shv path '" + shv_path + "' not granted for user " + cch->userName());
				cp::RpcMessage::setAccessGrant(meta, acg.grant);
				cp::RpcMessage::pushCallerId(meta, connection_id);
				if(m_deviceTree->root()) {
					shvDebug() << "REQUEST conn id:" << connection_id << meta.toStdString();
					m_deviceTree->root()->handleRawRpcRequest(std::move(meta), std::move(data));
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
			rpc::ServerConnection *conn = tcpServer()->connectionById(connection_id);
			if(conn) {
				shv::chainpack::RpcResponse rsp = cp::RpcResponse::forRequest(meta);
				rsp.setError(cp::RpcResponse::Error::create(
								 cp::RpcResponse::Error::MethodCallException
								 , e.what()));
				conn->sendMessage(rsp);
			}
		}
	}
	else if(cp::RpcMessage::isResponse(meta)) {
		shvDebug() << "RESPONSE conn id:" << connection_id << meta.toStdString();
		cp::RpcValue::Int caller_id = cp::RpcMessage::popCallerId(meta);
		if(caller_id > 0) {
			rpc::CommonRpcClientHandle *cch = commonClientConnectionById(caller_id);
			if(cch) {
				cch->sendRawData(std::move(meta), std::move(data));
			}
			else {
				shvWarning() << "Got RPC response for not-exists connection, may be it was closed meanwhile. Connection id:" << caller_id;
			}
		}
		else {
			shvError() << "Got RPC response without src connection specified, throwing message away." << meta.toStdString();
		}
	}
	else if(cp::RpcMessage::isSignal(meta)) {
		shvDebug() << "NOTIFY:" << meta.toStdString() << "from:" << connection_id;
		rpc::CommonRpcClientHandle *conn = commonClientConnectionById(connection_id);
		if(conn) {
			for(const std::string &mp : conn->mountPoints()) {
				std::string full_shv_path = shv::core::Utils::joinPath(mp, cp::RpcMessage::shvPath(meta).toString());
				if(!full_shv_path.empty()) {
					cp::RpcMessage::setShvPath(meta, full_shv_path);
					bool sig_sent = sendNotifyToSubscribers(connection_id, meta, data);
					if(!sig_sent && conn->isSlaveBrokerConnection()) {
						cp::RpcRequest rq;
						rq.setMethod(cp::Rpc::METH_REJECT_NOT_SUBSCRIBED)
								.setParams(cp::RpcValue::Map{
											   { cp::Rpc::PAR_PATH, full_shv_path},
											   { cp::Rpc::PAR_METHOD, cp::RpcMessage::method(meta).toString()}})
								.setShvPath(cp::Rpc::DIR_BROKER_APP);
						conn->sendMessage(rq);
					}
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

std::string BrokerApp::brokerClientDirPath(int client_id)
{
	return std::string(cp::Rpc::DIR_BROKER) + "/clients/" + std::to_string(client_id);
}

std::string BrokerApp::brokerClientAppPath(int client_id)
{
	return brokerClientDirPath(client_id) + "/app";
}

bool BrokerApp::sendNotifyToSubscribers(int sender_connection_id, const shv::chainpack::RpcValue::MetaData &meta_data, const std::string &data)
{
	// send it to all clients for now
	for(rpc::CommonRpcClientHandle *conn : allClientConnections()) {
		int id = conn->connectionId();
		if(id == sender_connection_id)
			continue;
		if(conn->isConnectedAndLoggedIn()) {
			const cp::RpcValue shv_path = cp::RpcMessage::shvPath(meta_data);
			const cp::RpcValue method = cp::RpcMessage::method(meta_data);
			int subs_ix = conn->isSubscribed(shv_path.toString(), method.toString());
			if(subs_ix >= 0) {
				//shvDebug() << "\t broadcasting to connection id:" << id;
				const rpc::ServerConnection::Subscription &subs = conn->subscriptionAt((size_t)subs_ix);
				std::string new_path = conn->toSubscribedPath(subs, shv_path.toString());
				if(new_path == shv_path.toString()) {
					conn->sendRawData(meta_data, std::string(data));
				}
				else {
					shv::chainpack::RpcValue::MetaData md2(meta_data);
					cp::RpcMessage::setShvPath(md2, new_path);
					conn->sendRawData(md2, std::string(data));
				}
				return true;
			}
		}
	}
	return false;
}

void BrokerApp::sendNotifyToSubscribers(int sender_connection_id, const std::string &shv_path, const std::string &method, const shv::chainpack::RpcValue &params)
{
	cp::RpcSignal ntf;
	ntf.setShvPath(shv_path);
	ntf.setMethod(method);
	ntf.setParams(params);
	// send it to all clients for now
	for(rpc::CommonRpcClientHandle *conn : allClientConnections()) {
		int id = conn->connectionId();
		if(id == sender_connection_id)
			continue;
		if(conn->isConnectedAndLoggedIn()) {
			int subs_ix = conn->isSubscribed(shv_path, method);
			if(subs_ix >= 0) {
				//shvDebug() << "\t broadcasting to connection id:" << id;
				const rpc::ServerConnection::Subscription &subs = conn->subscriptionAt((size_t)subs_ix);
				std::string new_path = conn->toSubscribedPath(subs, shv_path);
				if(new_path != shv_path)
					ntf.setShvPath(new_path);
				conn->sendMessage(ntf);
			}
		}
	}
}

void BrokerApp::addSubscription(int client_id, const std::string &path, const std::string &method)
{
	auto *conn = commonClientConnectionById(client_id);
	if(!conn)
		SHV_EXCEPTION("Connot create subscription, client doesn't exist.");
	//logSubscriptionsD() << "addSubscription connection id:" << client_id << "path:" << path << "method:" << method;
	conn->addSubscription(path, method);
	// check slave broker connections
	// whether this subsciption should be propagated to them
	for (int connection_id : tcpServer()->connectionIds()) {
		rpc::ServerConnection *conn = tcpServer()->connectionById(connection_id);
		if(conn->isSlaveBrokerConnection()) {
			for(const std::string &mpoint : conn->mountPoints()) {
				shv::core::StringViewList mntlst = shv::iotqt::node::ShvNode::splitShvPath(mpoint);
				shv::core::StringViewList subslst = shv::iotqt::node::ShvNode::splitShvPath(path);
				if(subslst.startsWith(mntlst)) {
					conn->callMethodSubscribe(subslst.mid(mntlst.size()).join('/'), method);
				}
				else if(mntlst.startsWith(subslst)) {
					conn->callMethodSubscribe(std::string(), method);
				}
			}
		}
	}
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
	for (int i : tcpServer()->connectionIds())
		ret.push_back(tcpServer()->connectionById(i));
	QList<rpc::MasterBrokerConnection *> mbc = masterBrokerConnections();
	std::copy(mbc.begin(), mbc.end(), std::back_inserter(ret));
	return ret;
}

rpc::CommonRpcClientHandle *BrokerApp::commonClientConnectionById(int connection_id)
{
	rpc::CommonRpcClientHandle *ret = tcpServer()->connectionById(connection_id);
	if(ret)
		return ret;
	ret = masterBrokerConnectionById(connection_id);
	return ret;
}

