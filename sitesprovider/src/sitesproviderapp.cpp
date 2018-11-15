#include "sitesproviderapp.h"
#include "appclioptions.h"

#include <shv/iotqt/rpc/deviceconnection.h>
#include <shv/iotqt/node/shvnodetree.h>
#include <shv/coreqt/log.h>
#include <shv/coreqt/exception.h>
#include <shv/chainpack/metamethod.h>
#include <shv/core/stringview.h>

#include <QTimer>
#include <QFileInfo>
#include <QDir>
#include <QJsonDocument>

#ifdef Q_OS_UNIX
#include <unistd.h>
#include <errno.h>
#include <string.h>
#endif

namespace cp = shv::chainpack;

static const char METH_GET_SITES[] = "getSites";
static const char METH_GET_CONFIG[] = "getConfig";
static const char METH_SAVE_CONFIG[] = "saveConfig";

static std::vector<cp::MetaMethod> root_meta_methods {
	{ cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, false },
	{ cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, false },
	{ cp::Rpc::METH_APP_NAME, cp::MetaMethod::Signature::RetVoid, false },
	{ cp::Rpc::METH_DEVICE_ID, cp::MetaMethod::Signature::RetVoid, false },
	{ METH_GET_SITES, cp::MetaMethod::Signature::RetVoid, false },
	{ METH_GET_CONFIG, cp::MetaMethod::Signature::RetParam, false },
	{ METH_SAVE_CONFIG, cp::MetaMethod::Signature::VoidParam, false },
};

static std::vector<cp::MetaMethod> empty_leaf_meta_methods {
	{ cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, false },
	{ cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, false },
	{ METH_GET_CONFIG, cp::MetaMethod::Signature::RetVoid, false },
	{ METH_SAVE_CONFIG, cp::MetaMethod::Signature::VoidParam, false },
};

static std::vector<cp::MetaMethod> meta_leaf_meta_methods {
	{ cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, false },
	{ cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, false },
};

static std::vector<cp::MetaMethod> config_leaf_meta_methods {
	{ cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, false },
	{ cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, false },
	{ cp::Rpc::METH_GET, cp::MetaMethod::Signature::RetVoid, false },
};

static std::vector<cp::MetaMethod> data_leaf_meta_methods {
	{ cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, false },
	{ cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, false },
	{ cp::Rpc::METH_GET, cp::MetaMethod::Signature::RetVoid, false },
};


AppRootNode::AppRootNode(QObject *parent)
	: Super(parent)
{
	connect(this, &AppRootNode::methodFinished, this, &AppRootNode::sendRpcMesage);
}

size_t AppRootNode::methodCount(const StringViewList &shv_path)
{
	return metaMethods(shv_path).size();
}

const cp::MetaMethod *AppRootNode::metaMethod(const StringViewList &shv_path, size_t ix)
{
	const std::vector<cp::MetaMethod> &meta_methods = metaMethods(shv_path);
	if(meta_methods.size() <= ix)
		SHV_EXCEPTION("Invalid method index: " + std::to_string(ix) + " of: " + std::to_string(meta_methods.size()));
	return &(meta_methods[ix]);
}

