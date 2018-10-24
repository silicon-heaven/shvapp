#ifndef SITESNODE_H
#define SITESNODE_H

#include <shv/iotqt/node/shvnode.h>

class SitesNode : public shv::iotqt::node::ShvNode
{
	using Super = shv::iotqt::node::ShvNode;
public:
	SitesNode(shv::iotqt::node::ShvNode *parent);

	size_t methodCount(const StringViewList &shv_path) override;
	const shv::chainpack::MetaMethod* metaMethod(const StringViewList &shv_path, size_t ix) override;
	shv::chainpack::RpcValue callMethod(const StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params) override;
};

#endif // SITESNODE_H
