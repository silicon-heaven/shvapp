#include "application.h"
#include "appclioptions.h"
#include "checklogtask.h"
#include "devicelogrequest.h"
#include "logdir.h"

#include <shv/core/utils/shvjournalfilereader.h>
#include <shv/core/utils/shvjournalfilewriter.h>
#include <shv/core/utils/shvlogfilereader.h>
#include <shv/coreqt/log.h>
#include <shv/coreqt/exception.h>
#include <shv/iotqt/rpc/rpc.h>

namespace cp = shv::chainpack;
using namespace shv::core::utils;

CheckLogTask::CheckLogTask(const QString &shv_path, CheckType check_type, QObject *parent)
	: Super(parent)
	, m_shvPath(shv_path)
	, m_checkType(check_type)
	, m_logDir(m_shvPath)
{
}

void CheckLogTask::exec()
{
	try {
		QStringList m_dirEntries = m_logDir.findFiles(QDateTime(), QDateTime());

		if (m_checkType == CheckType::TrimDirtyLogOnly) {
			int64_t dirty_begin = 0LL;
			int64_t regular_end = 0LL;
			if (m_logDir.exists(m_logDir.dirtyLogName())) {
				ShvJournalFileReader reader(m_logDir.dirtyLogPath().toStdString());
				if (reader.next()) {
					dirty_begin = reader.entry().epochMsec;
				}
			}
			if (m_dirEntries.count()) {
				ShvLogFileReader last_log(m_dirEntries.last().toStdString());
				regular_end = last_log.logHeader().until().toDateTime().msecsSinceEpoch();
			}
			if (dirty_begin && dirty_begin == regular_end) {
				getLog(QDateTime::fromMSecsSinceEpoch(dirty_begin, Qt::TimeSpec::UTC), QDateTime::currentDateTimeUtc());
			}
			else {
				m_checkType = CheckType::ReplaceDirtyLog;
			}
		}
		if (m_checkType == CheckType::ReplaceDirtyLog || m_checkType == CheckType::CheckDirtyLogState) {
			checkOldDataConsistency();
			if (m_checkType == CheckType::CheckDirtyLogState) {
				checkDirtyLogState();
			}
		}
		if (m_requests.count() == 0) {
			Q_EMIT finished(true);
		}
	}
	catch (shv::core::Exception &e) {
		shvError() << e.message();
		Q_EMIT finished(false);
	}
}

