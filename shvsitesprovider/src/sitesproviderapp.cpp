#include "sitesproviderapp.h"
#include "appclioptions.h"

#include <shv/iotqt/rpc/deviceconnection.h>
#include <shv/iotqt/node/shvnodetree.h>
#include <shv/coreqt/log.h>
#include <shv/coreqt/exception.h>
#include <shv/chainpack/cponreader.h>
#include <shv/chainpack/metamethod.h>
#include <shv/core/stringview.h>

#include <QDir>
#include <QFileInfo>
#include <QNetworkReply>
#include <QTimer>

#ifdef Q_OS_UNIX
#include <unistd.h>
#include <errno.h>
#include <string.h>
#endif

#include <fstream>

namespace cp = shv::chainpack;

static const char METH_GET_SITES[] = "getSites";
static const char METH_GET_CONFIG[] = "getConfig";
static const char METH_SAVE_CONFIG[] = "saveConfig";

static std::vector<cp::MetaMethod> root_meta_methods {
	{ cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, false, shv::chainpack::Rpc::GRANT_BROWSE },
	{ cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, false, shv::chainpack::Rpc::GRANT_BROWSE },
	{ cp::Rpc::METH_APP_NAME, cp::MetaMethod::Signature::RetVoid, false, shv::chainpack::Rpc::GRANT_READ },
	{ cp::Rpc::METH_DEVICE_ID, cp::MetaMethod::Signature::RetVoid, false, shv::chainpack::Rpc::GRANT_READ },
	{ METH_GET_SITES, cp::MetaMethod::Signature::RetVoid, false, shv::chainpack::Rpc::GRANT_READ },
	{ METH_GET_CONFIG, cp::MetaMethod::Signature::RetParam, false, shv::chainpack::Rpc::GRANT_READ },
	{ METH_SAVE_CONFIG, cp::MetaMethod::Signature::VoidParam, false, shv::chainpack::Rpc::GRANT_ADMIN },
};

static std::vector<cp::MetaMethod> empty_leaf_meta_methods {
	{ cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, false, shv::chainpack::Rpc::GRANT_BROWSE },
	{ cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, false, shv::chainpack::Rpc::GRANT_BROWSE },
	{ METH_GET_CONFIG, cp::MetaMethod::Signature::RetVoid, false, shv::chainpack::Rpc::GRANT_READ },
	{ METH_SAVE_CONFIG, cp::MetaMethod::Signature::VoidParam, false, shv::chainpack::Rpc::GRANT_READ },
};

static std::vector<cp::MetaMethod> meta_leaf_meta_methods {
	{ cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, false, shv::chainpack::Rpc::GRANT_BROWSE },
	{ cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, false, shv::chainpack::Rpc::GRANT_BROWSE },
};

static std::vector<cp::MetaMethod> file_leaf_meta_methods {
	{ cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, false, shv::chainpack::Rpc::GRANT_BROWSE },
	{ cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, false, shv::chainpack::Rpc::GRANT_BROWSE },
	{ cp::Rpc::METH_GET, cp::MetaMethod::Signature::RetVoid, false, shv::chainpack::Rpc::GRANT_READ },
};

static std::vector<cp::MetaMethod> data_leaf_meta_methods {
	{ cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, false, shv::chainpack::Rpc::GRANT_BROWSE },
	{ cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, false, shv::chainpack::Rpc::GRANT_BROWSE },
	{ cp::Rpc::METH_GET, cp::MetaMethod::Signature::RetVoid, false, shv::chainpack::Rpc::GRANT_READ },
};


AppRootNode::AppRootNode(QObject *parent)
	: Super(parent)
{
}

size_t AppRootNode::methodCount(const StringViewList &shv_path)
{
	return metaMethods(shv_path).size();
}

const cp::MetaMethod *AppRootNode::metaMethod(const StringViewList &shv_path, size_t ix)
{
	const std::vector<cp::MetaMethod> &meta_methods = metaMethods(shv_path);
	if (meta_methods.size() <= ix) {
		SHV_EXCEPTION("Invalid method index: " + std::to_string(ix) + " of: " + std::to_string(meta_methods.size()));
	}
	return &(meta_methods[ix]);
}

