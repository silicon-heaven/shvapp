#include "application.h"
#include "appclioptions.h"
#include "checklogtask.h"
#include "devicelogrequest.h"
#include "devicemonitor.h"
#include "dirconsistencychecktask.h"
#include "logdir.h"

#include <shv/core/utils/shvjournalfilereader.h>
#include <shv/core/utils/shvjournalfilewriter.h>
#include <shv/core/utils/shvlogfilereader.h>
#include <shv/coreqt/log.h>
#include <shv/coreqt/exception.h>
#include <shv/iotqt/rpc/rpc.h>

namespace cp = shv::chainpack;
using namespace shv::core::utils;

CheckLogTask::CheckLogTask(const QString &site_path, CheckType check_type, QObject *parent)
	: Super(parent)
	, m_sitePath(site_path)
	, m_checkType(check_type)
	, m_logDir(m_sitePath)
{
	connect(Application::instance()->deviceConnection(), &shv::iotqt::rpc::DeviceConnection::stateChanged, this, &CheckLogTask::onShvStateChanged);
	connect(Application::instance()->deviceMonitor(), &DeviceMonitor::deviceDisconnectedFromBroker, this, &CheckLogTask::onDeviceDisappeared);
	connect(this, &CheckLogTask::finished, [this]() {
		if (m_dirConsistencyCheckTask && !m_dirConsistencyCheckTask->isFinished()) {
			m_dirConsistencyCheckTask->requestInterruption();
		}
	});
}

void CheckLogTask::onShvStateChanged()
{
	if (Application::instance()->deviceConnection()->state() == shv::iotqt::rpc::ClientConnection::State::NotConnected) {
		abort();
	}
}

void CheckLogTask::onDeviceDisappeared(const QString &site_path)
{
	if (site_path == m_sitePath) {
		abort();
	}
}

void CheckLogTask::abort()
{
	for (DeviceLogRequest *request : findChildren<DeviceLogRequest*>()) {
		request->deleteLater();
	}
	m_dirEntries.clear();
	Q_EMIT finished(false);
}

void CheckLogTask::exec()
{
	try {
		m_dirEntries = m_logDir.findFiles(QDateTime(), QDateTime());

		if (m_checkType == CheckType::TrimDirtyLogOnly) {
			int64_t dirty_begin = 0LL;
			int64_t regular_end = 0LL;
			if (m_logDir.existsDirtyLog()) {
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
				getLog(QDateTime::fromMSecsSinceEpoch(dirty_begin, Qt::TimeSpec::UTC), QDateTime());
			}
			else {
				m_checkType = CheckType::ReplaceDirtyLog;
			}
		}
		if (m_checkType == CheckType::ReplaceDirtyLog || m_checkType == CheckType::CheckDirtyLogState) {
			m_dirConsistencyCheckTask = new DirConsistencyCheckTask(m_sitePath, m_checkType, this);
			connect(m_dirConsistencyCheckTask, &DirConsistencyCheckTask::checkCompleted, this, &CheckLogTask::onDirConsistencyChecked);
			connect(m_dirConsistencyCheckTask, &DirConsistencyCheckTask::error, this, [this](QString msg) {
				shvError() << msg;
				Q_EMIT finished(false);
			});
			connect(m_dirConsistencyCheckTask, &DirConsistencyCheckTask::finished, this, [this]() {
				m_dirConsistencyCheckTask = nullptr;
			});
			connect(m_dirConsistencyCheckTask, &DirConsistencyCheckTask::finished, m_dirConsistencyCheckTask, &DirConsistencyCheckTask::deleteLater);
			m_dirConsistencyCheckTask->start(QThread::Priority::LowPriority);

//			QFutureWatcher<QVector<DateTimeInterval>> *watcher = new QFutureWatcher<QVector<DateTimeInterval>>;
//			connect(watcher, &QFutureWatcher<QVector<DateTimeInterval>>::finished, &QFutureWatcher<QVector<DateTimeInterval>>::deleteLater);
//			connect(watcher, &QFutureWatcher<QVector<DateTimeInterval>>::finished, this, [this, watcher]() {
//				try {
//					onDirConsistencyChecked(watcher->result());
//				}
//				catch (const QException &ex) {
//					shvError() << ex.what();
//					Q_EMIT finished(false);
//				}
//				catch (const std::exception &ex) {
//					shvError() << ex.what();
//					Q_EMIT finished(false);
//				}
//			});

//			// Start the computation.
//			QFuture<QVector<DateTimeInterval>> future = QtConcurrent::run(this, &CheckLogTask::checkDirConsistency);
//			watcher->setFuture(future);
		}
		else if (m_requests.count() == 0) {
			Q_EMIT finished(true);
		}
	}
	catch (const std::exception &e) {
		shvError() << "CheckLogTask" << m_sitePath << e.what();
		Q_EMIT finished(false);
	}
}

CacheState CheckLogTask::checkLogCache(const QString &site_path, bool with_good_files)
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
	LogDir m_logDir(site_path);

	QStringList dir_entries = m_logDir.findFiles(QDateTime(), QDateTime());
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

	if (!Application::instance()->deviceMonitor()->isPushLogDevice(site_path)) {
		CacheFileState dirty_file_state;
		dirty_file_state.fileName = m_logDir.dirtyLogName();
		bool exists_dirty = m_logDir.existsDirtyLog();
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
	}
	return state;
}

