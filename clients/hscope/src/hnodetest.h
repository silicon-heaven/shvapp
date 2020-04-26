#ifndef HTESTNODE_H
#define HTESTNODE_H

#include "hnode.h"

#include <shv/chainpack/rpcvalue.h>

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
	void runTestSafe();
	void load() override;
	shv::chainpack::RpcValue callMethodRq(const shv::chainpack::RpcRequest &rq) override;

	shv::chainpack::RpcValue recentRun() const { return m_recentRun; }
	shv::chainpack::RpcValue nextRun() const { return m_nextRun; }

	Q_SIGNAL void runTestFinished();
private:
	void onParentBrokerConnectedChanged(bool is_connected);

	void runTestFirstTime();
	void runTest(const shv::chainpack::RpcRequest &rq, bool use_script_cache);

	HNodeAgent* agentNode();
	HNodeBroker* brokerNode();
protected:
	static constexpr bool USE_SCRIPT_CACHE = true;
	QTimer *m_runTimer = nullptr;
	bool m_disabled = true;
	int m_runCount = 0;
	shv::chainpack::RpcValue m_recentRun;
	shv::chainpack::RpcValue m_nextRun;
};

#endif // HTESTNODE_H
