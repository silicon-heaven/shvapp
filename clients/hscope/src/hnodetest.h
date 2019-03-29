#ifndef HTESTNODE_H
#define HTESTNODE_H

#include "hnode.h"

class QTimer;

class HNodeAgent;
class HNodeBroker;

class HNodeTest : public HNode
{
	Q_OBJECT

	using Super = HNode;
public:
	HNodeTest(const std::string &node_id, HNode *parent);
public:
	void load() override;
	shv::chainpack::RpcValue callMethodRq(const shv::chainpack::RpcRequest &rq) override;
protected:
	int runTest();

	HNodeAgent* agentNode();
	HNodeBroker* brokerNode();
protected:
	QTimer *m_runTimer = nullptr;
};

#endif // HTESTNODE_H
