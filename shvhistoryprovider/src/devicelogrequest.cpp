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

void getChunk()
{
	if(m_since < m_until) {
		{
			int rq_id = m_rpcConnection->nextRequestId();
			shv::iotqt::rpc::RpcResponseCallBack *cb = new shv::iotqt::rpc::RpcResponseCallBack(m_rpcConnection, rq_id, this);
			cb->start([this](const cp::RpcResponse &resp) {
				.....
				QTimer::singleShot(getChunk());
			});
			m_rpcConnection->callShvMethod(rq_id, shv_path.toStdString(), method.toStdString(), params);
		}
	}
}

void DeviceLogRequest::exec()
{
	try {
		getChunk();
		SingleRecordSetLogRequest *request = new SingleRecordSetLogRequest(m_shvPath, m_since, m_until, this);
		execRequest(request, [this, request]() {
			ShvMemoryJournal &log = request->receivedLog();

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