//QVector<DateTimeInterval> CheckLogTask::checkDirConsistency()
//{
//	QVector<DateTimeInterval> res;
//	QElapsedTimer elapsed;
//	elapsed.start();
//	m_dirEntries = m_logDir.findFiles(QDateTime(), QDateTime());
//	QDateTime requested_since;
//	for (int i = 0; i < m_dirEntries.count(); ++i) {
//		ShvLogHeader header;
//		try {
//			header = ShvLogFileReader(m_dirEntries[i].toStdString()).logHeader();
//		}
//		catch(const shv::chainpack::AbstractStreamReader::ParseException &ex) {
//			//when occurs error on file parsing, delete file and start again
//			shvError() << "error on parsing log file" << m_dirEntries[i] << "deleting it (" << ex.what() << ")";
//			QFile(m_dirEntries[i]).remove();
//			res.clear();
//			m_logDir.refresh();
//			m_dirEntries = m_logDir.findFiles(QDateTime(), QDateTime());
//			i = -1;
//			continue;
//		}
//		QDateTime file_since = rpcvalue_cast<QDateTime>(header.since());
//		QDateTime file_until = rpcvalue_cast<QDateTime>(header.until());
//		if (!requested_since.isValid()) {
//			std::ifstream in_file;
//			in_file.open(m_dirEntries[i].toStdString(),  std::ios::in | std::ios::binary);
//			shv::chainpack::ChainPackReader first_file_reader(in_file);
//			cp::RpcValue::MetaData meta_data;
//			first_file_reader.read(meta_data);
//			cp::RpcValue meta_hp = meta_data.value("HP");
//			bool has_first_log_mark = false;
//			if (meta_hp.isMap()) {
//				cp::RpcValue meta_first = meta_hp.toMap().value("firstLog");
//				has_first_log_mark = meta_first.isBool() && meta_first.toBool();
//			}
//			if (!has_first_log_mark) {
//				res << DateTimeInterval{ QDateTime(), file_since };
//			}
//		}
//		else if (requested_since < file_since) {
//			res << DateTimeInterval{ requested_since, file_since };
//		}
//		requested_since = file_until;
//	}
//	bool exists_dirty = m_logDir.existsDirtyLog();
//	if (m_checkType == CheckType::ReplaceDirtyLog || !exists_dirty) {
//		res << DateTimeInterval { requested_since, QDateTime() };
//	}
//	else if (exists_dirty){
//		QDateTime requested_until;
//		ShvJournalFileReader dirty_log(m_logDir.dirtyLogPath().toStdString());
//		if (dirty_log.next()) {
//			requested_until = QDateTime::fromMSecsSinceEpoch(dirty_log.entry().epochMsec, Qt::TimeSpec::UTC);
//			if (requested_until.isValid() && (!requested_since.isValid() || requested_since < requested_until)) {
//				res << DateTimeInterval { requested_since, requested_until };
//			}
//		}
//	}
//	logSanitizerTimes() << "checkDirConsistency for" << m_shvPath << "elapsed" << elapsed.elapsed() << "ms"
//						   " (dir has" << m_dirEntries.count() << "files)";

//	return res;
//}

void CheckLogTask::onDirConsistencyChecked(const QVector<DateTimeInterval> &requested_intervals)
{
	for (const DateTimeInterval &interval : requested_intervals) {
		getLog(interval.since, interval.until);
	}
	if (m_checkType == CheckType::CheckDirtyLogState) {
		checkDirtyLogState();
	}
	if (m_requests.count() == 0) {
		Q_EMIT finished(true);
	}
}

void CheckLogTask::checkDirtyLogState()
{
	if (m_logDir.existsDirtyLog()) {

		QElapsedTimer elapsed;
		elapsed.start();

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
			QDateTime journal_since = QDateTime::fromMSecsSinceEpoch(first_entry.epochMsec, Qt::TimeSpec::UTC);
			if (log_since.isValid() && log_since > journal_since) {
				journal_since = log_since;
			}
			int rec_count = 1;
			QDateTime current = QDateTime::currentDateTimeUtc();
			if (journal_since.secsTo(current) > Application::instance()->cliOptions()->trimDirtyLogInterval() * 60) {
				getLog(journal_since, QDateTime());
			}
			else {
				while (reader.next()) {
					++rec_count;
					if (rec_count >= Application::CHUNK_RECORD_COUNT) {
						getLog(journal_since, QDateTime());
						break;
					}
				}
			}
			logSanitizerTimes() << "checkDirtyLogState for" << m_sitePath << "elapsed" << elapsed.elapsed() << "ms"
								   " (checked" << rec_count << "rows)";
		}
	}
}

void CheckLogTask::execRequest(DeviceLogRequest *request)
{
	shvInfo() << "requesting log" << m_sitePath << request->since() << "-" << request->until();
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
	m_requests << new DeviceLogRequest(m_sitePath, since, until, this);
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
