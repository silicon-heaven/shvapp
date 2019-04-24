#ifndef HNODECONFIGNODE_H
#define HNODECONFIGNODE_H

#include <shv/iotqt/node/shvnode.h>

class HNode;

class ConfigNode : public shv::iotqt::node::RpcValueConfigNode
{
	Q_OBJECT

	using Super = shv::iotqt::node::RpcValueConfigNode;
public:
	ConfigNode(shv::iotqt::node::ShvNode *parent);

protected:
	HNode* parentHNode();
};

#endif // HNODECONFIGNODE_H
