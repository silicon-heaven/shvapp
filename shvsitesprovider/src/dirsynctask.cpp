#include "dirsynctask.h"
#include "sitesproviderapp.h"

#include <shv/iotqt/rpc/deviceconnection.h>
#include <shv/iotqt/rpc/rpc.h>
#include <shv/iotqt/rpc/rpcresponsecallback.h>

#include <QCryptographicHash>
#include <QFile>

namespace cp = shv::chainpack;

DirSyncTask::DirSyncTask(AppRootNode *parent)
	: QObject(parent)
	, m_timeoutTimer(this)
{
	connect(&m_timeoutTimer, &QTimer::timeout, this, &DirSyncTask::timeout);
	connect(this, &DirSyncTask::finished, this, &DirSyncTask::deleteLater);
	m_timeoutTimer.setSingleShot(true);
	m_timeoutTimer.start(5 * 60 * 1000);
}

void DirSyncTask::addDir(const QString &shv_path)
{
	m_dirsToSync[shv_path] = DirToSync();
}

shv::chainpack::RpcValue DirSyncTask::result() const
{
	cp::RpcValue::Map map;
	cp::RpcValue::List file_list;
	RpcCallStatus overall_status = RpcCallStatus::Ok;
	for (auto it = m_filesToSync.begin(); it != m_filesToSync.end(); ++it) {
		cp::RpcValue::Map file_map;
		file_map["path"] = it.key().toStdString();
		file_map["status"] = it.value().status == RpcCallStatus::Ok ? true : false;
		if (it.value().status == RpcCallStatus::Error) {
			file_map["error"] = it.value().error;
			overall_status = RpcCallStatus::Error;
		}
		file_list.push_back(file_map);
	}
	map["synchronizedFiles"] = file_list;
	map["overallStatus"] = overall_status == RpcCallStatus::Ok ? true : false;
	return map;
}

void DirSyncTask::start()
{
	if (!m_dirsToSync.count()) {
		Q_EMIT finished(true);
		return;
	}
	for (const QString &dir : m_dirsToSync.keys()) {
		callLs(dir);
	}
}

void DirSyncTask::timeout()
{
	if (!isLsComplete()) {
		m_error = "Timeout";
		Q_EMIT finished(false);
	}
	else {
		for (FileToSync &file : m_filesToSync) {
			file.error = "Timeout";
			file.status = RpcCallStatus::Error;
		}
		Q_EMIT finished(true);
	}
}

void DirSyncTask::callLs(const QString &shv_path)
{
	auto *conn = SitesProviderApp::instance()->rpcConnection();
	int rqid = conn->nextRequestId();
	shv::iotqt::rpc::RpcResponseCallBack *cb = new shv::iotqt::rpc::RpcResponseCallBack(conn, rqid, this);
	connect(cb, &shv::iotqt::rpc::RpcResponseCallBack::finished, this, [this, shv_path](const shv::chainpack::RpcResponse &resp) {
		onLsFinished(shv_path, resp);
	});
	conn->callShvMethod(rqid, "shv/" + shv_path.toStdString(), cp::Rpc::METH_LS, cp::RpcValue());
	cb->start();
}

void DirSyncTask::onLsFinished(const QString &shv_path, const shv::chainpack::RpcResponse &resp)
{
	if (resp.isSuccess()) {
		for (const cp::RpcValue &ls_item : resp.result().toList()) {
			QString file_path = shv_path + "/" + rpcvalue_cast<QString>(ls_item);
			m_lsResult << file_path;
			m_dirsToSync[shv_path].status = RpcCallStatus::Ok;
			callDir(file_path);
		}
	}
	else {
		m_error = resp.errorString();
		Q_EMIT finished(false);
	}
	checkLsIsComplete();
}

bool DirSyncTask::isLsComplete()
{
	if (m_lsResult.count()) {
		return false;
	}
	for (const DirToSync &dir : m_dirsToSync) {
		if (dir.status == RpcCallStatus::Unfinished) {
			return false;
		}
	}
	return true;
}

void DirSyncTask::checkLsIsComplete()
{
	if (isLsComplete()) {
		for (const QString &shv_path : m_filesToSync.keys()) {
			startFileSync(shv_path);
		}
	}
}

void DirSyncTask::callDir(const QString &shv_path)
{
	auto *conn = SitesProviderApp::instance()->rpcConnection();
	int rqid = conn->nextRequestId();
	shv::iotqt::rpc::RpcResponseCallBack *cb = new shv::iotqt::rpc::RpcResponseCallBack(conn, rqid, this);
	connect(cb, &shv::iotqt::rpc::RpcResponseCallBack::finished, this, [this, shv_path](const shv::chainpack::RpcResponse &resp) {
		onDirFinished(shv_path, resp);
	});
	conn->callShvMethod(rqid, "shv/" + shv_path.toStdString(), cp::Rpc::METH_DIR, cp::RpcValue());
	cb->start();
}