shv::chainpack::RpcValue AppRootNode::processRpcRequest(const cp::RpcRequest &rq)
{
	shv::core::StringView::StringViewList shv_path = shv::core::StringView(rq.shvPath().toString()).split('/');
	const cp::RpcValue::String method = rq.method().toString();

	auto async_callback = [this, rq](const cp::RpcValue &result) mutable {
		cp::RpcResponse resp = cp::RpcResponse::forRequest(rq);
		resp.setResult(result);
		Q_EMIT methodFinished(resp);
	};

	if (method == cp::Rpc::METH_APP_NAME) {
		return QCoreApplication::instance()->applicationName().toStdString();
	}
	else if (method == METH_GET_SITES) {
		return SitesProviderApp::instance()->getSites(async_callback);
	}
	else if (method == METH_GET_CONFIG) {
		if (shv_path.empty()) {
			return SitesProviderApp::instance()->getConfig(rq.params());
		}
		else {
			cp::RpcValue::List params;
			params.push_back(rq.shvPath());
			return SitesProviderApp::instance()->getConfig(params);
		}
	}
	else if (method == METH_SAVE_CONFIG) {
		if (shv_path.empty()) {
			SitesProviderApp::instance()->saveConfig(rq.params());
		}
		else {
			cp::RpcValue::List params;
			params.push_back(rq.shvPath());
			params.push_back(rq.params()[0]);
			SitesProviderApp::instance()->saveConfig(params);
		}
		return cp::RpcValue();
	}
	else if (method == cp::Rpc::METH_GET) {
		return SitesProviderApp::instance()->get(shv_path, async_callback);
	}
	else if (method == cp::Rpc::METH_DEVICE_ID) {
		SitesProviderApp *app = SitesProviderApp::instance();
		cp::RpcValue::Map opts = app->rpcConnection()->connectionOptions().toMap();;
		cp::RpcValue::Map dev = opts.value(cp::Rpc::KEY_DEVICE).toMap();
		return dev.value(cp::Rpc::KEY_DEVICE_ID).toString();
	}
	else if (method == cp::Rpc::METH_LS) {
		return SitesProviderApp::instance()->ls(shv_path, async_callback);
	}
	else if (method == cp::Rpc::METH_DIR) {
		return SitesProviderApp::instance()->dir(shv_path, rq.params(), async_callback);
	}
	return cp::RpcValue();
}

void AppRootNode::handleRpcRequest(const shv::chainpack::RpcRequest &rq)
{
	SitesProviderApp *app = SitesProviderApp::instance();
	if (!app->checkSites()) {
		app->downloadSites([this, rq]() {
			Super::handleRpcRequest(rq);
		});
	}
	else {
		Super::handleRpcRequest(rq);
	}
}

const std::vector<shv::chainpack::MetaMethod> &AppRootNode::metaMethods(const shv::iotqt::node::ShvNode::StringViewList &shv_path)
{
	if (shv_path.empty()) {
		return root_meta_methods;
	}
	else if (shv_path[shv_path.size() - 1] == "_meta") {
		return meta_leaf_meta_methods;
	}
	else if (shv_path[shv_path.size() - 1] == "_config") {
		return config_leaf_meta_methods;
	}
	else if (SitesProviderApp::instance()->hasData(shv_path)) {
		return data_leaf_meta_methods;
	}
	else {
		return empty_leaf_meta_methods;
	}
}

SitesProviderApp::SitesProviderApp(int &argc, char **argv, AppCliOptions* cli_opts)
	: Super(argc, argv)
	, m_cliOptions(cli_opts)
{
#ifdef Q_OS_UNIX
	if (setpgid(0, 0) != 0) {
		shvError() << "Error set process group ID:" << errno << ::strerror(errno);
	}
#endif
	cp::RpcMessage::setMetaTypeExplicit(cli_opts->isMetaTypeExplicit());
	m_rpcConnection = new shv::iotqt::rpc::DeviceConnection(this);

	if (!cli_opts->user_isset()) {
		cli_opts->setUser("iot");
	}

	if (!cli_opts->password_isset()) {
		cli_opts->setPassword("lub42DUB");
	}
	if (!cli_opts->deviceId_isset()) {
		cli_opts->setDeviceId("sitesprovider");
	}
	m_rpcConnection->setCliOptions(cli_opts);

	connect(m_rpcConnection, &shv::iotqt::rpc::ClientConnection::rpcMessageReceived, this, &SitesProviderApp::onRpcMessageReceived);

	m_rootNode = new AppRootNode();
	m_shvTree = new shv::iotqt::node::ShvNodeTree(m_rootNode, this);

	connect(m_shvTree->root(), &shv::iotqt::node::ShvRootNode::sendRpcMesage, m_rpcConnection, &shv::iotqt::rpc::ClientConnection::sendMessage);

	QTimer::singleShot(0, m_rpcConnection, &shv::iotqt::rpc::ClientConnection::open);
}

