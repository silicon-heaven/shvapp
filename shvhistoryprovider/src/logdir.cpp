#include "application.h"
#include "appclioptions.h"
#include "logdir.h"

#include <QDateTime>
#include <QRegularExpression>
#include <QVector>

#include <shv/core/utils/shvfilejournal.h>

LogDir::LogDir(const QString &shv_path)
	: m_dir(dirPath(shv_path), "*.chp", QDir::SortFlag::Name, QDir::Filter::Files)
{
	if (!m_dir.exists()) {
		QDir(QString::fromStdString(Application::instance()->cliOptions()->dataDir())).mkpath(shv_path);
		m_dir.refresh();
	}
}

QString LogDir::dirPath(const QString &shv_path)
{
	return QString::fromStdString(Application::instance()->cliOptions()->dataDir()) + QDir::separator() + shv_path;
}

QStringList LogDir::findFiles(const QDateTime &since, const QDateTime &until)
{
	QFileInfoList file_infos = m_dir.entryInfoList();
	QStringList file_names;
	QVector<int64_t> file_dates;
	for (int i = 0; i < file_infos.count(); ++i) {
		QString filename = file_infos[i].fileName();
		try {
			int64_t datetime = shv::core::utils::ShvFileJournal::JournalContext::fileNameToFileMsec(filename.toStdString());
			file_names << filename;
			file_dates << datetime;
		}
		catch(...) {}
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
	QStringList result;
	for (int i = since_pos; i < until_pos; ++i) {
		result << m_dir.filePath(file_names[i]);
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
