#include "application.h"
#include "appclioptions.h"
#include "devicemonitor.h"
#include "devicelogrequest.h"
#include "logdir.h"

#include <shv/core/utils/shvjournalfilereader.h>
#include <shv/core/utils/shvjournalfilewriter.h>
#include <shv/core/utils/shvlogfilereader.h>
#include <shv/core/utils/shvmemoryjournal.h>
#include <shv/coreqt/exception.h>
#include <shv/coreqt/log.h>
#include <shv/iotqt/rpc/rpc.h>
#include <shv/iotqt/rpc/rpcresponsecallback.h>

namespace cp = shv::chainpack;
using namespace shv::core::utils;

DeviceLogRequest::DeviceLogRequest(const QString &shv_path, const QDateTime &since, const QDateTime &until, QObject *parent)
	: Super(parent)
	, m_shvPath(shv_path)
	, m_since(since)
	, m_until(until)
	, m_logDir(shv_path)
	, m_askElesys(true)
	, m_appendLog(false)
{
	m_askElesys = Application::instance()->deviceMonitor()->isElesysDevice(m_shvPath);
}

void DeviceLogRequest::getChunk()
{
	m_appendLog = false;
	if (m_since.isValid()) {
		LogDir log_dir(m_shvPath);
		QStringList all_files = log_dir.findFiles(m_since.addMSecs(-1), m_since);
		if (all_files.count() == 1) {
			std::ifstream in_file;
			in_file.open(all_files[0].toStdString(), std::ios::in | std::ios::binary);
			if (in_file) {
				cp::RpcValue::MetaData metadata;
				cp::ChainPackReader(in_file).read(metadata);
				int orig_record_count = metadata.value("recordCount").toInt();
				cp::RpcValue orig_until_cp = metadata.value("until");
				if (!orig_until_cp.isDateTime()) {
					SHV_QT_EXCEPTION("Missing until in file " + all_files[0]);
				}
				int64_t orig_until = orig_until_cp.toDateTime().msecsSinceEpoch();
				m_appendLog = (orig_until == m_since.toMSecsSinceEpoch() && orig_record_count < Application::CHUNK_RECORD_COUNT);
			}
		}
	}

	auto *conn = Application::instance()->deviceConnection();
	int rq_id = conn->nextRequestId();
	shv::iotqt::rpc::RpcResponseCallBack *cb = new shv::iotqt::rpc::RpcResponseCallBack(conn, rq_id, this);
	connect(cb, &shv::iotqt::rpc::RpcResponseCallBack::finished, this, &DeviceLogRequest::onChunkReceived);
	cb->start(2 * 60 * 1000);
	if (m_askElesys) {
		if (m_since.isValid() && m_since.date().daysTo(QDate::currentDate()) < 3) {
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
		param_map["logParams"] = logParams(!m_appendLog).toRpcValue();

		path = QString::fromStdString(Application::instance()->cliOptions()->elesysPath());
		params = param_map;
	}
	else {
		path = "shv/" + m_shvPath;
		params = logParams(!m_appendLog).toRpcValue();
	}

	conn->callShvMethod(rq_id, path.toStdString(), cp::Rpc::METH_GET_LOG, params);
}

void DeviceLogRequest::onChunkReceived(const shv::chainpack::RpcResponse &response)
{
	if (!response.isValid()) {
		shvError() << "invalid response";
		Q_EMIT finished(false);
		return;
	}
	if (response.isError()) {
		shvError() << (m_askElesys ? "elesys error:" : "device error:") << response.error().message();
		Q_EMIT finished(false);
		return;
	}

	try {
		const cp::RpcValue &result = response.result();
		ShvMemoryJournal log(logParams(!m_appendLog));
		log.loadLog(result);

		QDateTime first_record_time;
		QDateTime last_record_time;
		bool is_finished;
		QDateTime until = m_until;
		if (m_since.isValid() && m_until.isValid() && log.entries().size() == 0) {
			log.append(ShvJournalEntry{
						   ShvJournalEntry::PATH_DATA_MISSING,
						   ShvJournalEntry::DATA_MISSING_NOT_EXISTS,
						   ShvJournalEntry::DOMAIN_SHV_SYSTEM,
						   ShvJournalEntry::NO_SHORT_TIME,
						   ShvJournalEntry::SampleType::Continuous,
						   m_since.toMSecsSinceEpoch()
					   });
			log.append(ShvJournalEntry{
						   ShvJournalEntry::PATH_DATA_MISSING,
						   "",
						   ShvJournalEntry::DOMAIN_SHV_SYSTEM,
						   ShvJournalEntry::NO_SHORT_TIME,
						   ShvJournalEntry::SampleType::Continuous,
						   m_until.toMSecsSinceEpoch() - 1
					   });
		}

		if (log.size()) {
			first_record_time = QDateTime::fromMSecsSinceEpoch(log.entries()[0].epochMsec, Qt::TimeSpec::UTC);
			last_record_time = QDateTime::fromMSecsSinceEpoch(log.entries()[log.size() - 1].epochMsec, Qt::TimeSpec::UTC);
			is_finished = (int)log.size() < Application::CHUNK_RECORD_COUNT;
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
			if (log.size()) {
				until = last_record_time;
			}
			else {
				getChunk();
				return;
			}
		}
		shvInfo() << "received log for" << m_shvPath << "with" << log.size() << "records"
				  << "since" << first_record_time
				  << "until" << last_record_time;
		if (m_appendLog) {
			appendToPreviousFile(log, until);
			if (!m_since.isValid() && log.size() == 0) {
				fixFirstLogFile();
			}
		}
		else {
			if (Application::instance()->deviceMonitor()->isElesysDevice(m_shvPath) || !log.hasSnapshot()) {
				tryReplayPreviousFile(log);
			}
			saveToNewFile(log, until);
		}
		if (m_logDir.exists(m_logDir.dirtyLogName())) {
			trimDirtyLog(until);
		}
		m_since = until;

		if (is_finished) {
			Q_EMIT finished(true);
			return;
		}
		getChunk();
	}
	catch (const shv::core::Exception &) {
		Q_EMIT finished(false);
	}
}


void DeviceLogRequest::exec()
{
	getChunk();
}

ShvGetLogParams DeviceLogRequest::logParams(bool with_snapshot) const
{
	ShvGetLogParams params;
	params.since = cp::RpcValue::fromValue(m_since);
	params.until = cp::RpcValue::fromValue(m_until);
	params.recordCountLimit = Application::CHUNK_RECORD_COUNT;
	params.withSnapshot = with_snapshot;
	params.withTypeInfo = true;
	return params;
}

void DeviceLogRequest::appendToPreviousFile(ShvMemoryJournal &log, const QDateTime &until)
{
	LogDir log_dir(m_shvPath);
	QStringList all_files = log_dir.findFiles(m_since.addMSecs(-1), m_since);
	std::ifstream in_file;
	in_file.open(all_files[0].toStdString(), std::ios::in | std::ios::binary);
	cp::ChainPackReader log_reader(in_file);
	std::string error;
	cp::RpcValue orig_log = log_reader.read(&error);
	cp::RpcValue hp_metadata = orig_log.metaValue("HP");
	if (!error.empty() || !orig_log.isList()) {
		SHV_QT_EXCEPTION("Cannot parse file " + all_files[0]);
	}

	ShvGetLogParams joined_params;
	joined_params.recordCountLimit = 2 * Application::CHUNK_RECORD_COUNT;
	joined_params.withSnapshot = true;
	joined_params.withTypeInfo = true;
	ShvMemoryJournal joined_log(joined_params);
	joined_log.loadLog(orig_log);

	for (const auto &entry : log.entries()) {
		joined_log.append(entry);
	}
	QString temp_filename = all_files[0];
	temp_filename.replace(".chp", ".tmp");

	QFile temp_file(temp_filename);
	if (!temp_file.open(QFile::WriteOnly | QFile::Truncate)) {
		SHV_QT_EXCEPTION("Cannot open file " + temp_file.fileName());
	}
	cp::RpcValue result = joined_log.getLog(joined_params);
	result.setMetaValue("until", cp::RpcValue::fromValue(until));
	if (hp_metadata.isMap()) {
		result.setMetaValue("HP", hp_metadata);
	}
	temp_file.write(QByteArray::fromStdString(result.toChainPack()));
	temp_file.close();

	QFile old_log_file(all_files[0]);
	if (!old_log_file.remove()) {
		SHV_QT_EXCEPTION("Cannot remove file " + all_files[0]);
	}
	if (!temp_file.rename(old_log_file.fileName())) {
		SHV_QT_EXCEPTION("Cannot rename file " + temp_file.fileName());
	}
}

void DeviceLogRequest::tryReplayPreviousFile(ShvMemoryJournal &log)
{
	LogDir log_dir(m_shvPath);
	QStringList all_files = log_dir.findFiles(m_since.addMSecs(-1), m_since);
	if (all_files.count() == 1) {
		ShvMemoryJournal new_log(logParams());
		ShvLogFileReader previous_log(all_files[0].toStdString());
		while (previous_log.next()) {
			new_log.append(previous_log.entry());
		}
		for (const auto &entry : log.entries()) {
			new_log.append(entry);
		}
		log = new_log;
	}
}

void DeviceLogRequest::saveToNewFile(ShvMemoryJournal &log, const QDateTime &until)
{
	cp::RpcValue log_cp = log.getLog(logParams());

	QDateTime since;
	if (m_since.isValid()) {
		since = m_since;
	}
	else {
		since = rpcvalue_cast<QDateTime>(log_cp.metaValue("since"));
		log_cp.setMetaValue("HP", cp::RpcValue::Map{{ "firstLog", true }});
	}
	log_cp.setMetaValue("until", cp::RpcValue::fromValue(until));
	QFile file(LogDir(m_shvPath).filePath(since));
	if (!file.open(QFile::WriteOnly)) {
		SHV_QT_EXCEPTION("Cannot open file " + file.fileName());
	}
	file.write(QByteArray::fromStdString(log_cp.toChainPack()));
	file.close();
}

void DeviceLogRequest::trimDirtyLog(const QDateTime &until)
{

	qint64 until_msec = until.toMSecsSinceEpoch();
	QString temp_filename = "dirty.tmp";
	ShvJournalFileReader dirty_reader(m_logDir.dirtyLogPath().toStdString());
	if (m_logDir.exists(temp_filename)) {
		if (!m_logDir.remove(temp_filename)) {
			SHV_QT_EXCEPTION("cannot remove file " + m_logDir.absoluteFilePath(temp_filename));
		}
	}
	if (dirty_reader.next() && dirty_reader.entry().epochMsec < until_msec) {
		ShvJournalFileWriter dirty_writer(m_logDir.absoluteFilePath(temp_filename).toStdString());
		dirty_writer.append(ShvJournalEntry{
								ShvJournalEntry::PATH_DATA_DIRTY,
								true,
								ShvJournalEntry::DOMAIN_SHV_SYSTEM,
								ShvJournalEntry::NO_SHORT_TIME,
								ShvJournalEntry::SampleType::Continuous,
								until_msec
							});
		bool first = true;
		int64_t old_start_ts = 0LL;
		std::string data_missing_value;
		while (dirty_reader.next()) {
			const ShvJournalEntry &entry = dirty_reader.entry();
			if (!old_start_ts) {
				old_start_ts = entry.epochMsec;
			}
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
		if (!m_logDir.remove(m_logDir.dirtyLogName())) {
			SHV_QT_EXCEPTION("Cannot remove file " + m_logDir.dirtyLogPath());
		}
		if (!m_logDir.rename(temp_filename, m_logDir.dirtyLogName())) {
			SHV_QT_EXCEPTION("Cannot rename file " + m_logDir.absoluteFilePath(temp_filename));
		}
		auto *conn = Application::instance()->deviceConnection();
		if (until_msec != old_start_ts && conn->state() == shv::iotqt::rpc::DeviceConnection::State::BrokerConnected) {
			conn->sendShvSignal((m_shvPath + "/" + Application::DIRTY_LOG_NODE + "/" + Application::START_TS_NODE).toStdString(),
								cp::Rpc::SIG_VAL_CHANGED, cp::RpcValue::DateTime::fromMSecsSinceEpoch(until_msec));
		}
	}
}

void DeviceLogRequest::fixFirstLogFile()
{
	QStringList log_files = m_logDir.findFiles(QDateTime(), QDateTime());
	if (log_files.count()) {
		QFile first_file(log_files[0]);
		if (first_file.open(QFile::ReadOnly)) {
			cp::RpcValue log = cp::RpcValue::fromChainPack(first_file.readAll().toStdString());
			log.setMetaValue("HP", cp::RpcValue::Map{{ "firstLog", true }});
			first_file.close();
			if (first_file.open(QFile::WriteOnly | QFile::Truncate)) {
				first_file.write(QByteArray::fromStdString(log.toChainPack()));
			}
		}
	}
}