SitesProviderApp::~SitesProviderApp()
{
	shvInfo() << "destroying shv file provider application";
}

SitesProviderApp *SitesProviderApp::instance()
{
	return qobject_cast<SitesProviderApp *>(QCoreApplication::instance());
}

cp::RpcValue SitesProviderApp::getSites(std::function<void(shv::chainpack::RpcValue)> callback)
{
	if (!checkSites()) {
		downloadSites([this, callback](){
			callback(m_sitesString);
		});
		return cp::RpcValue();
	}
	return m_sitesString;
}

cp::RpcValue SitesProviderApp::getConfig(const cp::RpcValue &params)
{
	cp::RpcValue param;
	if (!params.isList() && !params.isMap()) {
		SHV_EXCEPTION("getConfig: invalid parameters type");
	}
	if (params.isList()) {
		cp::RpcValue::List param_list = params.toList();
		if (param_list.size() != 1) {
			SHV_EXCEPTION("getConfig: invalid parameter count");
		}
		param = param_list[0];
	}
	else if (params.isMap()) {
		cp::RpcValue::Map param_map = params.toMap();
		if (param_map.size() != 1) {
			SHV_EXCEPTION("saveConfig: invalid parameter count");
		}
		if (!param_map.hasKey("node_id")) {
			SHV_EXCEPTION("getConfig: missing parameter node_id");
		}
		param = param_map["node_id"];
	}
	if (!param.isString()) {
		SHV_EXCEPTION("getConfig: invalid parameter type");
	}
	QString node_id = QString::fromStdString(param.toString());
	if (node_id.isEmpty()) {
		SHV_EXCEPTION("getConfig: parameter type must not be empty");
	}
	return getConfig(node_id);
}

shv::chainpack::RpcValue SitesProviderApp::get(const shv::core::StringViewList &shv_path, std::function<void (shv::chainpack::RpcValue)> callback)
{
	if (!checkSites()) {
		downloadSites([this, shv_path, callback](){
			callback(get(shv_path));
		});
		return cp::RpcValue();
	}
	return get(shv_path);
}

shv::chainpack::RpcValue SitesProviderApp::ls(const shv::core::StringViewList &shv_path, std::function<void(cp::RpcValue)> callback)
{
	if (!checkSites()) {
		downloadSites([this, shv_path, callback](){
			callback(ls(shv_path, 0, m_sitesCp));
		});
		return cp::RpcValue();
	}
	return ls(shv_path, 0, m_sitesCp);
}

cp::RpcValue SitesProviderApp::ls(const shv::core::StringViewList &shv_path, size_t index, const cp::RpcValue::Map &object)
{
	if (shv_path.size() - index == 0) {
		cp::RpcValue::List items;
		if (!hasData(shv_path) && shv_path[index - 1].toString() != "_meta" && shv_path[index - 1].toString() != "_config") {
			items.push_back("_config");
		}
		for (auto it = object.cbegin(); it != object.cend(); ++it) {
			items.push_back(it->first);
		}
		return items;
	}
	cp::RpcValue val;
	if (shv_path[index] == "_config") {
		QString new_path = QString::fromStdString(shv::core::StringView::join(shv_path.begin(), shv_path.end() - 1, '/'));
		checkConfig(new_path);
		val = m_configs[new_path].chainpack;
	}
	else {
		val = object.at(shv_path[index].toString());
	}
//	shvInfo() << shv::core::StringView::join(shv_path, '/') << std::to_string(index) << shv_path[index].toString();
	if (val.isMap()) {
		return ls(shv_path, index + 1, val.toMap());
	}
	else {
		return cp::RpcValue::List();
	}
}

bool SitesProviderApp::hasData(const shv::iotqt::node::ShvNode::StringViewList &shv_path)
{
	cp::RpcValue leaf = this->leaf(shv_path);
	return !leaf.isMap();
}

