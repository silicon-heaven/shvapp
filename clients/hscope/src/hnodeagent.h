#ifndef HNODENODE_H
#define HNODENODE_H

#include "hnode.h"

class HNodeAgent : public HNode
{
	Q_OBJECT

	using Super = HNode;
public:
	HNodeAgent(const std::string &node_id, HNode *parent);
public:
	void load() override;

	std::string agentShvPath() const;
private:
	void onParentBrokerConnectedChanged(bool is_connected);
	void onParentBrokerRpcMessageReceived(const shv::chainpack::RpcMessage &msg);
	void subscribeAgentMntChng();
private:
	//int m_agentPingRequestId = 0;
	void checkAgentConnected();
};

#endif // HNODENODE_H
