#ifndef DIRTYLOGMANAGER_H
#define DIRTYLOGMANAGER_H

#include <shv/chainpack/rpcvalue.h>
#include <shv/iotqt/rpc/clientconnection.h>

#include <QDateTime>
#include <QObject>

class ShvSubscription;

class DirtyLogManager : public QObject
{
	Q_OBJECT
public:
	explicit DirtyLogManager(QObject *parent = nullptr);
	~DirtyLogManager();

private:
	void onShvStateChanged(shv::iotqt::rpc::ClientConnection::State state);
	void onDeviceAppeared(const QString &shv_path);
	void onDeviceDisappeared(const QString &shv_path);

	void setDirtyLogsDirty();
	void setDirtyLogDirty(const QString &shv_path);
	void writeDirtyLog(const QString &shv_path, const QString &path, const shv::chainpack::RpcValue &value, std::string domain, bool is_connected);
	void checkDirtyLog(const QString &shv_path, bool is_connected);
	void onDeviceDataChanged(const QString &path, const QString &method, const shv::chainpack::RpcValue &data);

	ShvSubscription *m_chngSubscription;
};

#endif // DIRTYLOGMANAGER_H
