#include "nodes.h"
#include "sitesproviderapp.h"
#include "appclioptions.h"

#include <shv/coreqt/log.h>
#include <shv/coreqt/exception.h>
#include <shv/iotqt/rpc/deviceconnection.h>
#include <shv/chainpack/cponreader.h>

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkReply>

#include <fstream>

namespace cp = shv::chainpack;

using namespace std;

const string FILES_NODE = "_files";
const QString CPON_SUFFIX = QStringLiteral("cpon");
const QString CPTEMPL_SUFFIX = QStringLiteral("cptempl");

static const char METH_GET_SITES[] = "getSites";
static const char METH_GET_CONFIG[] = "getConfig";
static const char METH_SAVE_CONFIG[] = "saveConfig";
static const char METH_RELOAD_SITES[] = "reloadSites";
static const char METH_SITES_SYNCED_BEFORE[] = "sitesSyncedBefore";
static const char METH_SITES_RELOADED[] = "reloaded";
static const char METH_FILE_READ[] = "read";
static const char METH_FILE_WRITE[] = "write";
static const char METH_FILE_HASH[] = "hash";
static const char METH_FILE_MK[] = "mkfile";

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

static std::vector<cp::MetaMethod> files_node_dir_meta_methods {
	{ cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, shv::chainpack::Rpc::ROLE_BROWSE },
	{ cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, shv::chainpack::Rpc::ROLE_BROWSE },
	{ METH_FILE_MK, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, shv::chainpack::Rpc::ROLE_WRITE },
};

static std::vector<cp::MetaMethod> file_meta_methods {
	{ cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, shv::chainpack::Rpc::ROLE_BROWSE },
	{ cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, shv::chainpack::Rpc::ROLE_BROWSE },
	{ METH_FILE_READ, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::None, shv::chainpack::Rpc::ROLE_READ },
	{ METH_FILE_WRITE, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, shv::chainpack::Rpc::ROLE_WRITE },
	{ METH_FILE_HASH, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::None, shv::chainpack::Rpc::ROLE_READ },
};

static std::vector<cp::MetaMethod> data_leaf_meta_methods {
	{ cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, shv::chainpack::Rpc::ROLE_BROWSE },
	{ cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, shv::chainpack::Rpc::ROLE_BROWSE },
	{ cp::Rpc::METH_GET, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsGetter, shv::chainpack::Rpc::ROLE_READ },
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
	else if (method == METH_FILE_WRITE) {
		if(!params.isString())
			throw shv::core::Exception("Content must be string.");
		return writeFile(shv_path, params.toString());
	}
	else if (method == METH_FILE_MK) {
		return mkFile(shv_path, params);
	}
	else if (method == METH_FILE_HASH) {
		auto rv = readFile(shv_path);
		const string &bytes = rv.toString();
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
	else if (shv_path.indexOf("_meta") != -1) {
		if (hasData(shv_path)) {
			return data_leaf_meta_methods;
		}
		else {
			return meta_leaf_meta_methods;
		}
	}
	else if (shv_path.indexOf(FILES_NODE) != -1) {
		if (isFile(shv_path)) {
			return file_meta_methods;
		}
		else {
			return file_dir_meta_methods;
		}
	}
	return empty_leaf_meta_methods;
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
	return ls_helper(shv_path, 0, m_sites);
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
			if (file_info.suffix() == CPTEMPL_SUFFIX) {
				items.push_back(file_info.fileName().toStdString());
				new_item = file_info.completeBaseName().toStdString();
			}
			else {
				new_item = file_info.fileName().toStdString();
			}
			auto it = std::find(items.begin(), items.end(), new_item);
			if (it != items.end() && file_info.suffix() == CPON_SUFFIX) {
				items.erase(it);
			}
			items.push_back(new_item);
		}
	}
	return items;
}

