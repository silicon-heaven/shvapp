#ifndef MIGRATION_H
#define MIGRATION_H

#include <QDir>
#include <QString>

class QFileInfo;

class Migration
{
public:
	Migration();
	void exec();
	bool isNeeded() const;

private:
	void migrateDir(const QString &path);
	bool migrateFile(const QFileInfo &fileinfo, bool first);

	QDir m_root;
};

#endif // MIGRATION_H
