#include "devicelogrequest.h"
#include "application.h"
#include "singlerecordsetlogrequest.h"
#include "logdir.h"

#include <shv/coreqt/exception.h>
#include <shv/iotqt/rpc/rpc.h>
#include <shv/coreqt/log.h>
#include <shv/core/utils/shvjournalfilereader.h>
#include <shv/core/utils/shvjournalfilewriter.h>

namespace cp = shv::chainpack;
using namespace shv::core::utils;

DeviceLogRequest::DeviceLogRequest(const QString &shv_path, const QDateTime &since, const QDateTime &until, QObject *parent)
	: Super(parent)
	, m_shvPath(shv_path)
	, m_since(since)
	, m_until(until)
	, m_logDir(shv_path)
{
}

void DeviceLogRequest::exec()
{
	try {
		SingleRecordSetLogRequest *request = new SingleRecordSetLogRequest(m_shvPath, m_since, m_until, this);
		execRequest(request, [this, request]() {
			ShvMemoryJournal &log = request->receivedLog();

			QDateTime first_record_time;
			QDateTime last_record_time;
			if (log.entries().size()) {
				first_record_time = QDateTime::fromMSecsSinceEpoch(log.entries()[0].epochMsec, Qt::TimeSpec::UTC);
				last_record_time = QDateTime::fromMSecsSinceEpoch(log.entries()[log.entries().size() - 1].epochMsec, Qt::TimeSpec::UTC);
			}

			shvInfo() << "received log for" << m_shvPath << "with" << log.entries().size() << "records"
					  << "since" << first_record_time
					<< "until" << last_record_time;

			const ShvLogHeader &header = log.logHeader();
			QDateTime until = rpcvalue_cast<QDateTime>(header.until());

			bool is_finished =
					log.entries().size() == 0 ||
					(header.recordCountLimit() != 0 && (int)log.entries().size() < header.recordCountLimit()) ||
					m_since == m_until;

			if (!header.since().isValid()) {
				log.setSince(m_since.toMSecsSinceEpoch());
			}
			if (!header.until().isValid()) {
				until = is_finished ? m_until : last_record_time;
				log.setUntil(until.toMSecsSinceEpoch());
			}

			QFile file(LogDir(m_shvPath).filePath(m_since));
			if (!file.open(QFile::WriteOnly)) {
				SHV_QT_EXCEPTION("Cannot open file " + file.fileName());
			}
			file.write(QByteArray::fromStdString(log.getLog(logParams()).toChainPack()));
			file.close();
			stripDirtyLog(until);
			m_since = until;

			if (is_finished) {
				shvInfo() << "device log request finished";
				Q_EMIT finished(true);
			}
			else {
				exec();
			}
		});
	}
	catch (const shv::core::Exception &e) {
		error(e.what());
	}
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

void DeviceLogRequest::stripDirtyLog(const QDateTime &until)
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
	std::string unavail;
	while (dirty_reader.next()) {
		if (dirty_reader.entry().epochMsec > until_msec) {
			if (first) {
				first = false;
				if (!unavail.empty()) {
						dirty_writer.append(ShvJournalEntry{
												"data_unavailable",
												unavail,
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
			if (dirty_reader.entry().path == "data_unavailable" && dirty_reader.entry().value.isString()) {
				unavail = dirty_reader.entry().value.toString();
			}
		}
	}
	m_logDir.remove(m_logDir.dirtyLogName());
	m_logDir.rename(temp_filename, m_logDir.dirtyLogName());
}
