#ifndef SYNCTASK_H
#define SYNCTASK_H

#include <QObject>
#include <QTimer>

#include "nodes.h"

enum class RpcCallStatus { Unfinished, Ok, Error };

struct DirToSync
{
	RpcCallStatus status = RpcCallStatus::Unfinished;
};

struct FileToSync
{
	std::string error;
	RpcCallStatus status = RpcCallStatus::Unfinished;
};

class DirSyncTask : public QObject
{
	Q_OBJECT

public:
	DirSyncTask(const QString &master_broker_path, AppRootNode *parent);

	void addDir(const QString &shv_path);
	shv::chainpack::RpcValue result() const;
	const std::string &error() const { return m_error; }

	void start();
	Q_SIGNAL void finished(bool success);

private:
	void timeout();

	void callLs(const QString &shv_path);
	void onLsFinished(const QString &shv_path, const shv::chainpack::RpcResponse &resp);
	bool isLsComplete();
	void checkLsIsComplete();

	void callDir(const QString &shv_path);
	void onDirFinished(const QString &shv_path, const shv::chainpack::RpcResponse &resp);

	void startFileSync(const QString &shv_path);
	void getFileHash(const QString &shv_path);
	void onHashReceived(const QString &shv_path, const shv::chainpack::RpcResponse &resp);
	void getFile(const QString &shv_path);
	void onFileReceived(const QString &shv_path, const shv::chainpack::RpcResponse &resp);
	void checkSyncFilesIsComplete();
	std::string makeShvPath(const QString &site_path);

	QMap<QString, DirToSync> m_dirsToSync;
	QMap<QString, FileToSync> m_filesToSync;
	QStringList m_lsResult;
	std::string m_error;
	QTimer m_timeoutTimer;
	QStringList m_offlineDevices;
	QString m_masterBrokerPath;
};

#endif // SYNCTASK_H
