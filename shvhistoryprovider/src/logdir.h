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
	bool remove(const QString &file) { return m_dir.remove(file); }
	bool rename(const QString &old_name, const QString &new_name) { return m_dir.rename(old_name, new_name); }
	void refresh() { m_dir.refresh(); }
	QString absoluteFilePath(const QString &file) const { return m_dir.absoluteFilePath(file); }

private:
	static QString dirPath(const QString &shv_path);

	QDir m_dir;
};

#endif // LOGDIR_H
