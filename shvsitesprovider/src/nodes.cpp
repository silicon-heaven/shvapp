#include "nodes.h"
#include "sitesproviderapp.h"
#include "appclioptions.h"

#include <shv/coreqt/log.h>
#include <shv/coreqt/exception.h>
#include <shv/iotqt/rpc/deviceconnection.h>
#include <shv/chainpack/cponreader.h>

#include <QDir>
#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkReply>

#include <fstream>

namespace cp = shv::chainpack;

using namespace std;

const string FILES_NODE = "_files";

static const char METH_GET_SITES[] = "getSites";
static const char METH_GET_CONFIG[] = "getConfig";
static const char METH_SAVE_CONFIG[] = "saveConfig";
static const char METH_RELOAD_SITES[] = "reloadSites";
static const char METH_SITES_SYNCED_BEFORE[] = "sitesSyncedBefore";
static const char METH_SITES_RELOADED[] = "reloaded";
static const char METH_FILE_READ[] = "read";
static const char METH_FILE_HASH[] = "hash";

static std::vector<cp::MetaMethod> root_meta_methods {
	{ cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, shv::chainpack::Rpc::ROLE_BROWSE },
	{ cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, shv::chainpack::Rpc::ROLE_BROWSE },
	{ cp::Rpc::METH_APP_NAME, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsGetter, shv::chainpack::Rpc::ROLE_READ },
	{ cp::Rpc::METH_DEVICE_ID, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsGetter, shv::chainpack::Rpc::ROLE_READ },
	{ cp::Rpc::METH_DEVICE_TYPE, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsGetter, cp::Rpc::ROLE_BROWSE},
	{ METH_GET_SITES, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::None, shv::chainpack::Rpc::ROLE_READ },
	{ METH_GET_CONFIG, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, shv::chainpack::Rpc::ROLE_READ },
	{ METH_SAVE_CONFIG, cp::MetaMethod::Signature::VoidParam, cp::MetaMethod::Flag::None, shv::chainpack::Rpc::ROLE_ADMIN },
	{ METH_RELOAD_SITES, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::None, shv::chainpack::Rpc::ROLE_COMMAND},
	{ METH_SITES_SYNCED_BEFORE, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::None, shv::chainpack::Rpc::ROLE_READ },
	{ METH_SITES_RELOADED, cp::MetaMethod::Signature::VoidParam, cp::MetaMethod::Flag::IsSignal, shv::chainpack::Rpc::ROLE_READ }
};

static std::vector<cp::MetaMethod> empty_leaf_meta_methods {
	{ cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, shv::chainpack::Rpc::ROLE_BROWSE },
	{ cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, shv::chainpack::Rpc::ROLE_BROWSE },
	{ METH_GET_CONFIG, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::None, shv::chainpack::Rpc::ROLE_READ },
	{ METH_SAVE_CONFIG, cp::MetaMethod::Signature::VoidParam, cp::MetaMethod::Flag::None, shv::chainpack::Rpc::ROLE_READ },
};

static std::vector<cp::MetaMethod> meta_leaf_meta_methods {
	{ cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, shv::chainpack::Rpc::ROLE_BROWSE },
	{ cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, shv::chainpack::Rpc::ROLE_BROWSE },
};

static std::vector<cp::MetaMethod> file_dir_meta_methods {
	{ cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, shv::chainpack::Rpc::ROLE_BROWSE },
	{ cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, shv::chainpack::Rpc::ROLE_BROWSE },
};

static std::vector<cp::MetaMethod> file_meta_methods {
	{ cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, shv::chainpack::Rpc::ROLE_BROWSE },
	{ cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, shv::chainpack::Rpc::ROLE_BROWSE },
	{ METH_FILE_READ, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::None, shv::chainpack::Rpc::ROLE_READ },
	{ METH_FILE_HASH, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::None, shv::chainpack::Rpc::ROLE_READ },
};

