#ifndef CLIENTDIRNODE_H
#define CLIENTDIRNODE_H

#include <shv/iotqt/node/shvnode.h>

class ClientDirNode : public shv::iotqt::node::MethodsTableNode
{
	Q_OBJECT
	using Super = shv::iotqt::node::MethodsTableNode;
public:
	ClientDirNode(int client_id, shv::iotqt::node::ShvNode *parent = nullptr);
	~ClientDirNode() override {}

	shv::chainpack::RpcValue callMethod(const StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params) override;
private:
	int m_clientId = 0;
};

#endif // CLIENTDIRNODE_H
