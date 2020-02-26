#include "application.h"
#include "appclioptions.h"
#include "devicelogrequest.h"
#include "devicemonitor.h"
#include "logdir.h"

#include <shv/coreqt/exception.h>
#include <shv/iotqt/rpc/rpc.h>
#include <shv/iotqt/rpc/rpcresponsecallback.h>
#include <shv/coreqt/log.h>
#include <shv/core/utils/shvjournalfilereader.h>
#include <shv/core/utils/shvjournalfilewriter.h>
#include <shv/core/utils/shvmemoryjournal.h>

namespace cp = shv::chainpack;
using namespace shv::core::utils;

DeviceLogRequest::DeviceLogRequest(const QString &shv_path, const QDateTime &since, const QDateTime &until, QObject *parent)
	: Super(parent)
	, m_shvPath(shv_path)
	, m_since(since)
	, m_until(until)
	, m_logDir(shv_path)
	, m_askElesys(true)
{
	m_askElesys = Application::instance()->deviceMonitor()->isElesysDevice(m_shvPath);
}

void DeviceLogRequest::getChunk()
{
	auto *conn = Application::instance()->deviceConnection();
	int rq_id = conn->nextRequestId();
	shv::iotqt::rpc::RpcResponseCallBack *cb = new shv::iotqt::rpc::RpcResponseCallBack(conn, rq_id, this);
	connect(cb, &shv::iotqt::rpc::RpcResponseCallBack::finished, this, &DeviceLogRequest::onChunkReceived);
	cb->start();
	if (m_askElesys) {
		if (m_since.date().daysTo(QDate::currentDate()) < 3) {
			m_askElesys = false;
		}
	}
	QString path;
	cp::RpcValue params;
	if (m_askElesys) {
		path = m_shvPath;
		if (Application::instance()->cliOptions()->test()) {
			if (path.startsWith("test/")) {
				path = path.mid(5);
			}
		}
		cp::RpcValue::Map param_map;
		param_map["shvPath"] = cp::RpcValue::fromValue(path);
		param_map["logParams"] = logParams().toRpcValue();

		path = QString::fromStdString(Application::instance()->cliOptions()->elesysPath());
		params = param_map;
	}
	else {
		path = "shv/" + m_shvPath;
		params = logParams().toRpcValue();
	}

	conn->callShvMethod(rq_id, path.toStdString(), "getLog", params);
}

void DeviceLogRequest::onChunkReceived(const shv::chainpack::RpcResponse &response)
{
	if (response.isError()) {
		shvError() << response.error().message();
		Q_EMIT finished(false);
		return;
	}

	const cp::RpcValue &result = response.result();
	ShvMemoryJournal log(logParams());
	log.loadLog(result);

	QDateTime first_record_time;
	QDateTime last_record_time;
	bool is_finished;
	ShvGetLogParams params = logParams();
	QDateTime until = m_until;
	if (log.entries().size()) {
		first_record_time = QDateTime::fromMSecsSinceEpoch(log.entries()[0].epochMsec, Qt::TimeSpec::UTC);
		last_record_time = QDateTime::fromMSecsSinceEpoch(log.entries()[log.entries().size() - 1].epochMsec, Qt::TimeSpec::UTC);
		is_finished = (int)log.entries().size() < Application::SINGLE_FILE_RECORD_COUNT;
		if (!is_finished) {
			until = last_record_time;
		}
	}
	else {
		is_finished = true;
	}

	if (is_finished && m_askElesys) {
		m_askElesys = false;
		is_finished = false;
	}
	shvInfo() << "received log for" << m_shvPath << "with" << log.entries().size() << "records"
			  << "since" << first_record_time
			<< "until" << last_record_time;

	QFile file(LogDir(m_shvPath).filePath(m_since));
	if (!file.open(QFile::WriteOnly)) {
		SHV_QT_EXCEPTION("Cannot open file " + file.fileName());
	}
	file.write(QByteArray::fromStdString(log.getLog(logParams()).toChainPack()));
	file.close();
	trimDirtyLog(until);
	m_since = until;

	if (is_finished) {
		Q_EMIT finished(true);
		return;
	}
	getChunk();
}


void DeviceLogRequest::exec()
{
	getChunk();
}

ShvGetLogParams DeviceLogRequest::logParams() const
{
	ShvGetLogParams params;
	params.since = cp::RpcValue::fromValue(m_since);
	params.until = cp::RpcValue::fromValue(m_until);
	params.maxRecordCount = Application::SINGLE_FILE_RECORD_COUNT;
	params.withSnapshot = true;
	return params;
}

void DeviceLogRequest::trimDirtyLog(const QDateTime &until)
{
	qint64 until_msec = until.toMSecsSinceEpoch();
	ShvLogHeader header;
	QString temp_filename = "dirty_new.log2";
	ShvJournalFileReader dirty_reader(m_logDir.dirtyLogPath().toStdString(), &header);
	if (m_logDir.exists(temp_filename)) {
		m_logDir.remove(temp_filename);
	}
	ShvJournalFileWriter dirty_writer(m_logDir.absoluteFilePath(temp_filename).toStdString());
	dirty_writer.append(ShvJournalEntry{
							"dirty",
							true,
							ShvJournalEntry::DOMAIN_VAL_CHANGE,
							ShvJournalEntry::NO_SHORT_TIME,
							ShvJournalEntry::SampleType::Continuous,
							until_msec
						});
	bool first = true;
	std::string data_missing_value;
	while (dirty_reader.next()) {
		const ShvJournalEntry &entry = dirty_reader.entry();
		if (entry.epochMsec > until_msec) {
			if (first) {
				first = false;
				if (!data_missing_value.empty()) {
						dirty_writer.append(ShvJournalEntry {
												ShvJournalEntry::PATH_DATA_MISSING,
												data_missing_value,
												ShvJournalEntry::DOMAIN_VAL_CHANGE,
												ShvJournalEntry::NO_SHORT_TIME,
												ShvJournalEntry::SampleType::Continuous,
												until_msec
											});
				}
			}
			dirty_writer.append(dirty_reader.entry());
		}
		else {
			if (entry.path == ShvJournalEntry::PATH_DATA_MISSING && entry.value.isString()) {
				data_missing_value = entry.value.toString();
			}
		}
	}
	m_logDir.remove(m_logDir.dirtyLogName());
	m_logDir.rename(temp_filename, m_logDir.dirtyLogName());
}
