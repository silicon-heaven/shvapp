#include "nodes.h"
#include "gitpushtask.h"
#include "sitesproviderapp.h"
#include "dirsynctask.h"
#include "appclioptions.h"

#include <shv/coreqt/log.h>
#include <shv/coreqt/exception.h>
#include <shv/iotqt/rpc/deviceconnection.h>
#include <shv/coreqt/rpc.h>

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QProcess>

namespace cp = shv::chainpack;

using namespace std;

const QString TEMPLATE_DIR = QStringLiteral("_template");
const QString TARGET_DIR = QStringLiteral("_target");
const QString FILES_NODE = QStringLiteral("_files");
const QString META_NODE = QStringLiteral("_meta");
const QString META_FILE = QStringLiteral("_meta.json");
const QString CPON_SUFFIX = QStringLiteral("cpon");
const QString CPTEMPL_SUFFIX = QStringLiteral("cptempl");

static const char METH_GIT_COMMIT[] = "gitCommit";
static const char METH_APP_VERSION[] = "version";
static const char METH_GET_SITES[] = "getSites";
static const char METH_RELOAD_SITES[] = "reloadSites";
static const char METH_SITES_SYNCED_BEFORE[] = "sitesSyncedBefore";
static const char METH_SITES_RELOADED[] = "reloaded";
static const char METH_FILE_READ[] = "read";
static const char METH_FILE_READ_COMPRESSED[] = "readCompressed";
static const char METH_FILE_WRITE[] = "write";
static const char METH_FILE_HASH[] = "hash";
static const char METH_PULL_FILES[] = "pullFilesFromDevice";
static const char METH_FILE_SIZE[] = "size";
static const char METH_FILE_SIZE_COMPRESSED[] = "sizeCompressed";
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
//	{ METH_RELOAD_SITES, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::None, shv::chainpack::Rpc::ROLE_COMMAND},
//	{ METH_SITES_SYNCED_BEFORE, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::None, shv::chainpack::Rpc::ROLE_READ },
//	{ METH_SITES_RELOADED, cp::MetaMethod::Signature::VoidParam, cp::MetaMethod::Flag::IsSignal, shv::chainpack::Rpc::ROLE_READ },
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
	{ METH_FILE_SIZE, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::None, shv::chainpack::Rpc::ROLE_BROWSE },
	{ METH_FILE_SIZE_COMPRESSED, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, shv::chainpack::Rpc::ROLE_BROWSE },
	{ METH_FILE_READ, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::None, shv::chainpack::Rpc::ROLE_READ },
	{ METH_FILE_READ_COMPRESSED, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, shv::chainpack::Rpc::ROLE_READ },
	{ METH_FILE_WRITE, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, shv::chainpack::Rpc::ROLE_WRITE },
	{ METH_FILE_HASH, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::None, shv::chainpack::Rpc::ROLE_READ },
};

static std::vector<cp::MetaMethod> data_meta_methods {
	{ cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, shv::chainpack::Rpc::ROLE_BROWSE },
	{ cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, shv::chainpack::Rpc::ROLE_BROWSE },
	{ cp::Rpc::METH_GET, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsGetter, shv::chainpack::Rpc::ROLE_READ },
};

enum class CompressionType {
	Invalid,
	QCompress,
};

static CompressionType compression_type_from_string(const std::string &type_str, CompressionType default_type)
{
	if (type_str.empty())
		return default_type;
	else if (type_str == "qcompress")
		return CompressionType::QCompress;
	else
		return CompressionType::Invalid;
}

