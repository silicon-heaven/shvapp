#ifndef FSTABNODE_H
#define FSTABNODE_H

#include <shv/iotqt/node/shvnode.h>

class FSTabNode : public shv::iotqt::node::MethodsTableNode
{
	Q_OBJECT

	using Super = shv::iotqt::node::MethodsTableNode;
public:
	FSTabNode(shv::iotqt::node::ShvNode *parent = nullptr);

	shv::chainpack::RpcValue callMethod(const StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params) override;
};

#endif // FSTABNODE_H
