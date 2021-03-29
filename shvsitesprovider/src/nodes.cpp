#include "nodes.h"
#include "gitpushtask.h"
#include "sitesproviderapp.h"
#include "dirsynctask.h"
#include "appclioptions.h"

#include <shv/core/utils/shvpath.h>
#include <shv/coreqt/log.h>
#include <shv/coreqt/exception.h>
#include <shv/iotqt/rpc/deviceconnection.h>
#include <shv/iotqt/rpc/rpc.h>
#include <shv/iotqt/rpc/rpcresponsecallback.h>
#include <shv/chainpack/cponreader.h>

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QProcess>

#include <fstream>

namespace cp = shv::chainpack;

using namespace std;

const string FILES_NODE = "_files";
const QString CPON_SUFFIX = QStringLiteral("cpon");
const QString CPTEMPL_SUFFIX = QStringLiteral("cptempl");

static const char METH_GIT_COMMIT[] = "gitCommit";
static const char METH_APP_VERSION[] = "version";
static const char METH_GET_SITES[] = "getSites";
static const char METH_RELOAD_SITES[] = "reloadSites";
static const char METH_SITES_SYNCED_BEFORE[] = "sitesSyncedBefore";
static const char METH_SITES_RELOADED[] = "reloaded";
static const char METH_FILE_READ[] = "read";
static const char METH_FILE_WRITE[] = "write";
static const char METH_FILE_HASH[] = "hash";
static const char METH_PULL_FILES[] = "pullFiles";
static const char METH_FILE_MK[] = "mkfile";
static const char METH_GIT_PUSH[] = "addFilesToVersionControl";

static std::vector<cp::MetaMethod> root_meta_methods {
	{ cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, shv::chainpack::Rpc::ROLE_BROWSE },
	{ cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, shv::chainpack::Rpc::ROLE_BROWSE },
	{ cp::Rpc::METH_APP_NAME, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsGetter, shv::chainpack::Rpc::ROLE_READ },
	{ METH_APP_VERSION, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsGetter, shv::chainpack::Rpc::ROLE_READ },
	{ cp::Rpc::METH_DEVICE_ID, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsGetter, shv::chainpack::Rpc::ROLE_READ },
	{ cp::Rpc::METH_DEVICE_TYPE, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsGetter, cp::Rpc::ROLE_BROWSE},
	{ METH_GIT_COMMIT, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsGetter, cp::Rpc::ROLE_READ},
	{ METH_GET_SITES, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::None, shv::chainpack::Rpc::ROLE_READ },
	{ METH_RELOAD_SITES, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::None, shv::chainpack::Rpc::ROLE_COMMAND},
	{ METH_SITES_SYNCED_BEFORE, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::None, shv::chainpack::Rpc::ROLE_READ },
	{ METH_SITES_RELOADED, cp::MetaMethod::Signature::VoidParam, cp::MetaMethod::Flag::IsSignal, shv::chainpack::Rpc::ROLE_READ },
	{ METH_PULL_FILES, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::None, shv::chainpack::Rpc::ROLE_WRITE },
//	{ METH_GIT_PUSH, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::None, shv::chainpack::Rpc::ROLE_ADMIN },
};

static std::vector<cp::MetaMethod> dir_meta_methods {
	{ cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, shv::chainpack::Rpc::ROLE_BROWSE },
	{ cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, shv::chainpack::Rpc::ROLE_BROWSE },
	{ METH_PULL_FILES, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::None, shv::chainpack::Rpc::ROLE_WRITE },
	{ METH_GET_SITES, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::None, shv::chainpack::Rpc::ROLE_READ },
};

static std::vector<cp::MetaMethod> device_meta_methods {
	{ cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, shv::chainpack::Rpc::ROLE_BROWSE },
	{ cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, shv::chainpack::Rpc::ROLE_BROWSE },
	{ METH_PULL_FILES, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::None, shv::chainpack::Rpc::ROLE_WRITE },
	{ METH_GET_SITES, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::None, shv::chainpack::Rpc::ROLE_READ },
};

static std::vector<cp::MetaMethod> meta_meta_methods {
	{ cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, shv::chainpack::Rpc::ROLE_BROWSE },
	{ cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, shv::chainpack::Rpc::ROLE_BROWSE },
};

