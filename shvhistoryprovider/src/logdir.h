#ifndef LOGDIR_H
#define LOGDIR_H

#include <QDir>

class LogDir
{
public:
	LogDir(const QString &shv_path);

	QStringList findFiles(const QDateTime &since, const QDateTime &until);
	QString filePath(const QDateTime &datetime);

	QString dirtyLogName() const;
	QString dirtyLogPath() const;

	static QString fileName(const QDateTime &datetime);

	bool exists(const QString &file) const { return m_dir.exists(file); }
	void remove(const QString &file) { m_dir.remove(file); }
	void rename(const QString &old_name, const QString &new_name) { m_dir.rename(old_name, new_name); }
	QString absoluteFilePath(const QString &file) const { return m_dir.absoluteFilePath(file); }

private:
	static QString dirPath(const QString &shv_path);

	QDir m_dir;
};

#endif // LOGDIR_H
