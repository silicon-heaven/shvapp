#include "application.h"
#include "appclioptions.h"
#include "logdir.h"

#include <QDateTime>
#include <QVector>

#include <shv/core/utils/shvfilejournal.h>
#include <shv/chainpack/chainpackreader.h>
#include <shv/iotqt/rpc/rpc.h>

#include <iostream>
#include <fstream>

LogDir::LogDir(const QString &shv_path)
	: m_dir(dirPath(shv_path), "*", QDir::SortFlag::Name, QDir::Filter::Files)
{
	if (!m_dir.exists()) {
		QDir(QString::fromStdString(Application::instance()->cliOptions()->logCacheDir())).mkpath(shv_path);
		m_dir.refresh();
	}
}

QString LogDir::dirPath(const QString &shv_path)
{
	return QString::fromStdString(Application::instance()->cliOptions()->logCacheDir()) + QDir::separator() + shv_path;
}

QStringList LogDir::findFiles(const QDateTime &since, const QDateTime &until)
{
	QFileInfoList file_infos = m_dir.entryInfoList();
	QStringList file_names;
	QVector<int64_t> file_dates;
	for (int i = 0; i < file_infos.count(); ++i) {
		const QFileInfo &file_info = file_infos[i];
		QString filename = file_info.fileName();
		if (file_info.suffix() != "chp") {
			if (filename != dirtyLogName()) {
				Application::instance()->registerAlienFile(file_info.absoluteFilePath());
			}
		}
		else {
			try {
				int64_t datetime = shv::core::utils::ShvFileJournal::JournalContext::fileNameToFileMsec(filename.toStdString());
				file_names << filename;
				file_dates << datetime;
			}
			catch (...) {
				Application::instance()->registerAlienFile(file_info.absoluteFilePath());
			}
		}
	}

	if (file_dates.count() == 0) {
		return QStringList();
	}

	int since_pos = 0;
	if (since.isValid()) {
		auto it = std::upper_bound(file_dates.begin(), file_dates.end(), since.toMSecsSinceEpoch());
		if (it != file_dates.begin()) {
			--it;
		}
		since_pos = (int)(it - file_dates.begin());
	}
	int until_pos = file_dates.count();
	if (until.isValid()) {
		auto it = std::upper_bound(file_dates.begin(), file_dates.end(), until.toMSecsSinceEpoch());
		until_pos = (int)(it - file_dates.begin());
	}
	if (until_pos - since_pos == 1 && until_pos == file_dates.count()) { //we are on a last file, we must check until
		std::ifstream in_file;
		in_file.open(m_dir.absoluteFilePath(file_names[0]).toLocal8Bit().toStdString(), std::ios::in | std::ios::binary);
		shv::chainpack::ChainPackReader log_reader(in_file);
		shv::chainpack::RpcValue::MetaData meta_data;
		log_reader.read(meta_data);
		QDateTime file_until = rpcvalue_cast<QDateTime>(meta_data.value("until"));
		if (file_until < since) {
			return QStringList();
		}
	}
	QStringList result;
	for (int i = since_pos; i < until_pos; ++i) {
		result << m_dir.absoluteFilePath(file_names[i]);
	}
	return result;
}

QString LogDir::fileName(const QDateTime &datetime)
{
	return QString::fromStdString(shv::core::utils::ShvFileJournal::JournalContext::msecToBaseFileName(datetime.toMSecsSinceEpoch())) + ".chp";
}

QString LogDir::filePath(const QDateTime &datetime)
{
	return m_dir.absoluteFilePath(fileName(datetime));
}

QString LogDir::dirtyLogName() const
{
	return QStringLiteral("dirty.log2");
}

QString LogDir::dirtyLogPath() const
{
	return m_dir.absoluteFilePath(dirtyLogName());
}
