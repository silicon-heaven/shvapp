#include "application.h"
#include "appclioptions.h"
#include "devicemonitor.h"
#include "getlogmerge.h"
#include "rootnode.h"
#include "siteitem.h"
#include "logdir.h"
#include "logsanitizer.h"

#include <shv/chainpack/metamethod.h>
#include <shv/coreqt/log.h>
#include <shv/coreqt/exception.h>
#include <shv/iotqt/rpc/deviceconnection.h>
#include <shv/iotqt/node/shvnodetree.h>
#include <shv/coreqt/rpc.h>
#include <shv/core/utils/shvjournalfilereader.h>
#include <shv/core/utils/shvlogfilereader.h>
#include <shv/core/utils/shvlogrpcvaluereader.h>

#include <QElapsedTimer>
#include <QScopeGuard>

namespace cp = shv::chainpack;
using namespace shv::core::utils;

static const char METH_GET_VERSION[] = "version";
static const char METH_GIT_COMMIT[] = "gitCommit";
static const char METH_RELOAD_SITES[] = "reloadSites";
static const char METH_GET_UPTIME[] = "uptime";
static const char METH_GET_LOGVERBOSITY[] = "logVerbosity";
static const char METH_SET_LOGVERBOSITY[] = "setLogVerbosity";
static const char METH_TRIM_DIRTY_LOG[] = "trim";
static const char METH_SANITIZE_LOG_CACHE[] = "sanitizeLogCache";
static const char METH_CHECK_LOG_CACHE[] = "checkLogCache";
static const char METH_PUSH_LOG[] = "pushLog";

static std::vector<cp::MetaMethod> root_meta_methods {
	{ cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_BROWSE },
	{ cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_BROWSE },
	{ cp::Rpc::METH_DEVICE_ID, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsGetter, cp::Rpc::ROLE_READ },
	{ cp::Rpc::METH_APP_NAME, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsGetter, cp::Rpc::ROLE_READ },
	{ METH_GET_VERSION, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsGetter, cp::Rpc::ROLE_READ },
	{ METH_GIT_COMMIT, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsGetter, cp::Rpc::ROLE_READ},
	{ METH_RELOAD_SITES, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_COMMAND },
	{ METH_GET_UPTIME, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsGetter, cp::Rpc::ROLE_READ },
	{ METH_GET_LOGVERBOSITY, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsGetter, cp::Rpc::ROLE_READ },
	{ METH_SET_LOGVERBOSITY, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::IsSetter, cp::Rpc::ROLE_COMMAND },
};

static std::vector<cp::MetaMethod> branch_meta_methods {
	{ cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_BROWSE },
	{ cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_BROWSE },
	{ cp::Rpc::METH_GET_LOG, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_READ },
};

static std::vector<cp::MetaMethod> normal_leaf_meta_methods {
	{ cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_BROWSE },
	{ cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_BROWSE },
	{ cp::Rpc::METH_GET_LOG, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_READ },
	{ METH_SANITIZE_LOG_CACHE, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_SERVICE },
	{ METH_CHECK_LOG_CACHE, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_SERVICE,
				"Checks state of file cache. Use param {\"withGoodFiles\": true} or 1 to see complete output" },
};

static std::vector<cp::MetaMethod> push_log_leaf_meta_methods {
	{ cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_BROWSE },
	{ cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_BROWSE },
	{ cp::Rpc::METH_GET_LOG, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_READ },
	{ METH_CHECK_LOG_CACHE, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_SERVICE,
				"Checks state of file cache. Use param {\"withGoodFiles\": true} or 1 to see complete output" },
	{ METH_SANITIZE_LOG_CACHE, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_SERVICE },
	{ METH_PUSH_LOG, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_WRITE },
};

static std::vector<cp::MetaMethod> dirty_log_meta_methods {
	{ cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_BROWSE },
	{ cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_BROWSE },
	{ METH_TRIM_DIRTY_LOG, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_READ },
};

static std::vector<cp::MetaMethod> start_ts_meta_methods {
	{ cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_BROWSE },
	{ cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_BROWSE },
	{ cp::Rpc::METH_GET, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsGetter, cp::Rpc::ROLE_READ },
	{ cp::Rpc::SIG_VAL_CHANGED, cp::MetaMethod::Signature::VoidParam, cp::MetaMethod::Flag::IsSignal, cp::Rpc::ROLE_READ },
};