static std::vector<cp::MetaMethod> data_leaf_meta_methods {
	{ cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, shv::chainpack::Rpc::ROLE_BROWSE },
	{ cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, shv::chainpack::Rpc::ROLE_BROWSE },
	{ cp::Rpc::METH_GET, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::None, shv::chainpack::Rpc::ROLE_READ },
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
			param_list.push_back(shv_path.join('/'));
			param_list.push_back(params.toList()[0]);
			saveConfig(params);
		}
		return cp::RpcValue();
	}
	else if (method == METH_FILE_READ) {
		return readFile(shv_path);
	}
	else if (method == METH_FILE_HASH) {
		const string &bytes = readFile(shv_path).toString();
		QCryptographicHash h(QCryptographicHash::Sha1);
		h.addData(bytes.data(), bytes.size());
		return h.result().toHex().toStdString();
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
	else if(method == cp::Rpc::METH_DEVICE_TYPE) {
		return "SitesProvider";
	}
	else if (method == METH_RELOAD_SITES) {
		if (!m_sitesSyncedBefore.isValid() || m_sitesSyncedBefore.elapsed() > 5000) {
			downloadSites([](){});
		}
		return true;
	}
	else if (method == METH_SITES_SYNCED_BEFORE) {
		if(m_sitesSyncedBefore.isValid())
			return nullptr;
		return static_cast<int>(m_sitesSyncedBefore.elapsed() / 1000);
	}
	return Super::callMethod(shv_path, method, params);
}

