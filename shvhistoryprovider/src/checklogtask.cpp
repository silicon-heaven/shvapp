#include "application.h"
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

CheckLogTask::CheckLogTask(const QString &shv_path, CheckLogType check_type, QObject *parent)
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

		if (m_checkType == CheckLogType::TrimDirtyLogOnly) {
			int64_t dirty_begin = 0LL;
			int64_t regular_end = 0LL;
			if (m_logDir.exists(m_logDir.dirtyLogName())) {
				ShvLogHeader header;
				ShvJournalFileReader reader(m_logDir.dirtyLogPath().toStdString(), &header);
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
				m_checkType = CheckLogType::ReplaceDirtyLog;
			}
		}
		if (m_checkType == CheckLogType::ReplaceDirtyLog || m_checkType == CheckLogType::CheckDirtyLogState) {
			checkOldDataConsistency();
			if (m_checkType == CheckLogType::CheckDirtyLogState) {
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

void CheckLogTask::checkOldDataConsistency()
{
	m_dirEntries = m_logDir.findFiles(QDateTime(), QDateTime());
	QDateTime requested_since = Application::WORLD_BEGIN;
	for (int i = 0; i < m_dirEntries.count(); ++i) {
		ShvLogHeader header = ShvLogFileReader(m_dirEntries[i].toStdString()).logHeader();
		QDateTime file_since = rpcvalue_cast<QDateTime>(header.since());
		QDateTime file_until = rpcvalue_cast<QDateTime>(header.until());
		if (requested_since < file_since) {
			getLog(requested_since, file_since);
		}
		requested_since = file_until;
	}
	bool exists_dirty = m_logDir.exists(m_logDir.dirtyLogName());
	QDateTime requested_until;
	if (m_checkType == CheckLogType::ReplaceDirtyLog || !exists_dirty) {
		requested_until = QDateTime::currentDateTimeUtc();
	}
	else if (exists_dirty){
		ShvLogHeader header;
		ShvJournalFileReader dirty_log(m_logDir.dirtyLogPath().toStdString(), &header);
		if (dirty_log.next()) {
			requested_until = QDateTime::fromMSecsSinceEpoch(dirty_log.entry().epochMsec, Qt::TimeSpec::UTC);
		}
	}

	if (requested_until.isValid() && requested_since < requested_until) {
		getLog(requested_since, requested_until);
	}
}

void CheckLogTask::checkDirtyLogState()
{
	if (m_logDir.exists(m_logDir.dirtyLogName())) {

		//Zjisteni datumu od kdy chceme nahradit dirty log normalnim logem:
		//Existuji 2 zdroje, konec posledniho radneho logu a zacatek dirty logu.
		//Normalne by mely byt konzistentni, po dotazeni radneho logu, se zacatek dirty logu zahodi,
		//ale neni to atomicke, takze se muze stat, ze se radny log stahne, ale dirty log se neupravi.
		//Pokud je tedy zacatek dirty logu starsi nez konec posledniho radneho logu, pouzije se konec
		//posledniho radneho logu. Nekonzistence muze nastat i opacna, radne logy se uz stahuji, ale jeste
		//nejsou vsechny dotazene a konec posledniho radneho logu je starsi nez zacatek dirty logu. Potom
		//pouzijeme zacatek dirty logu.
		QDateTime log_since;
		if (m_dirEntries.count()) {
			ShvLogFileReader last_log(m_dirEntries.last().toStdString());
			log_since = rpcvalue_cast<QDateTime>(last_log.logHeader().until());
		}
		ShvLogHeader header;
		ShvJournalFileReader reader(m_logDir.dirtyLogPath().toStdString(), &header);
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
			if (rec_count >= Application::SINGLE_FILE_RECORD_COUNT || journal_since.secsTo(current) > 60 * 60 * 12) {
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
