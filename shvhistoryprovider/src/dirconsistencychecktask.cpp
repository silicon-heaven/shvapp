#include "dirconsistencychecktask.h"

#include "application.h"
#include "logdir.h"

#include <shv/coreqt/log.h>
#include <shv/core/utils/shvjournalfilereader.h>
#include <shv/core/utils/shvlogfilereader.h>
#include <shv/core/utils/shvlogheader.h>
#include <shv/iotqt/rpc/rpc.h>

namespace cp = shv::chainpack;
using namespace shv::core::utils;

DirConsistencyCheckTask::DirConsistencyCheckTask(const QString &shv_path, CheckType check_type, QObject *parent)
	: Super(parent)
	, m_shvPath(shv_path)
	, m_checkType(check_type)
{
}

void DirConsistencyCheckTask::run()
{
	try {
		QVector<DateTimeInterval> result;
		LogDir log_dir(m_shvPath);
		QElapsedTimer elapsed;
		elapsed.start();
		QStringList dir_entries = log_dir.findFiles(QDateTime(), QDateTime());
		QDateTime requested_since;
		for (int i = 0; i < dir_entries.count(); ++i) {
			if (isInterruptionRequested()) {
				result.clear();
				return;
			}
			ShvLogHeader header;
			try {
				header = ShvLogFileReader(dir_entries[i].toStdString()).logHeader();
			}
			catch(const shv::chainpack::AbstractStreamReader::ParseException &ex) {
				//when occurs error on file parsing, delete file and start again
				shvError() << "error on parsing log file" << dir_entries[i] << "deleting it (" << ex.what() << ")";
				QFile(dir_entries[i]).remove();
				result.clear();
				log_dir.refresh();
				dir_entries = log_dir.findFiles(QDateTime(), QDateTime());
				i = -1;
				continue;
			}
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
					result << DateTimeInterval{ QDateTime(), file_since };
				}
			}
			else if (requested_since < file_since) {
				result << DateTimeInterval{ requested_since, file_since };
			}
			requested_since = file_until;
		}
		bool exists_dirty = log_dir.existsDirtyLog();
		if (m_checkType == CheckType::ReplaceDirtyLog || !exists_dirty) {
			result << DateTimeInterval { requested_since, QDateTime() };
		}
		else if (exists_dirty){
			QDateTime requested_until;
			ShvJournalFileReader dirty_log(log_dir.dirtyLogPath().toStdString());
			if (dirty_log.next()) {
				requested_until = QDateTime::fromMSecsSinceEpoch(dirty_log.entry().epochMsec, Qt::TimeSpec::UTC);
				if (requested_until.isValid() && (!requested_since.isValid() || requested_since < requested_until)) {
					result << DateTimeInterval { requested_since, requested_until };
				}
			}
		}
		logSanitizerTimes() << "checkDirConsistency for" << m_shvPath << "elapsed" << elapsed.elapsed() << "ms"
							   " (dir has" << dir_entries.count() << "files)";
		Q_EMIT checkCompleted(result);
	}
	catch (const std::exception &ex) {
		Q_EMIT error(QString::fromStdString(ex.what()));
	}
}