void AppRootNode::handleRpcRequest(const shv::chainpack::RpcRequest &rq)
{
	if (!checkSites()) {
		downloadSites([this, rq]() {
			if (!m_downloadSitesError.empty()) {
				shvError() << m_downloadSitesError;
				cp::RpcResponse resp = cp::RpcResponse::forRequest(rq);
				resp.setError(cp::RpcResponse::Error::create(cp::RpcResponse::Error::MethodCallException, m_downloadSitesError));
				sendRpcMessage(resp);
			}
			else {
				Super::handleRpcRequest(rq);
			}
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
	else if (isDir(shv_path) || shv_path[shv_path.size() - 1] == FILES_NODE) {
		return file_dir_meta_methods;
	}
	else if (isFile(shv_path)) {
		return file_meta_methods;
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
	cp::RpcValue::Map object = m_sites;
	cp::RpcValue value;
	for (size_t i = 0; i < shv_path.size(); ++i) {
		value = object.value(shv_path[i].toString());
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
	return m_sites;
}

cp::RpcValue AppRootNode::getConfig(const cp::RpcValue &params)
{
	cp::RpcValue param;
	if (!params.isList() && !params.isMap() && !params.isString()) {
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
	else {
		param = params;
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
	return ls(shv_path, 0, m_sites);
}

cp::RpcValue AppRootNode::lsDir(const shv::core::StringViewList &shv_path)
{
	cp::RpcValue::List items;
	QString path = nodeLocalPath(shv_path.join('/'));
	QFileInfo fi(path);
	if(fi.isDir()) {
		QDir dir(nodeLocalPath(shv_path.join('/')));
		QFileInfoList file_infos = dir.entryInfoList(QDir::Filter::Dirs | QDir::Filter::Files | QDir::Filter::NoDotAndDotDot, QDir::SortFlag::Name);
		for (const QFileInfo &file_info : file_infos) {
			std::string new_item;
			if (file_info.suffix() == "cptempl") {
				new_item = (file_info.completeBaseName() + ".cpon").toStdString();
			}
			else {
				new_item = file_info.fileName().toStdString();
			}
			auto it = std::find(items.begin(), items.end(), new_item);
			if (it != items.end()) {
				items.erase(it);
				shvWarning() << "Duplicate template name with real file" << shv_path.join('/');
			}
			else {
				items.push_back(new_item);
			}
		}
	}
	return items;
}

cp::RpcValue AppRootNode::ls(const shv::core::StringViewList &shv_path, size_t index, const cp::RpcValue::Map &object)
{
	shvLogFuncFrame() << shv_path.join('/');
	if (shv_path.size() == index) {
		cp::RpcValue::List items;
		for (const auto &kv : object) {
			items.push_back(kv.first);
		}
		QFileInfo fi(nodeLocalPath(shv_path.join('/')));
		if(fi.isDir())
			items.insert(items.begin(), FILES_NODE);
		return cp::RpcValue(items);
	}
	std::string node_name = shv_path[index].toString();
	if(node_name == FILES_NODE) {
		return lsDir(shv_path);
	}
	const shv::chainpack::RpcValue val = object.value(node_name);
	if (val.isMap())
		return ls(shv_path, index + 1, val.toMap());
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
		*connection = connect(this, &AppRootNode::downloadFinished, [connection, callback]() {
			disconnect(*connection);
			delete connection;
			callback();
		});
		return;
	}
	m_downloadingSites = true;
	m_downloadSitesError.clear();
	QNetworkAccessManager *network_manager = new QNetworkAccessManager(this);
	network_manager->setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);
	connect(network_manager, &QNetworkAccessManager::finished, [this, network_manager, callback](QNetworkReply *reply) {
		try {
			if (reply->error()) {
				SHV_QT_EXCEPTION("Download sites.json error:" + reply->errorString());
			}
			std::string sites_string = reply->readAll().toStdString();
			m_sitesSyncedBefore.start();
			std::string err;
			cp::RpcValue sites_cp = cp::RpcValue::fromCpon(sites_string, &err);
			if (!err.empty()) {
				shvError() << sites_string;
				SHV_EXCEPTION(err);
			}
			if (!sites_cp.isMap()) {
				SHV_EXCEPTION("Sites.json must be map");
			}
			const cp::RpcValue::Map &sites_cp_map = sites_cp.toMap();
			if (!sites_cp_map.hasKey("shv")) {
				SHV_EXCEPTION("Sites.json does not have root key shv");
			}
			cp::RpcValue shv_cp = sites_cp_map.value("shv");
			if (!shv_cp.isMap()) {
				SHV_EXCEPTION("Shv key in sites.json must be map");
			}
			m_sites = shv_cp.toMap();
			shvInfo() << "Downloaded sites.json";
			if (SitesProviderApp::instance()->rpcConnection()->isBrokerConnected()) {
				cp::RpcSignal ntf;
				ntf.setMethod(METH_SITES_RELOADED);
				rootNode()->emitSendRpcMessage(ntf);
			}
		}
		catch (std::exception &e) {
			m_downloadSitesError = e.what();
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
	return m_downloadSitesError.empty() && (m_sitesSyncedBefore.isValid() && m_sitesSyncedBefore.elapsed() < 3600 * 1000);
}

shv::chainpack::RpcValue AppRootNode::getConfig(const QString &shv_path) const
{
	cp::RpcValue result;

	std::string file_name = nodeConfigPath(shv_path).toStdString();
	std::ifstream in_file;
	in_file.open(file_name, std::ios::in | std::ios::binary);
	if (in_file) {
		std::string err;
		result = cp::CponReader(in_file).read(&err);
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
	return leaf(shv_path);
}

shv::chainpack::RpcValue AppRootNode::readFile(const shv::core::StringViewList &shv_path)
{
	QFile f(nodeLocalPath(shv_path.join('/')));
	if (f.open(QFile::ReadOnly)) {
		QByteArray file_content = f.readAll();
		return file_content.toStdString();
	}
	QString templ_filename = nodeLocalPath(shv_path.join('/'));
	if (templ_filename.endsWith(".cpon")) {
		templ_filename.replace(templ_filename.length() - 5, 5, ".cptempl");
		if (QFile(templ_filename).exists()) {
			cp::RpcValue result = readAndMergeTempl(templ_filename);
			return result.toCpon("\t");
		}
	}
	SHV_EXCEPTION("Cannot open file: " + f.fileName().toStdString() + " for reading.");
}

shv::chainpack::RpcValue AppRootNode::readAndMergeTempl(const QString &path)
{
	static const char BASED_ON_KEY[] = "basedOn";

	QFile f(path);
	if (!f.open(QFile::ReadOnly)) {
		QString filename = path;
		if (filename.endsWith(".cpon")) {
			filename.replace(filename.length() - 5, 5, ".cptempl");
			f.setFileName(path);
			if (!f.open(QFile::ReadOnly)) {
				SHV_QT_EXCEPTION("Cannot open template: " + f.fileName() + " for reading.");
			}
		}
	}
	cp::RpcValue file_content = cp::RpcValue::fromCpon(f.readAll().toStdString());
	if (file_content.isMap()) {
		cp::RpcValue::Map file_content_map = file_content.toMap();
		if (file_content_map.hasKey(BASED_ON_KEY)) {
			QString templ_path = QString::fromStdString(file_content.toMap().value(BASED_ON_KEY).toString());
			QStringList templ_path_parts = templ_path.split('/');
			templ_path_parts.insert(templ_path_parts.count() - 1, QString::fromStdString(FILES_NODE));
			templ_path = templ_path_parts.join('/');
			if (templ_path.startsWith('/')) {
				templ_path = QString::fromStdString(SitesProviderApp::instance()->cliOptions()->localSitesDir()) + '/' + templ_path;
			}
			else {
				templ_path = QFileInfo(path).absolutePath() + "/../" + templ_path;
			}
			file_content_map.erase(BASED_ON_KEY);
			cp::RpcValue templ = readAndMergeTempl(templ_path);
			templ = mergeRpcValue(templ, file_content_map);
			return templ;
		}
	}
	return file_content;
}

shv::chainpack::RpcValue AppRootNode::mergeRpcValue(const shv::chainpack::RpcValue &base, const shv::chainpack::RpcValue &extend)
{
	cp::RpcValue::Map result;
	const cp::RpcValue::Map &base_map = base.toMap();
	const cp::RpcValue::Map &extend_map = extend.toMap();

	for (const std::string &base_key : base_map.keys()) {
		if (extend_map.hasKey(base_key)) {
			if (base_map.at(base_key).type() != cp::RpcValue::Type::Map || extend_map.at(base_key).type() != cp::RpcValue::Type::Map) {
				result.setValue(base_key, extend_map.at(base_key));
			}
			else {
				result.setValue(base_key, mergeRpcValue(base_map.at(base_key), extend_map.at(base_key)));
			}
		}
		else {
			result.setValue(base_key, base_map.at(base_key));
		}
	}
	for (const std::string &extend_key : extend_map.keys()) {
		if (!base_map.hasKey(extend_key)) {
			result.setValue(extend_key, extend_map.at(extend_key));
		}
	}
	return result;
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

QString AppRootNode::nodeFilesPath(const QString &shv_path) const
{
	return nodeLocalPath(shv_path) + "/" + QString::fromStdString(FILES_NODE);
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
	if (!path.endsWith('/') && !shv_path.startsWith('/') && !shv_path.isEmpty()) {
		path += '/';
	}
	return path + shv_path;
}

bool AppRootNode::isFile(const shv::iotqt::node::ShvNode::StringViewList &shv_path)
{
	QString filename = nodeLocalPath(shv_path.join('/'));
	QFileInfo fi(filename);
	if (fi.isFile())
		return true;

	if (filename.endsWith(".cpon")) {
		filename.replace(filename.length() - 5, 5, ".cptempl");
		QFileInfo fi(filename);
		if (fi.isFile())
			return true;
	}
	return false;
}

bool AppRootNode::isDir(const shv::iotqt::node::ShvNode::StringViewList &shv_path)
{
	QFileInfo fi(nodeLocalPath(shv_path.join('/')));
	return fi.isDir();
}