cp::RpcValue SitesProviderApp::dir(const shv::core::StringViewList &shv_path, const cp::RpcValue &params, std::function<void(cp::RpcValue)> callback)
{
	if (!checkSites()) {
		downloadSites([this, shv_path, params, callback]() {
			callback(m_rootNode->dir(shv_path, params));
		});
	}
	return m_rootNode->dir(shv_path, params);
}

void SitesProviderApp::saveConfig(const cp::RpcValue &params)
{
	cp::RpcValue param1;
	cp::RpcValue param2;
	if (!params.isList() && !params.isMap()) {
		SHV_EXCEPTION("saveConfig: invalid parameters type");
	}
	if (params.isList()) {
		cp::RpcValue::List param_list = params.toList();
		if (param_list.size() != 2) {
			SHV_EXCEPTION("saveConfig: invalid parameter count");
		}
		param1 = param_list[0];
		param2 = param_list[1];
	}
	else if (params.isMap()) {
		cp::RpcValue::Map param_map = params.toMap();
		if (param_map.size() != 2) {
			SHV_EXCEPTION("saveConfig: invalid parameter count");
		}
		if (!param_map.hasKey("node_id")) {
			SHV_EXCEPTION("saveConfig: missing parameter node_id");
		}
		if (!param_map.hasKey("config")) {
			SHV_EXCEPTION("saveConfig: missing parameter config");
		}
		param1 = param_map["node_id"];
		param2 = param_map["config"];
	}
	if (!param1.isString()) {
		SHV_EXCEPTION("saveConfig: invalid node_id type");
	}
	if (!param2.isString()) {
		SHV_EXCEPTION("saveConfig: invalid config type");
	}
	QString node_id = QString::fromStdString(param1.toString());
	if (node_id.isEmpty()) {
		SHV_EXCEPTION("saveConfig: node_id type must not be empty");
	}
	QByteArray config = QByteArray::fromStdString(param2.toString());
	saveConfig(node_id, config);
}

shv::chainpack::RpcValue SitesProviderApp::leaf(const shv::core::StringViewList &shv_path)
{
	cp::RpcValue::Map object = m_sitesCp;
	cp::RpcValue value;
	for (size_t i = 0; i < shv_path.size(); ++i) {
		if (shv_path[i] == "_config") {
			QString new_path = QString::fromStdString(shv::core::StringView::join(shv_path.begin(), shv_path.begin() + i, '/'));
			checkConfig(new_path);
			value = m_configs[new_path].chainpack;
		}
		else {
			value = object.at(shv_path[i].toString());
		}
		if (value.isMap()) {
			object = value.toMap();
		}
		else {
			break;
		}
	}
	return value;
}

void SitesProviderApp::onRpcMessageReceived(const cp::RpcMessage &msg)
{
	shvLogFuncFrame() << msg.toCpon();
	if (msg.isRequest()) {
		cp::RpcRequest rq(msg);
		if(m_shvTree->root()) {
			m_shvTree->root()->handleRpcRequest(rq);
		}
	}
}

void SitesProviderApp::downloadSites(std::function<void ()> callback)
{
	if (m_downloadingSites) {
		QMetaObject::Connection *connection = new QMetaObject::Connection();
		*connection = connect(this, &SitesProviderApp::downloadFinished, [this, connection, callback]() {
			disconnect(*connection);
			delete connection;
			callback();
		});
		return;
	}
	m_downloadingSites = true;
	QNetworkAccessManager *network_manager = new QNetworkAccessManager(this);

	connect(network_manager, &QNetworkAccessManager::finished, [this, network_manager, callback](QNetworkReply *reply) {
		if (reply->error()) {
			shvInfo() << "Download sites.json error:" << reply->errorString();
		}
		else{
			QByteArray sites = reply->readAll();
			m_sitesString = sites.toStdString();
			m_sitesTime = QDateTime::currentDateTime();
			std::string err;
			cp::RpcValue sites_cp = cp::RpcValue::fromCpon(m_sitesString, &err);
			if (!err.empty()) {
				SHV_EXCEPTION(err);
			}
			m_sitesCp = sites_cp.toMap();
			shvInfo() << "Downloaded sites.json";
		}
		reply->deleteLater();
		network_manager->deleteLater();
		m_downloadingSites = false;
		Q_EMIT downloadFinished();
		callback();
	});
	network_manager->get(QNetworkRequest(QUrl(QString::fromStdString(m_cliOptions->remoteSitesUrl()))));
}

