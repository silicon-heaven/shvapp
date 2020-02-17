#include "application.h"
#include "appclioptions.h"
#include "logdir.h"

#include <QDateTime>
#include <QRegularExpression>

QString LogDir::fileNameMask("([0-9][0-9][0-9][0-9])-([0-9][0-9])-([0-9][0-9])T([0-9][0-9]):([0-9][0-9]):([0-9][0-9]).([0-9][0-9][0-9]).");

QRegularExpression LogDir::fileNameRegExp(fileNameMask);

LogDir::LogDir(const QString &shv_path)	: QDir(dirPath(shv_path), "*.chp", QDir::SortFlag::Name, QDir::Filter::Files)
{
	if (!exists()) {
		QDir(QString::fromStdString(Application::instance()->cliOptions()->dataDir())).mkpath(shv_path);
		refresh();
	}
}

QString LogDir::dirPath(const QString &shv_path)
{
	return QString::fromStdString(Application::instance()->cliOptions()->dataDir()) + separator() + shv_path;
}

QDateTime LogDir::dateTimeFromFileName(const QString &filename)
{
	QDateTime datetime;
	QRegularExpressionMatch match = fileNameRegExp.match(filename);
	if (match.hasMatch()) {
		int year = match.captured(1).toInt();
		int month = match.captured(2).toInt();
		int day = match.captured(3).toInt();
		int hour = match.captured(4).toInt();
		int minutes = match.captured(5).toInt();
		int seconds = match.captured(6).toInt();
		int miliseconds = match.captured(7).toInt();
		datetime = QDateTime(QDate(year, month, day), QTime(hour, minutes, seconds, miliseconds), Qt::UTC);
	}
	return datetime;
}

QStringList LogDir::findFiles(const QDateTime &since, const QDateTime &until)
{
	QFileInfoList file_infos = entryInfoList();
	QStringList file_names;
	QVector<QDateTime> file_dates;
	for (int i = 0; i < file_infos.count(); ++i) {
		QString filename = file_infos[i].fileName();
		QDateTime datetime = dateTimeFromFileName(filename);
		if (datetime.isValid()) {
			file_names << filename;
			file_dates << datetime;
		}
	}

	if (file_dates.count() == 0) {
		return QStringList();
	}

	int since_pos = 0;
	if (since.isValid()) {
		auto it = std::upper_bound(file_dates.begin(), file_dates.end(), since);
		if (it != file_dates.begin()) {
			--it;
		}
		since_pos = (int)(it - file_dates.begin());
	}
	int until_pos = file_dates.count();
	if (until.isValid()) {
		auto it = std::upper_bound(file_dates.begin(), file_dates.end(), until);
		until_pos = (int)(it - file_dates.begin());
	}
	QStringList result;
	for (int i = since_pos; i < until_pos; ++i) {
		result << QDir::filePath(file_names[i]);
	}
	return result;
}

QString LogDir::fileName(const QDateTime &datetime)
{
	return QString("%1-%2-%3T%4:%5:%6.%7.chp")
			.arg(datetime.date().year(), 4)
			.arg(datetime.date().month(), 2, 10, QChar('0'))
			.arg(datetime.date().day(), 2, 10, QChar('0'))
			.arg(datetime.time().hour(), 2, 10, QChar('0'))
			.arg(datetime.time().minute(), 2, 10, QChar('0'))
			.arg(datetime.time().second(), 2, 10, QChar('0'))
			.arg(datetime.time().msec(), 3, 10, QChar('0'));
}

QString LogDir::filePath(const QDateTime &datetime)
{
	return absoluteFilePath(fileName(datetime));
}

QString LogDir::dirtyLogName() const
{
	return QStringLiteral("dirty.log2");
}

QString LogDir::dirtyLogPath() const
{
	return absoluteFilePath(dirtyLogName());
}