cp::RpcValue AppRootNode::ls_helper(const shv::core::StringViewList &shv_path, size_t index, const cp::RpcValue::Map &sites_node)
{
	//shvLogFuncFrame() << shv_path.join('/');
	if (shv_path.size() == index) {
		cp::RpcValue::List items;
		for (const auto &kv : sites_node) {
			items.push_back(kv.first);
		}
		QString local_dir = nodeLocalPath(shv_path.join('/'));
		QDirIterator it(local_dir, QStringList{"_*"}, QDir::NoDotAndDotDot | QDir::Dirs | QDir::Files, QDirIterator::NoIteratorFlags);
		//shvDebug() << local_dir << "---------------------------------------";
		while (it.hasNext()) {
			QString fn = it.next();
			fn = QFileInfo(fn).baseName();
			//shvDebug() << fn;
			items.insert(items.begin(), fn.toStdString());
		}
		return cp::RpcValue(items);
	}
	std::string node_name = shv_path[index].toString();
	if(node_name == FILES_NODE) {
		return lsDir(shv_path);
	}
	const shv::chainpack::RpcValue val = sites_node.value(node_name);
	if (val.isMap())
		return ls_helper(shv_path, index + 1, val.toMap());
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
	QString filename = nodeLocalPath(shv_path.join('/'));
	QFile f(filename);
	if (f.open(QFile::ReadOnly)) {
		QByteArray file_content = f.readAll();
		return file_content.toStdString();
	}
	return readConfig(filename).toCpon("  ");
}

shv::chainpack::RpcValue AppRootNode::writeFile(const shv::core::StringViewList &shv_path, const string &content)
{
	QString filename = nodeLocalPath(shv_path.join('/'));
	QFile f(filename);
	if (f.open(QFile::WriteOnly)) {
		qint64 n = f.write(content.data(), content.size());
		return n >= 0;
	}
	else {
		throw shv::core::Exception("Cannot open file '" + shv_path.join('/') + "' for writing.");
	}
	return readConfig(filename).toCpon("  ");
}

shv::chainpack::RpcValue AppRootNode::mkFile(const shv::core::StringViewList &shv_path, const shv::chainpack::RpcValue &params)
{
	std::string error;
	string file_name;
	bool has_content = false;
	if (params.isString()) {
		file_name = params.toString();

	}
	else if(params.isList()) {
		const cp::RpcValue::List &lst = params.toList();
		if (lst.size() != 2)
			throw shv::core::Exception("Invalid params, [\"name\", \"content\"] expected.");
		file_name = lst[0].toString();
		has_content = true;
	}
	if (file_name.empty())
		throw shv::core::Exception("File name is empty.");
	shv::core::StringViewList file_path = shv_path;
	file_path.push_back(file_name);
	if(has_content)
		return writeFile(file_path, params.toList()[1].toString());
	else
		return writeFile(file_path, "");
}

shv::chainpack::RpcValue AppRootNode::readConfig(const QString &path)
{
	QFile f(path);
	if (!f.open(QFile::ReadOnly)) {
		if (path.endsWith(CPON_SUFFIX)) {
			QString filename = path + "." + CPTEMPL_SUFFIX;
			f.setFileName(filename);
			if (!f.open(QFile::ReadOnly)) {
				SHV_QT_EXCEPTION("Cannot open template: " + f.fileName() + " for reading.");
			}
		}
	}
	return readAndMergeConfig(f);
}

shv::chainpack::RpcValue AppRootNode::readAndMergeConfig(QFile &file)
{
	static const char BASED_ON_KEY[] = "basedOn";

	shv::chainpack::RpcValue config = cp::RpcValue::fromCpon(file.readAll().toStdString());
	if (config.isMap()) {
		QString templ_path = QString::fromStdString(config.toMap().value(BASED_ON_KEY).toString());
		if (!templ_path.isEmpty()) {
			QStringList templ_path_parts = templ_path.split('/');
			templ_path_parts.insert(templ_path_parts.count() - 1, QString::fromStdString(FILES_NODE));
			templ_path = templ_path_parts.join('/');
			if (templ_path.startsWith('/')) {
				templ_path = nodeLocalPath(templ_path);
			}
			else {
				templ_path = QFileInfo(file.fileName()).absolutePath() + "/../" + templ_path;
			}
			cp::RpcValue::Map file_content_map = config.toMap();
			file_content_map.erase(BASED_ON_KEY);
			cp::RpcValue templ = readConfig(templ_path);
			return shv::chainpack::Utils::mergeMaps(templ, file_content_map);
		}
	}
	return config;
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

	if (filename.endsWith(CPON_SUFFIX)) {
		filename.append("." + CPTEMPL_SUFFIX);
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