static std::vector<cp::MetaMethod> file_dir_meta_methods {
	{ cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, shv::chainpack::Rpc::ROLE_BROWSE },
	{ cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, shv::chainpack::Rpc::ROLE_BROWSE },
	{ METH_FILE_MK, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, shv::chainpack::Rpc::ROLE_WRITE },
};

static std::vector<cp::MetaMethod> device_file_dir_meta_methods {
	{ cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, shv::chainpack::Rpc::ROLE_BROWSE },
	{ cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, shv::chainpack::Rpc::ROLE_BROWSE },
	{ METH_FILE_MK, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, shv::chainpack::Rpc::ROLE_WRITE },
	{ METH_PULL_FILES, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::None, shv::chainpack::Rpc::ROLE_WRITE },
};

static std::vector<cp::MetaMethod> file_meta_methods {
	{ cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, shv::chainpack::Rpc::ROLE_BROWSE },
	{ cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, shv::chainpack::Rpc::ROLE_BROWSE },
	{ METH_FILE_READ, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::None, shv::chainpack::Rpc::ROLE_READ },
	{ METH_FILE_WRITE, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, shv::chainpack::Rpc::ROLE_WRITE },
	{ METH_FILE_HASH, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::None, shv::chainpack::Rpc::ROLE_READ },
};

static std::vector<cp::MetaMethod> data_meta_methods {
	{ cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, shv::chainpack::Rpc::ROLE_BROWSE },
	{ cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, shv::chainpack::Rpc::ROLE_BROWSE },
	{ cp::Rpc::METH_GET, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsGetter, shv::chainpack::Rpc::ROLE_READ },
};


AppRootNode::AppRootNode(QObject *parent)
	: Super(parent)
{
	if (QDir(QString::fromStdString(SitesProviderApp::instance()->cliOptions()->localSitesDir()) + "/.git").exists()) {
		shvInfo() << "sites directory" << SitesProviderApp::instance()->cliOptions()->localSitesDir() << "identified as git repository";
		root_meta_methods.push_back({ METH_GIT_PUSH, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::None, shv::chainpack::Rpc::ROLE_ADMIN });
	}
	QFile sites_file(sitesFileName());
	if (sites_file.open(QFile::ReadOnly)) {
		cp::RpcValue sites = cp::RpcValue::fromCpon(sites_file.readAll().toStdString());
		if (sites.isMap()) {
			m_sites = sites.toMap();
		}
	}
}