CacheState CheckLogTask::checkLogCache(const QString &shv_path, bool with_good_files)
{
	auto eval_status = [](CacheFileState &file_state) {
		file_state.status = CacheStatus::OK;
		for (const auto &err : file_state.errors) {
			if ((int)err.status > (int)file_state.status) {
				file_state.status = err.status;
			}
		}
	};

	CacheState state;
	LogDir m_logDir(shv_path);

	QStringList dir_entries = m_logDir.findFiles(QDateTime(), QDateTime());
	dir_entries = m_logDir.findFiles(QDateTime(), QDateTime());
	QDateTime requested_since;
	for (int i = 0; i < dir_entries.count(); ++i) {
		CacheFileState file_state;
		file_state.fileName = dir_entries[i];
		ShvLogHeader header = ShvLogFileReader(dir_entries[i].toStdString()).logHeader();
		QDateTime file_since = rpcvalue_cast<QDateTime>(header.since());
		QDateTime file_until = rpcvalue_cast<QDateTime>(header.until());
		if (!requested_since.isValid()) {
			std::ifstream in_file;
			in_file.open(dir_entries[i].toStdString(),  std::ios::in | std::ios::binary);
			shv::chainpack::ChainPackReader first_file_reader(in_file);
			cp::RpcValue::MetaData meta_data;
			first_file_reader.read(meta_data);
			cp::RpcValue meta_hp = meta_data.value("HP");
			bool has_first_log_mark = false;
			if (meta_hp.isMap()) {
				cp::RpcValue meta_first = meta_hp.toMap().value("firstLog");
				has_first_log_mark = meta_first.isBool() && meta_first.toBool();
			}
			if (!has_first_log_mark) {
				file_state.errors << CacheError  {
								CacheStatus::Error,
								CacheError::Type::FirstFileMarkMissing,
								"first file doesn't have firstLog mark" };
			}
			state.since = file_since;
		}
		else if (requested_since < file_since) {
			file_state.errors << CacheError {
							CacheStatus::Error,
							CacheError::Type::SinceUntilGap,
							"missing data between " + requested_since.toString(Qt::DateFormat::ISODateWithMs)
							+ " and " + file_since.toString(Qt::DateFormat::ISODateWithMs) };
		}
		else if (requested_since > file_since) {
			file_state.errors << CacheError {
							CacheStatus::Error,
							CacheError::Type::SinceUntilOverlap,
							"data between " + requested_since.toString(Qt::DateFormat::ISODateWithMs)
							+ " and " + file_since.toString(Qt::DateFormat::ISODateWithMs) + " overlaps" };

		}
		requested_since = file_until;
		if (i + 1 < dir_entries.count() && header.recordCount() < Application::CHUNK_RECORD_COUNT - 2000) {
			file_state.errors << CacheError {
						Application::CHUNK_RECORD_COUNT - header.recordCount() > 4000 ? CacheStatus::Error : CacheStatus::Warning,
						CacheError::Type::Fragmentation,
						"file has less than " + QString::number(Application::CHUNK_RECORD_COUNT) + " records" };
		}
		file_state.recordCount = header.recordCount();
		state.recordCount += header.recordCount();
		++state.fileCount;
		state.until = file_until;
		eval_status(file_state);
		if (with_good_files || file_state.status != CacheStatus::OK) {
			state.files << file_state;
		}
	}
	CacheFileState dirty_file_state;
	dirty_file_state.fileName = m_logDir.dirtyLogName();
	bool exists_dirty = m_logDir.exists(m_logDir.dirtyLogName());
	if (!exists_dirty) {
		dirty_file_state.errors << CacheError { CacheStatus::Warning, CacheError::Type::DirtLogMissing, "missing dirty log" };
	}
	else {
		ShvJournalFileReader dirty_log(m_logDir.dirtyLogPath().toStdString());
		std::string entry_path;
		QDateTime entry_ts;
		if (dirty_log.next()) {
			++state.recordCount;
			entry_path = dirty_log.entry().path;
			entry_ts = QDateTime::fromMSecsSinceEpoch(dirty_log.entry().epochMsec, Qt::UTC);
		}
		if (entry_path != shv::core::utils::ShvJournalEntry::PATH_DATA_DIRTY) {
			dirty_file_state.errors << CacheError { CacheStatus::Error, CacheError::Type::DirtyLogMarkMissing,
							"missing dirty mark on begin of dirty log" };
		}
		if (!entry_ts.isValid()) {
			dirty_file_state.errors << CacheError { CacheStatus::Error, CacheError::Type::SinceUntilGap,
							("missing data since " + requested_since.toString(Qt::DateFormat::ISODateWithMs)) };
		}
		else if (entry_ts > requested_since) {
			dirty_file_state.errors << CacheError { CacheStatus::Error, CacheError::Type::SinceUntilGap,
							("missing data between " + requested_since.toString(Qt::DateFormat::ISODateWithMs)
							 + " and " + entry_ts.toString(Qt::DateFormat::ISODateWithMs)) };
		}
		else if (entry_ts < requested_since) {
			dirty_file_state.errors << CacheError { CacheStatus::Error, CacheError::Type::SinceUntilOverlap,
							("data between " + requested_since.toString(Qt::DateFormat::ISODateWithMs)
							 + " and " + entry_ts.toString(Qt::DateFormat::ISODateWithMs) + " overlaps") };
		}
		++state.fileCount;
		while (dirty_log.next()) {
			state.until = QDateTime::fromMSecsSinceEpoch(dirty_log.entry().epochMsec, Qt::UTC);
			++state.recordCount;
		}
	}
	eval_status(dirty_file_state);
	if (with_good_files || dirty_file_state.status != CacheStatus::OK) {
		state.files << dirty_file_state;
	}
	return state;
}

void CheckLogTask::checkOldDataConsistency()
{
	m_dirEntries = m_logDir.findFiles(QDateTime(), QDateTime());
	QDateTime requested_since;
	for (int i = 0; i < m_dirEntries.count(); ++i) {
		ShvLogHeader header = ShvLogFileReader(m_dirEntries[i].toStdString()).logHeader();
		QDateTime file_since = rpcvalue_cast<QDateTime>(header.since());
		QDateTime file_until = rpcvalue_cast<QDateTime>(header.until());
		if (!requested_since.isValid()) {
			std::ifstream in_file;
			in_file.open(m_dirEntries[i].toStdString(),  std::ios::in | std::ios::binary);
			shv::chainpack::ChainPackReader first_file_reader(in_file);
			cp::RpcValue::MetaData meta_data;
			first_file_reader.read(meta_data);
			cp::RpcValue meta_hp = meta_data.value("HP");
			bool has_first_log_mark = false;
			if (meta_hp.isMap()) {
				cp::RpcValue meta_first = meta_hp.toMap().value("firstLog");
				has_first_log_mark = meta_first.isBool() && meta_first.toBool();
			}
			if (!has_first_log_mark) {
				getLog(QDateTime(), file_since);
			}
		}
		else if (requested_since < file_since) {
			getLog(requested_since, file_since);
		}
		requested_since = file_until;
	}
	bool exists_dirty = m_logDir.exists(m_logDir.dirtyLogName());
	QDateTime requested_until;
	if (m_checkType == CheckType::ReplaceDirtyLog || !exists_dirty) {
		requested_until = QDateTime::currentDateTimeUtc();
	}
	else if (exists_dirty){
		ShvJournalFileReader dirty_log(m_logDir.dirtyLogPath().toStdString());
		if (dirty_log.next()) {
			requested_until = QDateTime::fromMSecsSinceEpoch(dirty_log.entry().epochMsec, Qt::TimeSpec::UTC);
			if (dirty_log.next()) {
				ShvJournalEntry second_entry = dirty_log.entry();
				if (second_entry.path == ShvJournalEntry::PATH_DATA_MISSING && second_entry.value.toString() == ShvJournalEntry::DATA_MISSING_UNAVAILABLE) {
					int64_t data_missing_begin = second_entry.epochMsec;
					if (dirty_log.next()) {
						ShvJournalEntry third_entry = dirty_log.entry();
						if (third_entry.path == ShvJournalEntry::PATH_DATA_MISSING && third_entry.value.toString().empty()) {
							int64_t data_missing_end = third_entry.epochMsec;
							if (data_missing_end - data_missing_begin > 5) {
								requested_until = QDateTime::currentDateTimeUtc();
								m_checkType = CheckType::ReplaceDirtyLog;
							}
						}
					}
				}
			}
		}
	}

	if (requested_until.isValid() && (!requested_since.isValid() || requested_since < requested_until)) {
		getLog(requested_since, requested_until);
	}
}