bool SitesProviderApp::checkSites() const
{
	return !m_sitesTime.isNull() && m_sitesTime.secsTo(QDateTime::currentDateTime()) < 3600;
}

QByteArray SitesProviderApp::getConfig(const QString &node_id)
{
	checkConfig(node_id);
	return m_configs[node_id].content;
}

shv::chainpack::RpcValue SitesProviderApp::get(const shv::core::StringViewList &shv_path)
{
	if (shv_path[shv_path.size() - 1] == "_config") {
		std::string new_path = shv::core::StringView::join(shv_path.begin(), shv_path.end() - 1, '/');
		QByteArray config = getConfig(QString::fromStdString(new_path));
		return config.toStdString();
	}
	cp::RpcValue val = leaf(shv_path);
	if (val.isBool()) {
		return val.toBool();
	}
	else if (val.isDouble()) {
		return val.toDouble();
	}
	else if (val.isString()) {
		return val.toString();
	}
	else {
		return val.toCpon();
	}
	Q_ASSERT_X(false, "", "Unsupported json type");
	return cp::RpcValue();
}

void SitesProviderApp::saveConfig(const QString &node_id, const QByteArray &value)
{
	Config &config = m_configs[node_id];
	std::string err;
	config.chainpack = cp::RpcValue::fromCpon(value.toStdString(), &err);
	if (!err.empty()) {
		SHV_EXCEPTION(err);
	}
	config.content = value;
	config.time = QDateTime::currentDateTime();

	QFile f(nodeConfigPath(node_id));
	QDir dir = QFileInfo(f).dir();

	if (!dir.exists()){
		if(!dir.mkpath(dir.absolutePath())) {
			shvError() << "Cannot create directory:" << dir.absolutePath();
			return;
		}
	}

	if (f.open(QFile::WriteOnly)) {
		f.write(value);
	}
	else {
		shvError() << "Cannot write to config file:" << nodeConfigPath(node_id);
	}
	f.close();
}

QString SitesProviderApp::nodeConfigPath(const QString &node_id)
{
	QString path = QString::fromStdString(m_cliOptions->dataDir());
	if (!node_id.startsWith(QDir::separator())) {
		path += QDir::separator();
	}
	path += node_id;
	if (!path.endsWith(QDir::separator())) {
		path += QDir::separator();
	}
	path += "config.cpon";
	return path;
}

void SitesProviderApp::checkConfig(const QString &path)
{
	QFileInfo file_info(nodeConfigPath(path));
	if (!m_configs.contains(path) || file_info.lastModified() > m_configs[path].time) {
		readConfig(path);
	}
}

void SitesProviderApp::readConfig(const QString &path)
{
	shvInfo() << "got request for config for" << path;
	QFile f(nodeConfigPath(path));
	QByteArray file_content;
	if (f.exists()) {
		if (f.open(QFile::ReadOnly)) {
			file_content = f.readAll();
			f.close();
		}
	}
	Config &config = m_configs[path];
	if (!file_content.isEmpty()) {
		std::string err;
		config.chainpack = cp::RpcValue::fromCpon(file_content.toStdString(), &err);
		if (!err.empty()) {
			SHV_EXCEPTION(err);
		}
	}
	else {
		config.chainpack = cp::RpcValue::String();
	}
	config.content = file_content;
	config.time = QFileInfo(f).lastModified();
}
