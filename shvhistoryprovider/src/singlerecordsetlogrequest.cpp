#include "application.h"
#include "appclioptions.h"
#include "devicemonitor.h"
#include "singlerecordsetlogrequest.h"

#include <shv/iotqt/rpc/rpc.h>
#include <shv/coreqt/log.h>

namespace cp = shv::chainpack;
using namespace shv::core::utils;

SingleRecordSetLogRequest::SingleRecordSetLogRequest(const QString &shv_path, const QDateTime &since, const QDateTime &until, QObject *parent)
	: Super(parent)
	, m_shvPath(shv_path)
	, m_since(since)
	, m_until(until)
	, m_log(logParams())
{
}

void SingleRecordSetLogRequest::exec()
{
	if (m_since.date().daysTo(QDate::currentDate()) < 3 ||
		!Application::instance()->deviceMonitor()->isElesysDevice(m_shvPath)) {
		askDevice([this](){
			Q_EMIT finished(true);
		});
		return;
	}

	askElesysProvider([this]() {
		if ((int)m_log.entries().size() < Application::SINGLE_FILE_RECORD_COUNT) {
			if (m_log.entries().size()) {
				m_since = QDateTime::fromMSecsSinceEpoch(m_log.entries()[m_log.entries().size() - 1].epochMsec, Qt::TimeSpec::UTC);
			}
			askDevice([this]() {
				Q_EMIT finished(true);
			});
		}
		else {
			Q_EMIT finished(true);
		}
	});
}

void SingleRecordSetLogRequest::askDevice(VoidCallback callback)
{
	shvCall("shv/" + m_shvPath, "getLog", logParams().toRpcValue(), [this, callback](const cp::RpcValue &result) {
		try {
			if (m_log.entries().size()) {
				ShvMemoryJournal device_log;
				device_log.loadLog(result);
				for (const auto &entry : device_log.entries()) {
					m_log.append(entry);
				}
			}
			else {
				m_log.loadLog(result);
			}
		}
		catch (const shv::core::Exception &e) {
			error(QString::fromStdString(e.message()));
			return;
		}
		callback();
	});
}

void SingleRecordSetLogRequest::askElesysProvider(VoidCallback callback)
{
	try {
		QString path = m_shvPath;
		if (Application::instance()->cliOptions()->test()) {
			if (path.startsWith("test/")) {
				path = path.mid(5);
			}
		}
		cp::RpcValue::Map params;
		params["shvPath"] = cp::RpcValue::fromValue(path);
		shv::core::utils::ShvGetLogParams log_params = logParams();
		QDateTime until = rpcvalue_cast<QDateTime>(log_params.until);
		QDateTime elesys_latest = QDateTime::currentDateTimeUtc().addDays(-1);
		if (!until.isValid() || until > elesys_latest) {
			log_params.until = cp::RpcValue::fromValue(elesys_latest);
		}
		params["logParams"] = log_params.toRpcValue();
		shvCall(Application::instance()->elesysPath(), "getLog", params, [this, callback](const cp::RpcValue &result) {
			try {
				m_log.loadLog(result);
			}
			catch (const shv::core::Exception &e) {
				error(QString::fromStdString(e.message()));
				return;
			}
			callback();
		});
	}
	catch (const shv::core::Exception &e) {
		error(QString::fromStdString(e.message()));
	}
}

void SingleRecordSetLogRequest::shvCall(const QString &shv_path, const QString &method, const cp::RpcValue &params, ResultHandler callback)
{
	Application *app = Application::instance();
	app->shvCall(shv_path, method, params, [this, callback](const cp::RpcResponse &response) {
		if (response.isError()) {
			error(QString::fromStdString(response.error().message()));
		}
		else {
			try {
				callback(response.result());
			}
			catch (const shv::core::Exception &e) {
				error(e.what());
			}
		}
	});
}

shv::core::utils::ShvGetLogParams SingleRecordSetLogRequest::logParams() const
{
	shv::core::utils::ShvGetLogParams params;
	params.since = cp::RpcValue::fromValue(m_since);
	params.until = cp::RpcValue::fromValue(m_until);
	params.maxRecordCount = Application::SINGLE_FILE_RECORD_COUNT;
	params.withSnapshot = true;
	return params;
}
