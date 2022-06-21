#ifndef SHVJOURNALNODE_H
#define SHVJOURNALNODE_H

#include <shv/iotqt/node/shvnode.h>
#include <shv/iotqt/node/localfsnode.h>

class ShvJournalNode : public shv::iotqt::node::LocalFSNode
{
	Q_OBJECT

	using Super = shv::iotqt::node::LocalFSNode;

public:
	ShvJournalNode(const QString& site_shv_path);

	size_t methodCount(const StringViewList& shv_path) override;
	const shv::chainpack::MetaMethod* metaMethod(const StringViewList& shv_path, size_t ix) override;
	shv::chainpack::RpcValue callMethodRq(const shv::chainpack::RpcRequest &rq) override;

private:
	void onRpcMessageReceived(const shv::chainpack::RpcMessage &msg);

	std::string m_siteShvPath;
	QString m_remoteLogShvPath;
	QString m_cacheDirPath;
};
#endif /*SHVJOURNALNODE_H*/