QString AppRootNode::sitesFileName() const
{
	return QString::fromStdString(SitesProviderApp::instance()->cliOptions()->configDir()) + "/sites.json";
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

cp::RpcValue AppRootNode::callMethodRq(const cp::RpcRequest &rq)
{
	std::string method = rq.method().toString();

	if (method == cp::Rpc::METH_APP_NAME) {
		return QCoreApplication::instance()->applicationName().toStdString();
	}
	else if(method == METH_GIT_COMMIT) {
#ifdef GIT_COMMIT
		return SHV_EXPAND_AND_QUOTE(GIT_COMMIT);
#else
		return "N/A";
#endif
	}
	else if (method == METH_APP_VERSION) {
		return QCoreApplication::instance()->applicationVersion().toStdString();
	}
	else if (method == METH_GET_SITES) {
		return getSites(rq.shvPath().toString());
	}
	else if (method == METH_FILE_READ) {
		return readFile(rpcvalue_cast<QString>(rq.shvPath()));
	}
	else if (method == METH_FILE_WRITE) {
		cp::RpcValue params = rq.params();
		if(!params.isString())
			throw shv::core::Exception("Content must be string.");
		return writeFile(rpcvalue_cast<QString>(rq.shvPath()), params.toString());
	}
	else if (method == METH_FILE_MK) {
		std::string shv_path = rq.shvPath().toString();
		return mkFile(shv::core::utils::ShvPath::split(shv_path), rq.params());
	}
	else if (method == METH_PULL_FILES) {
		DirSyncTask *sync_task = new DirSyncTask(this);
		std::string shv_path = rq.shvPath().toString();
		shv::core::StringViewList shv_path_view = shv::core::utils::ShvPath::split(shv_path);
		QString qshv_path = QString::fromStdString(shv_path);
		QString qfiles_node = QString::fromStdString(FILES_NODE);
		if (qshv_path.endsWith(qfiles_node)) {
			sync_task->addDir(qshv_path);
		}
		else if (isDevice(shv_path_view)) {
			qshv_path += "/" + qfiles_node;
			sync_task->addDir(qshv_path);
		}
		else {
			QStringList devices;
			findDevicesToSync(shv_path_view, devices);
			for (const QString &device_path : devices) {
				sync_task->addDir(device_path);
			}
		}
		connect(sync_task, &DirSyncTask::finished, [this, rq, sync_task](bool success) {
			cp::RpcResponse resp = cp::RpcResponse::forRequest(rq);
			if (success) {
				resp.setResult(sync_task->result());
			}
			else {
				resp.setError(cp::RpcResponse::Error::create(cp::RpcResponse::Error::MethodCallException, sync_task->error()));
			}
			emitSendRpcMessage(resp);
		});
		sync_task->start();
		return cp::RpcValue();
	}
	else if (method == METH_FILE_HASH) {
		string bytes = readFile(rpcvalue_cast<QString>(rq.shvPath())).toString();
		QCryptographicHash h(QCryptographicHash::Sha1);
		h.addData(bytes.data(), bytes.size());
		return h.result().toHex().toStdString();
	}
	else if (method == cp::Rpc::METH_GET) {
		std::string shv_path = rq.shvPath().toString();
		return get(shv::core::utils::ShvPath::split(shv_path));
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
			downloadSites();
		}
		return true;
	}
	else if (method == METH_SITES_SYNCED_BEFORE) {
		if(!m_sitesSyncedBefore.isValid())
			return nullptr;
		return static_cast<int>(m_sitesSyncedBefore.elapsed() / 1000);
	}
	else if (method == METH_GIT_PUSH) {
		GitPushTask *git_task = new GitPushTask(QString::fromStdString(rq.userId().toString()), this);
		connect(git_task, &GitPushTask::finished, [this, rq, git_task](bool success) {
			cp::RpcResponse resp = cp::RpcResponse::forRequest(rq);
			if (success) {
				resp.setResult(true);
			}
			else {
				resp.setError(cp::RpcResponse::Error::create(cp::RpcResponse::Error::MethodCallException, git_task->error().toStdString()));
			}
			emitSendRpcMessage(resp);
		});
		git_task->start();
		return cp::RpcValue();
	}
	return Super::callMethodRq(rq);
}

