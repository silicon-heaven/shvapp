#ifndef DEVICELOGDIR_H
#define DEVICELOGDIR_H

#include <QDir>

class LogDir : public QDir
{
public:
	LogDir(const QString &shv_path);

	QStringList findFiles(const QDateTime &since, const QDateTime &until);
	QString filePath(const QDateTime &datetime);

	QString dirtyLogName() const;
	QString dirtyLogPath() const;

	static QString fileName(const QDateTime &datetime);

private:
	static QDateTime dateTimeFromFileName(const QString &filename);
	static QString dirPath(const QString &shv_path);

	static QRegularExpression fileNameRegExp;
	static QString fileNameMask;
};

#endif // DEVICELOGDIR_H