RootNode::RootNode(QObject *parent)
	: Super(parent)
{
}

const std::vector<shv::chainpack::MetaMethod> &RootNode::metaMethods(const shv::iotqt::node::ShvNode::StringViewList &shv_path)
{
	if (shv_path.empty()) {
		return root_meta_methods;
	}
	else {
		if (shv_path[shv_path.size() - 1] == Application::DIRTY_LOG_NODE) {
			return dirty_log_meta_methods;
		}
		else if (shv_path[shv_path.size() - 1] == Application::START_TS_NODE) {
			return start_ts_meta_methods;
		}
		else if (!Application::instance()->deviceMonitor()->monitoredDevices().contains(QString::fromStdString(shv_path.join('/')))) {
			return branch_meta_methods;
		}
		else if (Application::instance()->deviceMonitor()->isPushLogDevice(QString::fromStdString(shv_path.join('/')))) {
			return  push_log_leaf_meta_methods;
		}
		return normal_leaf_meta_methods;
	}
}

size_t RootNode::methodCount(const StringViewList &shv_path)
{
	return metaMethods(shv_path).size();
}

const shv::chainpack::MetaMethod *RootNode::metaMethod(const StringViewList &shv_path, size_t ix)
{
	const std::vector<cp::MetaMethod> &meta_methods = metaMethods(shv_path);
	if (meta_methods.size() <= ix) {
		SHV_EXCEPTION("Invalid method index: " + std::to_string(ix) + " of: " + std::to_string(meta_methods.size()));
	}
	return &(meta_methods[ix]);
}

shv::chainpack::RpcValue RootNode::ls(const shv::core::StringViewList &shv_path, const shv::chainpack::RpcValue &params)
{
	Q_UNUSED(params);
	return ls(shv_path, 0, Application::instance()->deviceMonitor()->sites());
}

cp::RpcValue RootNode::ls(const shv::core::StringViewList &shv_path, size_t index, const SiteItem *site_item)
{
	if (shv_path.size() - index == 0) {
		cp::RpcValue::List items;
		for (int i = 0; i < site_item->children().count(); ++i) {
			items.push_back(cp::RpcValue::fromValue(site_item->children()[i]->objectName()));
		}
		if (site_item->children().count() == 0) {
			const SitesHPDevice *hp_site_item = qobject_cast<const SitesHPDevice*>(site_item);
			if (hp_site_item && !hp_site_item->isPushLog()) {
				items.push_back(Application::DIRTY_LOG_NODE);
			}
		}
		return items;
	}
	QString key = QString::fromStdString(shv_path[index].toString());
	SiteItem *child = site_item->findChild<SiteItem*>(key);
	if (child) {
		return ls(shv_path, index + 1, child);
	}
	if (index + 1 == shv_path.size() && key == Application::DIRTY_LOG_NODE) {
		cp::RpcValue::List items;
		items.push_back(Application::START_TS_NODE);
		return items;
	}
	return cp::RpcValue::List();
}

void RootNode::trimDirtyLog(const QString &shv_path)
{
	Application::instance()->logSanitizer()->trimDirtyLog(shv_path);
}