void CheckLogTask::checkDirtyLogState()
{
	if (m_logDir.exists(m_logDir.dirtyLogName())) {

		//Zjisteni datumu od kdy chceme nahradit dirty log normalnim logem:
		//Existuji 2 zdroje, konec posledniho radneho logu a zacatek dirty logu.
		//Normalne by mely byt konzistentni, po dotazeni radneho logu, se zacatek dirty logu zahodi,
		//ale neni to atomicke, protoze aplikace muze prave v tuto chvili spadnout nebo muze skoncit,
		//takze se muze stat, ze se radny log stahne, ale dirty log se neupravi.
		//Pokud je tedy zacatek dirty logu starsi nez konec posledniho radneho logu, pouzije se konec
		//posledniho radneho logu.
		//Nekonzistence muze nastat i opacna, radne logy se uz stahuji, ale jeste
		//nejsou vsechny dotazene a konec posledniho radneho logu je starsi nez zacatek dirty logu. Potom
		//pouzijeme zacatek dirty logu.
		QDateTime log_since;
		if (m_dirEntries.count()) {
			ShvLogFileReader last_log(m_dirEntries.last().toStdString());
			log_since = rpcvalue_cast<QDateTime>(last_log.logHeader().until());
		}
		ShvJournalFileReader reader(m_logDir.dirtyLogPath().toStdString());
		if (reader.next()) {
			ShvJournalEntry first_entry = reader.entry();
			int rec_count = 1;
			while (reader.next()) {
				++rec_count;
			}
			QDateTime journal_since = QDateTime::fromMSecsSinceEpoch(first_entry.epochMsec, Qt::TimeSpec::UTC);
			if (log_since.isValid() && log_since > journal_since) {
				journal_since = log_since;
			}
			QDateTime current = QDateTime::currentDateTimeUtc();
			if (rec_count >= Application::CHUNK_RECORD_COUNT || journal_since.secsTo(current) > Application::instance()->cliOptions()->trimDirtyLogInterval() * 60) {
				getLog(journal_since, current);
			}
		}
	}
}

void CheckLogTask::execRequest(DeviceLogRequest *request)
{
	shvInfo() << "requesting log" << m_shvPath << request->since() << "-" << request->until();
	connect(request, &DeviceLogRequest::finished, this, [this, request](bool success) {
		request->deleteLater();
		if (!success) {
			Q_EMIT finished(false);
			return;
		}
		try {
			checkRequestQueue();
		}
		catch (const shv::core::Exception &e) {
			shvError() << e.message();
			Q_EMIT finished(false);
		}
	}, Qt::QueuedConnection);
	request->exec();
}

void CheckLogTask::getLog(const QDateTime &since, const QDateTime &until)
{
	m_requests << new DeviceLogRequest(m_shvPath, since, until, this);
	if (m_requests.count() == 1) {
		execRequest(m_requests[0]);
	}
}

void CheckLogTask::checkRequestQueue()
{
	m_requests.removeFirst();
	if (m_requests.count() > 0) {
		execRequest(m_requests[0]);
	}
	else {
		Q_EMIT finished(true);
	}
}

const char *CacheError::typeToString(CacheError::Type t)
{
	static QMetaEnum type_enum = QMetaEnum::fromType<Type>();
	return type_enum.valueToKey((int)t);
}

const char *cacheStatusToString(CacheStatus st)
{
	switch (st) {
	case CacheStatus::OK:
		return "OK";
	case CacheStatus::Warning:
		return "Warning";
	case CacheStatus::Error:
		return "Error";
	}
	return "";
}
