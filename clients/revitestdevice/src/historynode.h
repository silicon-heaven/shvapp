#ifndef HISTORYNODE_H
#define HISTORYNODE_H

#include <shv/iotqt/node/shvnode.h>

class HistoryNode : public shv::iotqt::node::ShvNode
{
	using Super = shv::iotqt::node::ShvNode;
public:
	HistoryNode(shv::iotqt::node::ShvNode *parent);

	size_t methodCount(const StringViewList &shv_path) override;
	const shv::chainpack::MetaMethod* metaMethod(const StringViewList &shv_path, size_t ix) override;
	shv::chainpack::RpcValue callMethod(const StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params, const shv::chainpack::RpcValue &user_id) override;
};

#endif // HISTORYNODE_H