AppRootNode::AppRootNode(QObject *parent)
	: Super(parent)
	, m_downloadSitesTimer(this)
{
	if (QDir(QString::fromStdString(SitesProviderApp::instance()->cliOptions()->localSitesDir()) + "/.git").exists()) {
		shvInfo() << "sites directory" << SitesProviderApp::instance()->cliOptions()->localSitesDir() << "identified as git repository";
		root_meta_methods.push_back({ METH_GIT_PUSH, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::None, shv::chainpack::Rpc::ROLE_ADMIN });
	}
	if (SitesProviderApp::instance()->cliOptions()->syncSites()) {
		root_meta_methods.push_back({ METH_RELOAD_SITES, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::None, shv::chainpack::Rpc::ROLE_COMMAND});
		root_meta_methods.push_back({ METH_SITES_SYNCED_BEFORE, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::None, shv::chainpack::Rpc::ROLE_READ });
		root_meta_methods.push_back({ METH_SITES_RELOADED, cp::MetaMethod::Signature::VoidParam, cp::MetaMethod::Flag::IsSignal, shv::chainpack::Rpc::ROLE_READ });

		m_downloadSitesTimer.setInterval(30 * 60 * 1000);
		connect(&m_downloadSitesTimer, &QTimer::timeout, this, &AppRootNode::downloadSites);
		m_downloadSitesTimer.start();
		QTimer::singleShot(500, this, &AppRootNode::downloadSites);
	}
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
	auto qshv_path = rq.shvPath().to<QString>();
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
		return getSites(rq.shvPath().to<QString>());
	}
	else if (method == METH_FILE_READ) {
		return readFile(rq.shvPath().to<QString>());
	}
	else if (method == METH_FILE_READ_COMPRESSED) {
		return readFileCompressed(rq);
	}
	else if (method == METH_FILE_SIZE) {
		return readFile(rq.shvPath().to<QString>()).asData().second;
	}
	else if (method == METH_FILE_SIZE_COMPRESSED) {
		return readFileCompressed(rq).asData().second;
	}
	else if (method == METH_FILE_WRITE) {
		cp::RpcValue params = rq.params();
		if(!params.isString())
			throw shv::core::Exception("Content must be string.");
		return writeFile(qshv_path, params.asString());
	}
	else if (method == METH_FILE_MK) {
		return mkFile(qshv_path, rq.params());
	}
	else if (method == METH_PULL_FILES) {
		DirSyncTask *sync_task = new DirSyncTask(QString(), this);
		if (qshv_path.endsWith('/' + FILES_NODE)) {
			sync_task->addDir(qshv_path);
		}
		else if (isDevice(qshv_path)) {
			sync_task->addDir(qshv_path + '/' + FILES_NODE);
		}
		else {
			QStringList devices;
			findDevicesToSync(qshv_path, devices);
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
		string bytes = readFile(qshv_path).toString();
		QCryptographicHash h(QCryptographicHash::Sha1);
#if QT_VERSION_MAJOR >= 6
		using byte_array_view_t = QByteArrayView;
#else
		using byte_array_view_t = QByteArray;
#endif
		h.addData(byte_array_view_t(bytes.data(), static_cast<int>(bytes.size())));
		return h.result().toHex().toStdString();
	}
	else if (method == cp::Rpc::METH_GET) {
		return metaValue(qshv_path);
	}
	else if (method == cp::Rpc::METH_DEVICE_ID) {
		SitesProviderApp *app = SitesProviderApp::instance();
		cp::RpcValue::Map opts = app->rpcConnection()->connectionOptions().asMap();;
		cp::RpcValue::Map dev = opts.value(cp::Rpc::KEY_DEVICE).asMap();
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
		GitPushTask *git_task = new GitPushTask(QString::fromStdString(rq.userId().asString()), this);
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
	else if (method == cp::Rpc::METH_LS) {
		cp::RpcValue::List res;
		for (const QString &s : lsNode(qshv_path)) {
			res.push_back(s.toStdString());
		}
		return res;
	}
	return Super::callMethodRq(rq);
}

const std::vector<shv::chainpack::MetaMethod> &AppRootNode::metaMethods(const shv::iotqt::node::ShvNode::StringViewList &shv_path_list)
{
	if (shv_path_list.empty()) {
		return root_meta_methods;
	}
	QString shv_path = QString::fromStdString(shv_path_list.join('/'));
	cp::RpcValue meta = metaValue(shv_path);
	if (meta.isValid()) {
		if (meta.isMap()) {
			return meta_meta_methods;
		}
		else {
			return data_meta_methods;
		}
	}
	else if (shv_path.contains('/' + FILES_NODE)) {
		if (isFile(shv_path)) {
			return file_meta_methods;
		}
		else {
			if (shv_path.endsWith('/' + FILES_NODE) && isDevice(shv_path.chopped(FILES_NODE.length() + 1))) {
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

shv::chainpack::RpcValue AppRootNode::metaValue(const QString &shv_path)
{
	cp::RpcValue meta;
	qsizetype meta_ix = shv_path.indexOf('/' + META_NODE);
	if (meta_ix != -1) {
		QString path_rest = shv_path.mid(meta_ix + 1 + META_NODE.length());
		if (path_rest.isEmpty() || path_rest[0] == '/') {
			QFile meta_file(nodeMetaPath(shv_path.mid(0, meta_ix)));
			if (meta_file.open(QFile::ReadOnly)) {
				QByteArray meta_content = meta_file.readAll();
				meta = cp::RpcValue::fromCpon(meta_content.toStdString());
				meta_file.close();
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
				QStringList meta_keys = path_rest.split('/', QString::SkipEmptyParts);
#else
				QStringList meta_keys = path_rest.split('/', Qt::SkipEmptyParts);
#endif
				for (int i = 0; i < meta_keys.count(); ++i) {
					meta = meta.at(meta_keys[i].toStdString());
				}
			}
		}
	}
	return meta;
}

QString AppRootNode::nodeMetaPath(const QString &shv_path) const
{
	return nodeLocalPath(shv_path) + "/" + META_FILE;
}

cp::RpcValue AppRootNode::getSites(const QString &shv_path)
{
	cp::RpcValue::Map res;
	QDir dir(nodeLocalPath(shv_path));
	for (const QFileInfo &info : dir.entryInfoList(QDir::Filter::AllEntries | QDir::Filter::NoDotAndDotDot)) {
		if (info.fileName() == META_FILE) {
			QFile meta_file(info.filePath());
			if (meta_file.open(QFile::ReadOnly)) {
				std::string err;
				res[META_NODE.toStdString()] = cp::RpcValue::fromCpon(meta_file.readAll().toStdString(), &err);
				if (!err.empty()) {
					res.erase(res.find(META_NODE.toStdString()));
					shvWarning() << "Invalid json file" << meta_file.fileName() << err;
				}
			}
			else {
				shvWarning() << "Cannot open" << meta_file.fileName() << "for reading";
			}
		}
		else if (info.isDir() && !info.fileName().startsWith('_')) {
			res[info.fileName().toStdString()] = getSites(shv_path + "/" + info.fileName());
		}
	}
	return res;
}

void AppRootNode::findDevicesToSync(const QString &shv_path, QStringList &result)
{
	QStringList ls_result = lsNode(shv_path);
	for (const QString &ls_result_item : ls_result) {
		QString new_shv_path = shv_path + '/' + ls_result_item;
		if (ls_result_item[0] == '_') {
			if (ls_result_item == FILES_NODE && isDevice(shv_path)) {
				result << new_shv_path;
			}
		}
		else {
			findDevicesToSync(new_shv_path, result);
		}
	}
}

QStringList AppRootNode::lsNode(const QString &shv_path)
{
	QStringList ret;
	cp::RpcValue meta = metaValue(shv_path);
	if (meta.isValid()) {
		for (const std::string &meta_item : meta.asMap().keys()) {
			ret.push_back(QString::fromStdString(meta_item));
		}
	}
	else {
		for (const QString &dir_item : lsDir(shv_path)) {
			if (dir_item == META_FILE) {
				ret.push_back(META_NODE);
			}
			else {
				ret.push_back(dir_item);
			}
		}
	}
	return ret;
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

bool AppRootNode::isDevice(const QString &shv_path)
{
	QString file_path = nodeLocalPath(shv_path);
	QFileInfo fi(file_path);
	if (fi.isDir()) {
		QStringList entries = QDir(file_path).entryList(QDir::Filter::AllEntries | QDir::Filter::NoDotAndDotDot);
		if ((entries.count() == 1 || (entries.count() == 2 && entries.contains(FILES_NODE))) && entries.contains(META_FILE)) {
			return metaValue(shv_path + '/' + META_NODE).asMap().hasKey("type");
		}
	}
	return false;
}

void AppRootNode::onSitesDownloaded()
{
	m_sitesSyncedBefore.start();
	if (SitesProviderApp::instance()->rpcConnection()->isBrokerConnected()) {
		cp::RpcSignal ntf;
		ntf.setMethod(METH_SITES_RELOADED);
		rootNode()->emitSendRpcMessage(ntf);
	}
	Q_EMIT sitesDownloaded();
}

void AppRootNode::downloadSites()
{
	if (m_downloadingSites) {
		return;
	}
	m_downloadingSites = true;

	QDir("/tmp/sites").removeRecursively();

	QProcess *git = new QProcess(this);
	git->setWorkingDirectory("/tmp");
	QStringList arguments;
	arguments << "clone" << SitesProviderApp::instance()->remoteSitesUrl();
	connect(git, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this, [this, git](int exit_code, QProcess::ExitStatus exit_status) {
		git->deleteLater();
		if (exit_status == QProcess::ExitStatus::CrashExit) {
			shvError() << git->errorString();
		}
		else if (exit_code) {
			shvError() << git->readAllStandardError();
		}
		else {
			mergeSitesDirs(nodeLocalPath(), "/tmp/sites/sites");
			QDir("/tmp/sites").removeRecursively();
			onSitesDownloaded();
		}
		m_downloadingSites = false;
	});
	connect(git, &QProcess::errorOccurred, this, [this, git](QProcess::ProcessError error) {
		if (error == QProcess::FailedToStart) {
			git->deleteLater();
			shvError() << git->errorString();
			m_downloadingSites = false;
		}
	});
	git->start("git", arguments);
}

void AppRootNode::mergeSitesDirs(const QString &files_path, const QString &git_path)
{
	QDir files_dir(files_path);
	QDir git_dir(git_path);

	QFileInfoList files_entries = files_dir.entryInfoList(QDir::Filter::AllEntries | QDir::Filter::NoDotAndDotDot);
	QFileInfoList git_entries = git_dir.entryInfoList(QDir::Filter::AllEntries | QDir::Filter::NoDotAndDotDot);

	for (const QFileInfo &git_entry : git_entries) {
		if (git_entry.fileName() != TEMPLATE_DIR && git_entry.fileName() != TARGET_DIR) {
			bool is_in_files = false;
			for (int i = 0; i < files_entries.count(); ++i) {
				const QFileInfo &files_entry = files_entries[i];
				if (git_entry.fileName() == files_entry.fileName()) {
					is_in_files = true;
					files_entries.removeAt(i);
					break;
				}
			}
			if (is_in_files) {
				if (git_entry.isDir()) {
					mergeSitesDirs(files_path + '/' + git_entry.fileName(), git_path + '/' + git_entry.fileName());
				}
				else {
					QFile::remove(files_dir.filePath(git_entry.fileName()));
					QFile::rename(git_entry.filePath(), files_dir.filePath(git_entry.fileName()));
				}
			}
			else {
				if (git_entry.isDir()) {
					files_dir.mkdir(git_entry.fileName());
					mergeSitesDirs(files_path + '/' + git_entry.fileName(), git_path + '/' + git_entry.fileName());
				}
				else {
					QFile::rename(git_entry.filePath(), files_dir.filePath(git_entry.fileName()));
				}
			}
		}
	}

	for (const QFileInfo &files_entry : files_entries) {
		if ((files_entry.isDir() && files_entry.fileName() != FILES_NODE) || files_entry.fileName() == META_FILE) {
			QDir(files_path + '/' + files_entry.fileName()).removeRecursively();
		}
	}
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

shv::chainpack::RpcValue AppRootNode::readFileCompressed(const shv::chainpack::RpcRequest &request)
{
	const std::string compression_type_str = request.params().asMap().value("compressionType").toString();
	const CompressionType compression_type = compression_type_from_string(compression_type_str, CompressionType::QCompress);
	if (compression_type == CompressionType::Invalid) {
		SHV_EXCEPTION("Invalid compress type: " + compression_type_str + " on path: " + request.shvPath().asString());
	}

	cp::RpcValue result;
	const auto blob = readFile(request.shvPath().to<QString>()).asData();
	if (compression_type == CompressionType::QCompress) {
		const auto compressed_blob = qCompress(QByteArray::fromRawData(blob.first, static_cast<int>(blob.second)));
		result = cp::RpcValue::Blob(compressed_blob.cbegin(), compressed_blob.cend());
	}

	return result;
}

shv::chainpack::RpcValue AppRootNode::mkFile(const QString &shv_path, const shv::chainpack::RpcValue &params)
{
	std::string error;
	QString file_name;
	bool has_content = false;
	if (params.isString()) {
		file_name = params.to<QString>();

	}
	else if(params.isList()) {
		const cp::RpcValue::List &lst = params.asList();
		if (lst.size() != 2)
			throw shv::core::Exception("Invalid params, [\"name\", \"content\"] expected.");
		file_name = lst[0].to<QString>();
		has_content = true;
	}
	if (file_name.isEmpty())
		throw shv::core::Exception("File name is empty.");
	QString file_path = shv_path + '/' + file_name;
	if(has_content)
		return writeFile(file_path, params.asList()[1].toString());
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
		QString templ_path = QString::fromStdString(config.asMap().value(BASED_ON_KEY).asString());
		if (!templ_path.isEmpty()) {
			QStringList templ_path_parts = templ_path.split('/');
			templ_path_parts.insert(templ_path_parts.count() - 1, FILES_NODE);
			templ_path = templ_path_parts.join('/');
			if (templ_path.startsWith('/')) {
				templ_path = nodeLocalPath(templ_path);
			}
			else {
				templ_path = QFileInfo(file.fileName()).absolutePath() + "/../" + templ_path;
			}
			cp::RpcValue::Map file_content_map = config.asMap();
			file_content_map.erase(BASED_ON_KEY);
			cp::RpcValue templ = readConfig(templ_path);
			return shv::chainpack::Utils::mergeMaps(templ, file_content_map);
		}
	}
	return config;
}

QString AppRootNode::nodeFilesPath(const QString &shv_path) const
{
	return nodeLocalPath(shv_path) + '/' + FILES_NODE;
}

QString AppRootNode::nodeLocalPath(const QString &shv_path) const
{
	QString path = QString::fromStdString(SitesProviderApp::instance()->cliOptions()->localSitesDir());
	if (!path.endsWith('/') && !shv_path.startsWith('/') && !shv_path.isEmpty()) {
		path += '/';
	}
	return path + shv_path;
}

bool AppRootNode::isFile(const QString &shv_path)
{
	QString filename = nodeLocalPath(shv_path);
	QFileInfo fi(filename);
	if (fi.isFile())
		return true;

	if (filename.endsWith(CPON_SUFFIX)) {
		filename.append("." + CPTEMPL_SUFFIX);
		QFileInfo fi_with_suffix(filename);
		if (fi_with_suffix.isFile())
			return true;
	}
	return false;
}

bool AppRootNode::isDir(const QString &shv_path)
{
	QFileInfo fi(nodeLocalPath(shv_path));
	return fi.isDir();
}