void DirSyncTask::onDirFinished(const QString &shv_path, const shv::chainpack::RpcResponse &resp)
{
	if (resp.isSuccess()) {
		bool has_read = false;
		for (const cp::RpcValue &dir_item : resp.result().toList()) {
			if (dir_item.toString() == "read") {
				has_read = true;
				break;
			}
		}
		if (has_read) {
			m_filesToSync[shv_path] = FileToSync();
		}
		else {
			m_dirsToSync[shv_path] = DirToSync();
			callLs(shv_path);
		}
		m_lsResult.removeOne(shv_path);
		checkLsIsComplete();
	}
	else {
		m_error = resp.errorString();
		Q_EMIT finished(false);
	}
}

void DirSyncTask::startFileSync(const QString &shv_path)
{
	AppRootNode *root = qobject_cast<AppRootNode*>(parent());
	QString local_file_path = root->nodeLocalPath(shv_path);
	if (QFile(local_file_path).exists()) {
		getFileHash(shv_path);
	}
	else {
		getFile(shv_path);
	}
}

void DirSyncTask::getFileHash(const QString &shv_path)
{
	auto *conn = SitesProviderApp::instance()->rpcConnection();
	int rqid = conn->nextRequestId();
	shv::iotqt::rpc::RpcResponseCallBack *cb = new shv::iotqt::rpc::RpcResponseCallBack(conn, rqid, this);
	connect(cb, &shv::iotqt::rpc::RpcResponseCallBack::finished, this, [this, shv_path](const shv::chainpack::RpcResponse &resp) {
		onHashReceived(shv_path, resp);
	});
	conn->callShvMethod(rqid, "shv/" + shv_path.toStdString(), "hash", cp::RpcValue());
	cb->start();
}

void DirSyncTask::onHashReceived(const QString &shv_path, const shv::chainpack::RpcResponse &resp)
{
	if (resp.isSuccess()) {
		std::string remote_hash = resp.result().toString();
		AppRootNode *root = qobject_cast<AppRootNode*>(parent());
		std::string bytes = root->readFile(shv_path).toString();
		QCryptographicHash h(QCryptographicHash::Sha1);
		h.addData(bytes.data(), bytes.size());
		std::string local_hash = h.result().toHex().toStdString();
		if (local_hash != remote_hash) {
			getFile(shv_path);
		}
		else {
			m_filesToSync[shv_path].status = RpcCallStatus::Ok;
			checkSyncFilesIsComplete();
		}
	}
	else {
		m_filesToSync[shv_path].error = resp.errorString();
		m_filesToSync[shv_path].status = RpcCallStatus::Error;
		checkSyncFilesIsComplete();
	}
}

void DirSyncTask::getFile(const QString &shv_path)
{
	auto *conn = SitesProviderApp::instance()->rpcConnection();
	int rqid = conn->nextRequestId();
	shv::iotqt::rpc::RpcResponseCallBack *cb = new shv::iotqt::rpc::RpcResponseCallBack(conn, rqid, this);
	connect(cb, &shv::iotqt::rpc::RpcResponseCallBack::finished, this, [this, shv_path](const shv::chainpack::RpcResponse &resp) {
		onFileReceived(shv_path, resp);
	});
	conn->callShvMethod(rqid, "shv/" + shv_path.toStdString(), "read", cp::RpcValue());
	cb->start();
}

void DirSyncTask::onFileReceived(const QString &shv_path, const shv::chainpack::RpcResponse &resp)
{
	if (resp.isSuccess()) {
		std::string remote_hash = resp.result().toString();
		AppRootNode *root = qobject_cast<AppRootNode*>(parent());
		root->writeFile(shv_path, resp.result().toString());
		m_filesToSync[shv_path].status = RpcCallStatus::Ok;
		checkSyncFilesIsComplete();
	}
	else {
		m_filesToSync[shv_path].error = resp.errorString();
		m_filesToSync[shv_path].status = RpcCallStatus::Error;
		checkSyncFilesIsComplete();
	}
}

void DirSyncTask::checkSyncFilesIsComplete()
{
	for (const FileToSync &file : m_filesToSync) {
		if (file.status == RpcCallStatus::Unfinished) {
			return;
		}
	}
	Q_EMIT finished(true);
}