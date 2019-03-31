#include "hnodeagent.h"
#include "hnodetests.h"
#include "hnodebroker.h"
#include "hscopeapp.h"

#include <shv/coreqt/log.h>
#include <shv/iotqt/rpc/deviceconnection.h>
#include <shv/iotqt/rpc/rpcresponsecallback.h>

#include <QTimer>

namespace cp = shv::chainpack;

static const char KEY_SHV_PATH[] = "shvPath";

HNodeAgent::HNodeAgent(const std::string &node_id, HNode *parent)
	: Super(node_id, parent)
{
	shvDebug() << "creating:" << metaObject()->className() << node_id;
	connect(this, &HNode::statusChanged, [this]() { updateOverallStatus(); });
}

void HNodeAgent::load()
{
	Super::load();
	for (const std::string &dir : lsConfigDir()) {
		auto *nd = new HNodeTests(dir, this);
		nd->load();
	}
	connect(HScopeApp::instance(), &HScopeApp::brokerConnectedChanged, this, &HNodeAgent::onAppBrokerConnectedChanged);
	connect(HScopeApp::instance(), &HScopeApp::rpcMessageReceived, this, &HNodeAgent::onAppRpcMessageReceived);

	QTimer::singleShot(100, this, &HNodeAgent::checkAgentConnected);
}

std::string HNodeAgent::agentShvPath() const
{
	return configValueOnPath(KEY_SHV_PATH).toString();
}

std::string HNodeAgent::templateFileName()
{
	return "agent.config.cpon";
}

void HNodeAgent::onAppBrokerConnectedChanged(bool is_connected)
{
	if(is_connected) {
		subscribeAgentMntChng();
		checkAgentConnected();
	}
}

void HNodeAgent::onAppRpcMessageReceived(const shv::chainpack::RpcMessage &msg)
{
	shvLogFuncFrame() << msg.toCpon();
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
	shv::iotqt::rpc::DeviceConnection *conn = appRpcConnection();
	if(conn && conn->isBrokerConnected()) {
		shvDebug() << "subscribing mounted for:" << agentShvPath();
		conn->callMethodSubscribe(agentShvPath(), cp::Rpc::SIG_MOUNTED_CHANGED);
	}
}

void HNodeAgent::checkAgentConnected()
{
	shv::iotqt::rpc::DeviceConnection *conn = appRpcConnection();
	if(conn && conn->isBrokerConnected()) {
		int rq_id = conn->callShvMethod(agentShvPath(), "dir", "n");
		shv::iotqt::rpc::RpcResponseCallBack *cb = new shv::iotqt::rpc::RpcResponseCallBack(conn, rq_id, this);
		cb->setTimeout(5000);
		connect(cb, &shv::iotqt::rpc::RpcResponseCallBack::finished, this, [this](const cp::RpcResponse &resp) {
			if(resp.isValid()) {
				if(resp.isError()) {
					NodeStatus st{NodeStatus::Value::Error, "Agent connected chect error: " + resp.error().toString()};
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