void RootNode::pushLog(const QString &shv_path, const shv::chainpack::RpcValue &log, int64_t &log_since_ms, int64_t &log_until_ms)
{
	QString log_path = "/tmp/historyprovider/pushlog/" + shv_path;
	if (QDir().mkpath(log_path)) {
		QFile log_file(log_path + "/last_data.log");
		if (log_file.open(QFile::WriteOnly | QFile::Truncate)) {
			log_file.write(QByteArray::fromStdString(log.toCpon("  ")));
			log_file.close();
		}
		else {
			shvWarning() << "Cannot create file for logging of push log data";
		}
	}
	else {
		shvWarning() << "Cannot create directory for logging of push log data" << log_path;
	}
	ShvLogRpcValueReader log_reader(log, true);
	LogDir log_dir(shv_path);
	QDateTime log_since = log_reader.logHeader().since().to<QDateTime>();
	QDateTime log_until = log_reader.logHeader().until().to<QDateTime>();
	shvInfo() << "pushLog request from shv_path:" << shv_path << "since:" << log_since.toString(Qt::ISODate).toStdString()
			  << "until:" << log_until.toString(Qt::ISODate).toStdString() << "recordCount:" << log.asList().size();
	bool with_snapshot = log_reader.logHeader().withSnapShot();
	auto typeinfo = log_reader.logHeader().typeInfo();
	bool has_type_info = typeinfo.isValid();

	if (!log_since.isValid() || !log_until.isValid()) {
		SHV_EXCEPTION("Invalid log - missing since or until");
	}
	QStringList matching_files = log_dir.findFiles(log_since, log_until);

	log_since_ms = log_since.toMSecsSinceEpoch();
	if(log_since_ms == 0)
		SHV_EXCEPTION("Pushed log 'since' param is invalid.");
	log_until_ms = log_until.toMSecsSinceEpoch();
	if(log_until_ms == 0)
		SHV_EXCEPTION("Pushed log 'until' param is invalid.");

	QVector<ShvJournalEntry> entries_to_prepend;
	QVector<ShvJournalEntry> entries_to_append;

	if (matching_files.count()) {
		ShvLogFileReader first_file(matching_files[0].toStdString());
		if (first_file.logHeader().since().toDateTime().msecsSinceEpoch() < log_since_ms) {
			log_since = first_file.logHeader().since().to<QDateTime>();
			with_snapshot = first_file.logHeader().withSnapShot();
			while (first_file.next()) {
				const ShvJournalEntry &entry = first_file.entry();
				if (entry.epochMsec < log_since_ms) {
					entries_to_prepend << entry;
				}
				else {
					break;
				}
			}
		}
		ShvLogFileReader last_file(matching_files.last().toStdString());
		if (last_file.logHeader().until().toDateTime().msecsSinceEpoch() >= log_until_ms) {
			log_until = last_file.logHeader().until().to<QDateTime>();
			while (last_file.next()) {
				const ShvJournalEntry &entry = first_file.entry();
				if (entry.epochMsec >= log_until_ms) {
					entries_to_append << entry;
				}
			}
		}
	}

	ShvGetLogParams params;
	params.recordCountLimit = Application::CHUNK_RECORD_COUNT;
	params.withSnapshot = with_snapshot;
	params.withTypeInfo = has_type_info;
	params.since = cp::RpcValue::fromValue(log_since);
	params.until = cp::RpcValue::fromValue(log_until);

	bool first_log = log_dir.findFiles(QDateTime(), QDateTime()).count() - matching_files.count() == 0;

	QStringList new_files;
	auto cleanup = qScopeGuard([&new_files]{
		for (const QString &file : new_files) {
			QFile(file).remove();
		}
	});

	ShvMemoryJournal output_log;
	auto save_log = [&output_log, &shv_path, &params, &first_log, &new_files, &typeinfo]() {
		output_log.setTypeInfo(typeinfo);
		cp::RpcValue log_cp = output_log.getLog(params);
		if (first_log) {
			log_cp.setMetaValue("HP", cp::RpcValue::Map{{ "firstLog", true }});
			first_log = false;
		}
		QDateTime since = log_cp.metaValue("since").to<QDateTime>();
		QFile file(LogDir(shv_path).filePath(since) + ".new");
		if (!file.open(QFile::WriteOnly)) {
			SHV_QT_EXCEPTION("Cannot open file " + file.fileName());
		}
		new_files << file.fileName();
		file.write(QByteArray::fromStdString(log_cp.toChainPack()));
		file.close();
		params.since = log_cp.metaValue("until");
		output_log = ShvMemoryJournal();
	};

	auto append_log = [&output_log, &save_log](const ShvJournalEntry &entry) {
		output_log.append(entry);
		if (static_cast<int>(output_log.size()) >= Application::CHUNK_RECORD_COUNT) {
			save_log();
		}

	};
	for (const ShvJournalEntry &entry : entries_to_prepend) {
		append_log(entry);
	}
	while (log_reader.next()) {
		const ShvJournalEntry &entry = log_reader.entry();
		auto msec = entry.epochMsec;
		if (log_since_ms > msec) {
			SHV_EXCEPTION("Pushed log entry date-time is earlier than 'since' param.");
		}
		if (log_until_ms < msec) {
			SHV_EXCEPTION("Pushed log entry date-time is past the 'until' param.");
		}
		append_log(entry);
	}
	for (const ShvJournalEntry &entry : entries_to_append) {
		append_log(entry);
	}
	if (output_log.size()) {
		save_log();
	}
	for (const QString &file : matching_files) {
		QFile(file).remove();
	}
	for (const QString &new_file : new_files) {
		QString regular_filename = new_file.chopped(4);
		QFile regular_file(regular_filename);
		if (regular_file.exists()) {
			regular_file.remove();
		}
		QFile(new_file).rename(regular_filename);
	}
	new_files.clear();
}