cp::RpcValue AppRootNode::callMethod(const StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params)
{
	if (method == cp::Rpc::METH_APP_NAME) {
		return QCoreApplication::instance()->applicationName().toStdString();
	}
	else if (method == METH_GET_SITES) {
		return getSites();
	}
	else if (method == METH_GET_CONFIG) {
		if (shv_path.empty()) {
			return getConfig(params);
		}
		else {
			cp::RpcValue::List params;
			params.push_back(shv::core::StringView::join(shv_path.cbegin(), shv_path.cend(), '/'));
			return getConfig(params);
		}
	}
	else if (method == METH_SAVE_CONFIG) {
		if (shv_path.empty()) {
			saveConfig(params);
		}
		else {
			if (!params.isList() || params.toList().size() != 1) {
				SHV_QT_EXCEPTION("Missing argument for saveConfig method");
			}
			cp::RpcValue::List param_list;
			param_list.push_back(shv::core::StringView::join(shv_path.cbegin(), shv_path.cend(), '/'));
			param_list.push_back(params.toList()[0]);
			saveConfig(params);
		}
		return cp::RpcValue();
	}
	else if (method == cp::Rpc::METH_GET) {
		return get(shv_path);
	}
	else if (method == cp::Rpc::METH_DEVICE_ID) {
		SitesProviderApp *app = SitesProviderApp::instance();
		cp::RpcValue::Map opts = app->rpcConnection()->connectionOptions().toMap();;
		cp::RpcValue::Map dev = opts.value(cp::Rpc::KEY_DEVICE).toMap();
		return dev.value(cp::Rpc::KEY_DEVICE_ID).toString();
	}
	return Super::callMethod(shv_path, method, params);
}

void AppRootNode::handleRpcRequest(const shv::chainpack::RpcRequest &rq)
{
	if (!checkSites()) {
		downloadSites([this, rq]() {
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
	else if (isFile(shv_path)) {
		return file_leaf_meta_methods;
	}
	else if (hasData(shv_path)) {
		return data_leaf_meta_methods;
	}
	else {
		return empty_leaf_meta_methods;
	}
}

shv::chainpack::RpcValue AppRootNode::leaf(const shv::core::StringViewList &shv_path)
{
	cp::RpcValue::Map object = m_sitesValue;
	cp::RpcValue value;
	for (size_t i = 0; i < shv_path.size(); ++i) {
		value = object.at(shv_path[i].toString());
		if (value.isMap()) {
			object = value.toMap();
		}
		else {
			break;
		}
	}
	return value;
}

cp::RpcValue AppRootNode::getSites()
{
	return m_sitesJsonString;
}

cp::RpcValue AppRootNode::getConfig(const cp::RpcValue &params)
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
		if (!param_map.hasKey("shvPath")) {
			SHV_EXCEPTION("getConfig: missing parameter shvPath");
		}
		param = param_map["shvPath"];
	}
	if (!param.isString()) {
		SHV_EXCEPTION("getConfig: invalid parameter type");
	}
	QString shv_path = QString::fromStdString(param.toString());
	if (shv_path.isEmpty()) {
		SHV_EXCEPTION("getConfig: parameter type must not be empty");
	}
	return getConfig(shv_path);
}

shv::chainpack::RpcValue AppRootNode::ls(const shv::core::StringViewList &shv_path, const shv::chainpack::RpcValue &params)
{
	Q_UNUSED(params);
	return ls(shv_path, 0, m_sitesValue);
}

cp::RpcValue AppRootNode::ls(const shv::core::StringViewList &shv_path, size_t index, const cp::RpcValue::Map &object)
{
	if (shv_path.size() - index == 0) {
		cp::RpcValue::List items;
		for (auto it = object.cbegin(); it != object.cend(); ++it) {
			items.push_back(it->first);
		}
		bool is_meta = false;
		for (uint i = 0; i < shv_path.size(); ++i) {
			if (shv_path[i] == "_meta") {
				is_meta = true;
				break;
			}
		}
		if (!is_meta) {
			QDir dir(nodeLocalPath(shv_path));
			QFileInfoList file_infos = dir.entryInfoList(QDir::Filter::Files, QDir::SortFlag::Name);
			for (const QFileInfo &file_info : file_infos) {
				items.push_back(file_info.fileName().toStdString());
			}
		}
		return items;
	}
	std::string key = shv_path[index].toString();
	if (object.hasKey(key)) {
		cp::RpcValue val = object.at(key);
		if (val.isMap()) {
			return ls(shv_path, index + 1, val.toMap());
		}
	}
	return cp::RpcValue::List();
}

bool AppRootNode::hasData(const shv::iotqt::node::ShvNode::StringViewList &shv_path)
{
	cp::RpcValue leaf = this->leaf(shv_path);
	return !leaf.isMap();
}

void AppRootNode::saveConfig(const cp::RpcValue &params)
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
		if (!param_map.hasKey("shvPath")) {
			SHV_EXCEPTION("saveConfig: missing parameter shvPath");
		}
		if (!param_map.hasKey("config")) {
			SHV_EXCEPTION("saveConfig: missing parameter config");
		}
		param1 = param_map["shvPath"];
		param2 = param_map["config"];
	}
	if (!param1.isString()) {
		SHV_EXCEPTION("saveConfig: invalid shvPath type");
	}
	if (!param2.isString()) {
		SHV_EXCEPTION("saveConfig: invalid config type");
	}
	QString shv_path = QString::fromStdString(param1.toString());
	if (shv_path.isEmpty()) {
		SHV_EXCEPTION("saveConfig: shvPath type must not be empty");
	}
	QByteArray config = QByteArray::fromStdString(param2.toString());
	saveConfig(shv_path, config);
}

