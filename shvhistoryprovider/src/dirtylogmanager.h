#ifndef DIRTYLOGMANAGER_H
#define DIRTYLOGMANAGER_H

#include <shv/chainpack/rpcvalue.h>
#include <shv/iotqt/rpc/clientconnection.h>

#include <QDateTime>
#include <QObject>

namespace shv { namespace core { namespace utils { class ShvJournalEntry; }}}

class DirtyLogManager : public QObject
{
	Q_OBJECT
public:
	explicit DirtyLogManager(QObject *parent = nullptr);
	~DirtyLogManager();

private:
	void onShvStateChanged(shv::iotqt::rpc::ClientConnection::State state);
	void onDeviceAppeared(const QString &site_path);
	void onDeviceDisappeared(const QString &site_path);

	void insertDataMissingToDirtyLog();
	void insertDataMissingToDirtyLog(const QString &site_path);
	void writeDirtyLog(const QString &site_path, const QString &property, const shv::chainpack::RpcValue &value, int64_t timestamp, std::string domain, bool is_connected);
	void writeDirtyLog(const QString &site_path, const shv::core::utils::ShvJournalEntry &entry, bool is_connected);
	void checkDirtyLog(const QString &site_path, bool is_connected);
	void onDeviceDataChanged(const QString &site_path, const QString &property, const QString &method, const shv::chainpack::RpcValue &data);
};

#endif // DIRTYLOGMANAGER_H