shv::chainpack::RpcValue RootNode::getStartTS(const QString &shv_path)
{
	LogDir log_dir(shv_path);
	ShvJournalFileReader dirty_log(log_dir.dirtyLogPath().toStdString());
	if (dirty_log.next()) {
		return dirty_log.entry().dateTime();
	}
	return cp::RpcValue(nullptr);
}

void RootNode::onRpcMessageReceived(const shv::chainpack::RpcMessage &msg)
{
	if (msg.isRequest()) {
		cp::RpcRequest rq(msg);
		cp::RpcResponse resp = cp::RpcResponse::forRequest(rq.metaData());
		try {
			rq.setShvPath(rq.shvPath().asString());
			shv::chainpack::RpcValue result = processRpcRequest(rq);
			if (result.isValid()) {
				resp.setResult(result);
			}
			else {
				return;
			}
		}
		catch (const std::exception &e) {
			resp.setError(cp::RpcResponse::Error::create(cp::RpcResponse::Error::MethodCallException, e.what()));
		}
		if (resp.requestId().toInt() > 0) { // RPC calls with requestID == 0 does not expect response
			sendRpcMessage(resp);
		}
	}
}

shv::chainpack::RpcValue RootNode::callMethod(const shv::iotqt::node::ShvNode::StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params, const shv::chainpack::RpcValue &user_id)
{
	if (method == cp::Rpc::METH_APP_NAME) {
		return Application::instance()->applicationName().toStdString();
	}
	if (method == cp::Rpc::METH_DEVICE_ID) {
		return Application::instance()->cliOptions()->deviceId();
	}
	else if (method == METH_GET_VERSION) {
		return Application::instance()->applicationVersion().toStdString();
	}
	else if(method == METH_GIT_COMMIT) {
#ifdef GIT_COMMIT
		return SHV_EXPAND_AND_QUOTE(GIT_COMMIT);
#else
		return "N/A";
#endif
	}
	else if (method == METH_GET_UPTIME) {
		return Application::instance()->uptime().toStdString();
	}
	else if (method == cp::Rpc::METH_GET_LOG) {
		return getLog(QString::fromStdString(shv_path.join('/')), params);
	}
	else if (method == METH_RELOAD_SITES) {
		Application::instance()->deviceMonitor()->downloadSites();
		return true;
	}
	else if (method == METH_SANITIZE_LOG_CACHE) {
		Application *app = Application::instance();
		QString q_shv_path = QString::fromStdString(shv_path.join('/'));
		if (app->deviceMonitor()->isPushLogDevice(q_shv_path) && app->deviceMonitor()->syncLogBroker(q_shv_path).isEmpty()) {
			SHV_QT_EXCEPTION("Cannot sanitize logs on pushLog device");
		}
		return app->logSanitizer()->sanitizeLogCache(q_shv_path, CheckType::CheckDirtyLogState);
	}
	else if (method == METH_CHECK_LOG_CACHE) {
		bool with_good_files = false;
		if (params.isValid()) {
			if (params.isInt()) {
				if (params.toInt() == 1) {
					with_good_files = true;
				}
			}
			else if (params.isMap()) {
				with_good_files = params.asMap().value("withGoodFiles").toBool();
			}
		}
		CacheState info = CheckLogTask::checkLogCache(QString::fromStdString(shv_path.join('/')), with_good_files);
		cp::RpcValue::Map res;
		cp::RpcValue::List res_files;
		for (const auto &file : info.files) {
			cp::RpcValue::Map res_file;
			res_file["fileName"] = file.fileName.toStdString();
			res_file["status"] = cacheStatusToString(file.status);
			res_file["recordCount"] = file.recordCount;
			if (file.errors.count()) {
				cp::RpcValue::List res_errs;
				for (const auto &err : file.errors) {
					cp::RpcValue::Map res_err;
					res_err["status"] = cacheStatusToString(err.status);
					res_err["type"] = CacheError::typeToString(err.type);
					res_err["description"] = err.description.toStdString();
					res_errs.push_back(res_err);
				}
				res_file["errors"] = res_errs;
			}
			res_files.push_back(res_file);
		}
		res["files"] = res_files;
		res["recordCount"] = info.recordCount;
		res["fileCount"] = info.fileCount;
		res["since"] = cp::RpcValue::fromValue(info.since);
		res["until"] = cp::RpcValue::fromValue(info.until);
		return res;
	}
	else if (method == METH_TRIM_DIRTY_LOG && shv_path.size() && shv_path.value(-1) == Application::DIRTY_LOG_NODE) {
		shv::iotqt::node::ShvNode::StringViewList path = shv_path.mid(0, shv_path.size() - 1);
		trimDirtyLog(QString::fromStdString(path.join('/')));
		return true;
	}
	else if (method == cp::Rpc::METH_GET && shv_path.size() > 2 && shv_path.value(-1) == Application::START_TS_NODE) {
		shv::iotqt::node::ShvNode::StringViewList path = shv_path.mid(0, shv_path.size() - 2);
		return getStartTS(QString::fromStdString(path.join('/')));
	}
	else if (method == METH_GET_LOGVERBOSITY) {
		return NecroLog::topicsLogThresholds();
	}
	else if (method == METH_SET_LOGVERBOSITY) {
		const std::string &s = params.asString();
		NecroLog::setTopicsLogThresholds(s);
		return true;
	}
	else if (method == METH_PUSH_LOG) {
		Application *app = Application::instance();
		QString q_shv_path = QString::fromStdString(shv_path.join('/'));
		if (!app->deviceMonitor()->isPushLogDevice(q_shv_path)) {
			SHV_QT_EXCEPTION("Cannot push log to this device");
		}

		int64_t since = 0LL;
		int64_t until = 0LL;
		std::string err = "success";
		try {
			pushLog(q_shv_path, params, since, until);
		}
		catch(const std::exception &e) {
			err = e.what();
			shvWarning() << err;
		}
		cp::RpcValue::Map result;
		result["since"] = since == 0LL ? cp::RpcValue(nullptr) : cp::RpcValue::DateTime::fromMSecsSinceEpoch(since);
		result["until"] = until == 0LL ? cp::RpcValue(nullptr) : cp::RpcValue::DateTime::fromMSecsSinceEpoch(until);
		result["msg"] = err;
		return result;
	}
	return Super::callMethod(shv_path, method, params, user_id);
}