void AppRootNode::downloadSites(std::function<void ()> callback)
{
	if (m_downloadingSites) {
		QMetaObject::Connection *connection = new QMetaObject::Connection();
		*connection = connect(this, &AppRootNode::downloadFinished, [this, connection, callback]() {
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
			m_sitesJsonString = sites.toStdString();
			m_sitesTime = QDateTime::currentDateTime();
			std::string err;
			cp::RpcValue sites_cp = cp::RpcValue::fromCpon(m_sitesJsonString, &err);
			if (!err.empty()) {
				SHV_EXCEPTION(err);
			}
			m_sitesValue = sites_cp.toMap();
			shvInfo() << "Downloaded sites.json";
		}
		reply->deleteLater();
		network_manager->deleteLater();
		m_downloadingSites = false;
		Q_EMIT downloadFinished();
		callback();
	});
	network_manager->get(QNetworkRequest(QUrl(QString::fromStdString(SitesProviderApp::instance()->cliOptions()->remoteSitesUrl()))));
}

bool AppRootNode::checkSites() const
{
	return !m_sitesTime.isNull() && m_sitesTime.secsTo(QDateTime::currentDateTime()) < 3600;
}

shv::chainpack::RpcValue AppRootNode::getConfig(const QString &shv_path) const
{
	cp::RpcValue result;

	std::string file_name = nodeConfigPath(shv_path).toStdString();
	std::ifstream in_file;
	in_file.open(file_name, std::ios::in | std::ios::binary);
	if (in_file) {
		std::string err;
		cp::CponReader(in_file).read(result, err);
		in_file.close();
		if (!err.empty()) {
			SHV_EXCEPTION(err);
		}
	}
	if (!result.isValid()){
		result = cp::RpcValue::Map();
	}
	return result;
}

shv::chainpack::RpcValue AppRootNode::get(const shv::core::StringViewList &shv_path)
{
	if (!shv_path.empty() && !(shv_path[shv_path.size() - 1] == "_meta") && isFile(shv_path)) {
		QFile f(nodeLocalPath(shv_path));
		if (!f.open(QFile::ReadOnly)) {
			return std::string();
		}
		QByteArray file_content = f.readAll();
		f.close();
		return file_content.toStdString();
	}
	return leaf(shv_path);
}

void AppRootNode::saveConfig(const QString &shv_path, const QByteArray &value)
{
	QFile f(nodeConfigPath(shv_path));
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
		shvError() << "Cannot write to config file:" << nodeConfigPath(shv_path);
	}
	f.close();
}

QString AppRootNode::nodeConfigPath(const QString &shv_path) const
{
	QString path = nodeLocalPath(shv_path);
	path += "/config.cpon";
	return path;
}

QString AppRootNode::nodeLocalPath(const QString &shv_path) const
{
	QString path = QString::fromStdString(SitesProviderApp::instance()->cliOptions()->localSitesDir());
	if (!path.endsWith(QDir::separator()) && !shv_path.startsWith(QDir::separator())) {
		path += QDir::separator();
	}
	return path + shv_path;
}

QString AppRootNode::nodeLocalPath(const std::string &shv_path) const
{
	return nodeLocalPath(QString::fromStdString(shv_path));
}

QString AppRootNode::nodeLocalPath(const shv::core::StringViewList &shv_path) const
{
	if (shv_path.size()) {
		return nodeLocalPath(shv::core::StringView::join(shv_path.begin(), shv_path.end(), '/'));
	}
	else {
		return nodeLocalPath(QString());
	}
}

bool AppRootNode::isFile(const shv::iotqt::node::ShvNode::StringViewList &shv_path)
{
	uint i = 0;
	shv::chainpack::RpcValue::Map map = m_sitesValue;
	while (true) {
		std::string key = shv_path[i].toString();
		if (map.hasKey(key)) {
			if (i + 1 == shv_path.size()) {
				return false;
			}
			cp::RpcValue child = map[key];
			if (child.isMap()) {
				map = child.toMap();
				++i;
				continue;
			}
			else {
				break;
			}
		}
		else {
			break;
		}
	}
	return QFile::exists(nodeLocalPath(shv_path));
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
		cli_opts->setUser("shvsitesprovider");
	}
	if (!cli_opts->password_isset()) {
		cli_opts->setPassword("lub42DUB");
		cli_opts->setLoginType("PLAIN");
	}
	if (!cli_opts->deviceId_isset()) {
		cli_opts->setDeviceId("shvsitesprovider");
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
	shvInfo() << "destroying shvsitesprovider application";
}

SitesProviderApp *SitesProviderApp::instance()
{
	return qobject_cast<SitesProviderApp *>(QCoreApplication::instance());
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
