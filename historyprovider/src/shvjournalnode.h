#ifndef SHVJOURNALNODE_H
#define SHVJOURNALNODE_H

#include "logtype.h"

#include <shv/iotqt/node/shvnode.h>
#include <shv/iotqt/node/localfsnode.h>

struct SlaveHpInfo {
	bool is_leaf;
	LogType log_type;
	std::string shv_path;
};

class ShvJournalNode : public shv::iotqt::node::LocalFSNode
{
	Q_OBJECT

	using Super = shv::iotqt::node::LocalFSNode;

public:
	ShvJournalNode(const std::vector<SlaveHpInfo>& slave_hps, ShvNode* parent = nullptr);

	size_t methodCount(const StringViewList& shv_path) override;
	const shv::chainpack::MetaMethod* metaMethod(const StringViewList& shv_path, size_t ix) override;
	shv::chainpack::RpcValue callMethodRq(const shv::chainpack::RpcRequest &rq) override;
	void sanitizeSize();
	void trimDirtyLog(const QString& cache_dir_path);
	void syncLog(const std::string& path, const std::function<void(shv::chainpack::RpcResponse::Error)>);

private:
	void onRpcMessageReceived(const shv::chainpack::RpcMessage &msg);
	qint64 calculateCacheDirSize() const;

	std::vector<SlaveHpInfo> m_slaveHps;
	QString m_remoteLogShvPath;
	QString m_cacheDirPath;
	int64_t m_dirtyLogFirstTimestamp;
};
#endif /*SHVJOURNALNODE_H*/