cp::RpcValue RootNode::getLog(const QString &shv_path, const shv::chainpack::RpcValue &params) const
{
	shv::core::utils::ShvGetLogParams log_params;
	if (params.isValid()) {
		if (!params.isMap()) {
			SHV_QT_EXCEPTION("Bad param format");
		}
		log_params = params;
	}

	if (log_params.since.isValid() && log_params.until.isValid() && log_params.since.toDateTime() > log_params.until.toDateTime()) {
		SHV_QT_EXCEPTION("since is after until");
	}
	static int static_request_no = 0;
	int request_no = static_request_no++;
	QElapsedTimer tm;
	tm.start();
	shvMessage() << "got request" << request_no << "for log" << shv_path << "with params:\n" << log_params.toRpcValue().toCpon("    ");
	if (log_params.recordCountLimit > shv::core::utils::AbstractShvJournal::DEFAULT_GET_LOG_RECORD_COUNT_LIMIT) {
		log_params.recordCountLimit = shv::core::utils::AbstractShvJournal::DEFAULT_GET_LOG_RECORD_COUNT_LIMIT;
	}
	GetLogMerge request(shv_path, log_params);
	try {
		shv::chainpack::RpcValue result = request.getLog();
		shvMessage() << "request number" << request_no << "finished in" << tm.elapsed() << "ms with" << result.asList().size() << "records";
		return result;
	}
	catch (const shv::core::Exception &e) {
		shvMessage() << "request number" << request_no << "finished in" << tm.elapsed() << "ms with error" << e.message();
		throw;
	}
}
