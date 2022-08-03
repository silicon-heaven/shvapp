#ifndef SHVJOURNALNODE_H
#define SHVJOURNALNODE_H

#include <shv/iotqt/node/shvnode.h>
#include <shv/iotqt/node/localfsnode.h>

enum class LogType {
	PushLog,
	Normal,
	Legacy
};

class ShvJournalNode : public shv::iotqt::node::LocalFSNode
{
	Q_OBJECT

	using Super = shv::iotqt::node::LocalFSNode;

public:
	ShvJournalNode(const QString& site_shv_path, const QString& remote_log_shv_path, const LogType log_type);

	size_t methodCount(const StringViewList& shv_path) override;
	const shv::chainpack::MetaMethod* metaMethod(const StringViewList& shv_path, size_t ix) override;
	shv::chainpack::RpcValue callMethodRq(const shv::chainpack::RpcRequest &rq) override;
	void sanitizeSize();

private:
	void syncLog(const std::function<void(shv::chainpack::RpcResponse::Error)>);
	void syncLogLegacy(const std::function<void(shv::chainpack::RpcResponse::Error)>);
	void onRpcMessageReceived(const shv::chainpack::RpcMessage &msg);
	qint64 calculateCacheDirSize() const;

	std::string m_siteShvPath;
	QString m_remoteLogShvPath;
	QString m_cacheDirPath;
	LogType m_logType;
};
#endif /*SHVJOURNALNODE_H*/
