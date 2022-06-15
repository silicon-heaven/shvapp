#ifndef SHVJOURNALNODE_H
#define SHVJOURNALNODE_H

#include <shv/iotqt/node/shvnode.h>

class ShvJournalNode : public shv::iotqt::node::ShvNode
{
	Q_OBJECT

	using Super = shv::iotqt::node::ShvNode;

public:
	ShvJournalNode(const QString& sites_path);

	size_t methodCount(const StringViewList& shv_path) override;
	const shv::chainpack::MetaMethod* metaMethod(const StringViewList& shv_path, size_t ix) override;
	shv::chainpack::RpcValue callMethodRq(const shv::chainpack::RpcRequest &rq) override;

private:

	QString m_logsPath;
	QString m_repoPath;
};
#endif /*SHVJOURNALNODE_H*/