void AppRootNode::handleRpcRequest(const shv::chainpack::RpcRequest &rq)
{
	if (!checkSites()) {
		QMetaObject::Connection *conn = new QMetaObject::Connection();
		*conn = connect(this, &AppRootNode::sitesDownloaded, this, [this, conn, rq]() {
			disconnect(*conn);
			delete conn;
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
		downloadSites();
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
			return data_meta_methods;
		}
		else {
			return meta_meta_methods;
		}
	}
	else if (shv_path.indexOf(FILES_NODE) != -1) {
		if (isFile(shv_path)) {
			return file_meta_methods;
		}
		else {
			if (shv_path.at(shv_path.length() - 1) == FILES_NODE && isDevice(shv_path.mid(0, shv_path.size() - 1))) {
				return device_file_dir_meta_methods;
			}
			else {
				return file_dir_meta_methods;
			}
		}
	}
	if (isDevice(shv_path)) {
		return device_meta_methods;
	}
	else {
		return dir_meta_methods;
	}
}

shv::chainpack::RpcValue AppRootNode::findSitesTreeValue(const shv::core::StringViewList &shv_path)
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

cp::RpcValue AppRootNode::getSites(const std::string &path)
{
	std::vector<std::string> path_parts = shv::core::String::split(path, '/');
	cp::RpcValue res = m_sites;
	for (const std::string &path_part : path_parts) {
		res = res.at(path_part);
		if (!res.isMap()) {
			return cp::RpcValue(nullptr);
		}
	}
	return res;
}

void AppRootNode::findDevicesToSync(const StringViewList &shv_path, QStringList &result)
{
	cp::RpcValue::List ls_result = ls_helper(shv_path, 0, m_sites).toList();
	for (const cp::RpcValue &ls_result_item : ls_result) {
		std::string last_path_item = ls_result_item.toString();
		shv::core::StringViewList new_shv_path(shv_path);
		new_shv_path.push_back(last_path_item);
		if (last_path_item[0] == '_') {
			if (last_path_item == FILES_NODE && isDevice(shv_path)) {
				result << QString::fromStdString(new_shv_path.join('/'));
			}
		}
		else {
			findDevicesToSync(new_shv_path, result);
		}
	}
}

shv::chainpack::RpcValue AppRootNode::ls(const shv::core::StringViewList &shv_path, const shv::chainpack::RpcValue &params)
{
	Q_UNUSED(params);
	return ls_helper(shv_path, 0, m_sites);
}

QStringList AppRootNode::lsDir(const QString &shv_path)
{
	QStringList items;
	QString path = nodeLocalPath(shv_path);
	QFileInfo fi(path);
	if(fi.isDir()) {
		QDir dir(nodeLocalPath(shv_path));
		QFileInfoList file_infos = dir.entryInfoList(QDir::Filter::Dirs | QDir::Filter::Files | QDir::Filter::NoDotAndDotDot, QDir::SortFlag::Name);
		for (const QFileInfo &file_info : file_infos) {
			QString new_item;
			if (file_info.suffix() == CPTEMPL_SUFFIX) {
				items.push_back(file_info.fileName());
				new_item = file_info.completeBaseName();
			}
			else {
				new_item = file_info.fileName();
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
		if (shv_path.indexOf("_meta") == -1) {
			items.push_back(FILES_NODE);
		}
		for (const auto &kv : sites_node) {
			items.push_back(kv.first);
		}
		return cp::RpcValue(items);
	}
	std::string node_name = shv_path[index].toString();
	if (node_name == FILES_NODE) {
		QStringList ls_dir = lsDir(QString::fromStdString(shv_path.join('/')));
		cp::RpcValue::List ret;
		for (const QString &ls_dir_item : ls_dir) {
			ret.push_back(ls_dir_item.toStdString());
		}
		return ret;
	}
	const shv::chainpack::RpcValue val = sites_node.value(node_name);
	if (val.isMap())
		return ls_helper(shv_path, index + 1, val.toMap());
	return cp::RpcValue::List();
}

bool AppRootNode::hasData(const shv::iotqt::node::ShvNode::StringViewList &shv_path)
{
	cp::RpcValue leaf = this->findSitesTreeValue(shv_path);
	return !leaf.isMap();
}

bool AppRootNode::isDevice(const shv::iotqt::node::ShvNode::StringViewList &shv_path)
{
	cp::RpcValue node = this->findSitesTreeValue(shv_path);
	if (!node.isMap()) {
		return false;
	}
	const cp::RpcValue::Map &node_map = node.toMap();
	return node_map.size() == 1 && node_map.hasKey("_meta") && node_map.value("_meta").toMap().hasKey("type");
}

void AppRootNode::downloadSites()
{
	if (m_downloadingSites) {
		return;
	}
	m_downloadingSites = true;
	m_downloadSitesError.clear();
	auto on_sites_received = [this](const shv::chainpack::RpcValue &res) {
		if (res.isMap()) {
			m_sites = res.toMap();
			shvInfo() << "Downloaded sites.json";
			m_sitesSyncedBefore.start();
			if (SitesProviderApp::instance()->rpcConnection()->isBrokerConnected()) {
				cp::RpcSignal ntf;
				ntf.setMethod(METH_SITES_RELOADED);
				rootNode()->emitSendRpcMessage(ntf);
			}
		}
		m_downloadingSites = false;
		QFile sites_file(sitesFileName());
		if (sites_file.open(QFile::WriteOnly | QFile::Truncate)) {
			sites_file.write(QByteArray::fromStdString(cp::RpcValue(m_sites).toCpon("  ")));
			sites_file.close();
		}
		Q_EMIT sitesDownloaded();
	};
	QString url_scheme = QUrl(SitesProviderApp::instance()->remoteSitesUrl()).scheme();
	if (url_scheme.startsWith("shv")) {
		downloadSitesFromShv(this, on_sites_received);
	}
	else {
		downloadSitesByNetworkManager(this, on_sites_received);
	}
}

void AppRootNode::downloadSitesByNetworkManager(QObject *context, std::function<void(const shv::chainpack::RpcValue &)> callback)
{
	QNetworkAccessManager *network_manager = new QNetworkAccessManager(this);
	network_manager->setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);
	connect(network_manager, &QNetworkAccessManager::finished, context, [this, network_manager, callback](QNetworkReply *reply) {
		cp::RpcValue res;
		try {
			if (reply->error()) {
				SHV_QT_EXCEPTION("Download sites.json error:" + reply->errorString());
			}
			std::string sites_string = reply->readAll().toStdString();
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
			res = sites_cp_map.value("shv");
			if (!res.isMap()) {
				SHV_EXCEPTION("Shv key in sites.json must be map");
			}
		}
		catch (std::exception &e) {
			m_downloadSitesError = e.what();
		}
		reply->deleteLater();
		network_manager->deleteLater();
		callback(res);
	});
	network_manager->get(QNetworkRequest(SitesProviderApp::instance()->remoteSitesUrl()));
}

void AppRootNode::downloadSitesFromShv(QObject *context, std::function<void(const shv::chainpack::RpcValue &)> callback)
{
	shv::iotqt::rpc::RpcCall *rpc_call = shv::iotqt::rpc::RpcCall::create(SitesProviderApp::instance()->rpcConnection());
	QString url = SitesProviderApp::instance()->remoteSitesUrl();
	QString url_scheme = QUrl(url).scheme();
	rpc_call->setShvPath(url.mid(url_scheme.length() + 3))
			->setMethod("getSites");
	connect(rpc_call, &shv::iotqt::rpc::RpcCall::error, context, [this, callback](const QString &err) {
		m_downloadSitesError = err.toStdString();
		callback(cp::RpcValue());
	});
	connect(rpc_call, &shv::iotqt::rpc::RpcCall::result, context, [this, callback](const cp::RpcValue &res) {
		callback(res);
	});
	rpc_call->start();
}

bool AppRootNode::checkSites() const
{
	QString url_scheme = QUrl(SitesProviderApp::instance()->remoteSitesUrl()).scheme();
	return m_downloadSitesError.empty() && ((m_sitesSyncedBefore.isValid() &&
			m_sitesSyncedBefore.elapsed() < 3600 * 1000) || !url_scheme.startsWith("http"));
}

shv::chainpack::RpcValue AppRootNode::get(const shv::core::StringViewList &shv_path)
{
	return findSitesTreeValue(shv_path);
}

shv::chainpack::RpcValue AppRootNode::readFile(const QString &shv_path)
{
	QString filename = nodeLocalPath(shv_path);
	QFile f(filename);
	if (f.open(QFile::ReadOnly)) {
		QByteArray file_content = f.readAll();
		return file_content.toStdString();
	}
	return readConfig(filename).toCpon("  ");
}

shv::chainpack::RpcValue AppRootNode::writeFile(const QString &shv_path, const string &content)
{
	QString filename = nodeLocalPath(shv_path);
	QStringList shv_path_parts = shv_path.split('/');
	QString dirname = shv_path_parts.mid(0, shv_path_parts.count() - 1).join('/');
	if (!QDir(dirname).exists()) {
		QDir(nodeLocalPath(QString())).mkpath(dirname);
	}
	QFile f(filename);
	if (f.open(QFile::WriteOnly)) {
		qint64 n = f.write(content.data(), content.size());
		return n >= 0;
	}
	else {
		throw shv::coreqt::Exception("Cannot open file '" + shv_path + "' for writing.");
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
	QString file_path = QString::fromStdString(shv_path.join('/')) + "/" + QString::fromStdString(file_name);
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

QString AppRootNode::nodeFilesPath(const QString &shv_path) const
{
	return nodeLocalPath(shv_path) + "/" + QString::fromStdString(FILES_NODE);
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
