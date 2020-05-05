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

CacheState CheckLogTask::checkLogCache(const QString &shv_path)
{
	QStringList short_files;
	CacheState state;
	LogDir m_logDir(shv_path);

	QStringList dir_entries = m_logDir.findFiles(QDateTime(), QDateTime());
	dir_entries = m_logDir.findFiles(QDateTime(), QDateTime());
	QDateTime requested_since;
	for (int i = 0; i < dir_entries.count(); ++i) {
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
				state.errors << CacheError  {
								CacheError::Type::FirstFileMarkMissing,
								dir_entries[i],
								"first file doesn't have firstLog mark" };
			}
			state.since = file_since;
		}
		else if (requested_since < file_since) {
			state.errors << CacheError {
							CacheError::Type::SinceUntilGap,
							dir_entries[i],
							"missing data between " + requested_since.toString(Qt::DateFormat::ISODateWithMs)
							+ " and " + file_since.toString(Qt::DateFormat::ISODateWithMs) };
		}
		else if (requested_since > file_since) {
			state.errors << CacheError {
							CacheError::Type::SinceUntilOverlap,
							dir_entries[i],
							"data between " + requested_since.toString(Qt::DateFormat::ISODateWithMs)
							+ " and " + file_since.toString(Qt::DateFormat::ISODateWithMs + " overlaps") };

		}
		requested_since = file_until;
		if (header.recordCount() < Application::CHUNK_RECORD_COUNT - 1) {
			short_files << dir_entries[i];
		}
		state.recordCount += header.recordCount();
		++state.fileCount;
		state.until = file_until;
	}
	bool exists_dirty = m_logDir.exists(m_logDir.dirtyLogName());
	if (!exists_dirty) {
		state.errors << CacheError { CacheError::Type::DirtLogMissing, m_logDir.dirtyLogPath(), "missing dirty log" };
	}
	else {
		ShvJournalFileReader dirty_log(m_logDir.dirtyLogPath().toStdString());
		std::string entry_path;
		QDateTime entry_ts;
		if (dirty_log.next()) {
			entry_path = dirty_log.entry().path;
			entry_ts = QDateTime::fromMSecsSinceEpoch(dirty_log.entry().epochMsec, Qt::UTC);
		}
		if (entry_path != shv::core::utils::ShvJournalEntry::PATH_DATA_DIRTY) {
			state.errors << CacheError { CacheError::Type::DirtyLogMarkMissing, m_logDir.dirtyLogPath(),
							"missing dirty mark on begin of dirty log" };
		}
		if (!entry_ts.isValid()) {
			state.errors << CacheError { CacheError::Type::SinceUntilGap, m_logDir.dirtyLogPath(),
							("missing data since " + requested_since.toString(Qt::DateFormat::ISODateWithMs)) };
		}
		else if (entry_ts > requested_since) {
			state.errors << CacheError { CacheError::Type::SinceUntilGap, m_logDir.dirtyLogPath(),
							("missing data between " + requested_since.toString(Qt::DateFormat::ISODateWithMs)
							 + " and " + entry_ts.toString(Qt::DateFormat::ISODateWithMs)) };
		}
		else if (entry_ts < requested_since) {
			state.errors << CacheError { CacheError::Type::SinceUntilOverlap, m_logDir.dirtyLogPath(),
							("data between " + requested_since.toString(Qt::DateFormat::ISODateWithMs)
							 + " and " + entry_ts.toString(Qt::DateFormat::ISODateWithMs) + " overlaps") };
		}
		++state.fileCount;
		while (dirty_log.next()) {
			state.until = QDateTime::fromMSecsSinceEpoch(dirty_log.entry().epochMsec, Qt::UTC);
		}
	}
	if (short_files.count()) {
		state.errors << CacheError { CacheError::Type::Fragmentation, short_files.join(", "),
						("there are " + QString::number(short_files.count()) + " files where is less than " +
					   QString::number(Application::CHUNK_RECORD_COUNT) + " records") };
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
