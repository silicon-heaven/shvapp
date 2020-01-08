#include "hnodeagent.h"
#include "hnodetests.h"
#include "hnodebroker.h"
#include "hscopeapp.h"

#include <shv/coreqt/log.h>
#include <shv/iotqt/rpc/deviceconnection.h>
#include <shv/iotqt/rpc/rpcresponsecallback.h>
#include <shv/core/utils/shvpath.h>

#include <QTimer>

namespace cp = shv::chainpack;

static const char KEY_SHV_PATH[] = "shvPath";
static const char KEY_DISABLED[] = "disabled";

HNodeAgent::HNodeAgent(const std::string &node_id, HNode *parent)
	: Super(node_id, parent)
{
	shvDebug() << "creating:" << metaObject()->className() << node_id;
	connect(this, &HNode::statusChanged, [this]() { updateOverallStatus(); });

	HNodeBroker *pbnd = parentBrokerNode();
	connect(pbnd, &HNodeBroker::brokerConnectedChanged, this, &HNodeAgent::onParentBrokerConnectedChanged);
	connect(pbnd, &HNodeBroker::rpcMessageReceived, this, &HNodeAgent::onParentBrokerRpcMessageReceived);
}

void HNodeAgent::load()
{
	Super::load();
	if(isDisabled()) {
		setStatus({NodeStatus::Value::Disabled, "Node is disabled"});
	}
	else {
		for (const std::string &dir : lsConfigDir()) {
			auto *nd = new HNodeTests(dir, this);
			nd->load();
		}

		QTimer::singleShot(100, this, &HNodeAgent::checkAgentConnected);
	}
}

std::string HNodeAgent::agentShvPath() const
{
	return configValueOnPath(KEY_SHV_PATH).toString();
}

bool HNodeAgent::isDisabled() const
{
	return configValueOnPath(KEY_DISABLED).toBool();
}

void HNodeAgent::onParentBrokerConnectedChanged(bool is_connected)
{
	if(is_connected) {
		subscribeAgentMntChng();
		checkAgentConnected();
	}
}

void HNodeAgent::onParentBrokerRpcMessageReceived(const shv::chainpack::RpcMessage &msg)
{
	shvLogFuncFrame() << shvPath() << msg.toCpon();
	if(msg.isSignal()) {
		cp::RpcSignal sig(msg);
		if(sig.method().toString() == cp::Rpc::SIG_MOUNTED_CHANGED && sig.shvPath().toString() == agentShvPath()) {
			bool is_mounted = sig.params().toBool();
			if(is_mounted) {
				setStatus({NodeStatus::Value::Ok, "Agent connected."});
			}
			else {
				setStatus({NodeStatus::Value::Error, "Agent disconnected."});
			}
		}
	}
}

void HNodeAgent::subscribeAgentMntChng()
{
	HNodeBroker *pbnd = parentBrokerNode();
	if(pbnd) {
		shv::iotqt::rpc::ClientConnection *conn = pbnd->rpcConnection();
		if(conn && conn->isBrokerConnected()) {
			shvDebug() << "subscribing mounted for:" << agentShvPath();
			conn->callMethodSubscribe(agentShvPath(), cp::Rpc::SIG_MOUNTED_CHANGED);
		}
	}
}

void HNodeAgent::checkAgentConnected()
{
	HNodeBroker *pbnd = parentBrokerNode();
	if(pbnd) {
		shv::iotqt::rpc::ClientConnection *conn = pbnd->rpcConnection();
		if(conn && conn->isBrokerConnected()) {
			int rq_id = conn->callShvMethod(agentShvPath(), "dir", "n");
			shv::iotqt::rpc::RpcResponseCallBack *cb = new shv::iotqt::rpc::RpcResponseCallBack(conn, rq_id, this);
			cb->start(5000, this, [this](const cp::RpcResponse &resp) {
				if(resp.isValid()) {
					if(resp.isError()) {
						NodeStatus st{NodeStatus::Value::Error, "Agent connected check error: " + resp.error().toString()};
						//logTest() << "\t setting status to:" << st.toRpcValue().toCpon();
						setStatus(st);
					}
					else {
						NodeStatus st{NodeStatus::Value::Ok, "Agent connected."};
						//logTest() << "\t setting status to:" << st.toRpcValue().toCpon();
						setStatus(st);
					}
				}
				else {
					NodeStatus st{NodeStatus::Value::Error, "Agent connected chect timeout."};
					//logTest() << "\t setting status to:" << st.toRpcValue().toCpon();
					setStatus(st);
				}
			});
		}
		else {
			// do nothing, broker node will fire error state
		}
	}
}
