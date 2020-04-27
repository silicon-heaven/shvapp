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

HNodeAgent::HNodeAgent(const std::string &node_id, HNode *parent)
	: Super(node_id, parent)
{
	shvDebug() << "creating:" << metaObject()->className() << node_id;

	//connect(this, &HNode::statusChanged, [this]() { updateOverallStatus(); });
	HNodeBroker *pbnd = parentBrokerNode();
	connect(pbnd, &HNodeBroker::brokerConnectedChanged, this, &HNodeAgent::onParentBrokerConnectedChanged);
	connect(pbnd, &HNodeBroker::rpcMessageReceived, this, &HNodeAgent::onParentBrokerRpcMessageReceived);
}

void HNodeAgent::load()
{
	Super::load();
	if(isDisabled()) {
		SHV_SAFE_DELETE(m_checkAgentConnectedTimer);
	}
	else {
		for (const std::string &dir : lsConfigDir()) {
			auto *nd = new HNodeTests(dir, this);
			nd->load();
		}

		if(m_checkAgentConnectedTimer == nullptr) {
			static constexpr int INTERVAL = 10 * 60 * 1000;
			// event id mntchng wil work in 90% of cases, check connected every INTERVAL
			// for example shvbroker.reloadConfig() might not fire any event in HScope, but user rights might be changed
			m_checkAgentConnectedTimer = new QTimer(this);
			connect(m_checkAgentConnectedTimer, &QTimer::timeout, this, &HNodeAgent::checkAgentConnected);
			m_checkAgentConnectedTimer->start(INTERVAL);
		}

		QTimer::singleShot(100, this, &HNodeAgent::checkAgentConnected);
	}
}

std::string HNodeAgent::agentShvPath() const
{
	return configValueOnPath(KEY_SHV_PATH).toString();
}

void HNodeAgent::onParentBrokerConnectedChanged(bool is_connected)
{
	shvMessage() << "parent broker connected changed:" << is_connected;
	if(is_connected) {
		if(!isDisabled()) {
			subscribeAgentMntChng();
			checkAgentConnected();
		}
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
				setStatus({NodeStatus::Level::Ok, "Agent connected."});
				reload();
			}
			else {
				setStatus({NodeStatus::Level::Error, "Agent disconnected."});
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
			shvMessage() << "subscribing mounted for:" << agentShvPath();
			int rq_id = conn->callMethodSubscribe(agentShvPath(), cp::Rpc::SIG_MOUNTED_CHANGED);
			shv::iotqt::rpc::RpcResponseCallBack *cb = new shv::iotqt::rpc::RpcResponseCallBack(conn, rq_id, this);
			cb->start(5000, this, [this](const cp::RpcResponse &resp) {
				if(resp.result() == true) {
					shvMessage() << "\t OK";
					return;
				}
				shvMessage() << "\t ERROR";
				if(resp.isError()) {
					NodeStatus st{NodeStatus::Level::Error, "Agent subscribe mount change error: " + resp.error().toString()};
					setStatus(st);
				}
				else {
					NodeStatus st{NodeStatus::Level::Error, "Agent subscribe mount change empty response received."};
					//logTest() << "\t setting status to:" << st.toRpcValue().toCpon();
					setStatus(st);
				}
			});
		}
	}
}

void HNodeAgent::checkAgentConnected()
{
	if(isDisabled())
		return;
	HNodeBroker *pbnd = parentBrokerNode();
	if(pbnd) {
		shv::iotqt::rpc::ClientConnection *conn = pbnd->rpcConnection();
		if(conn && conn->isBrokerConnected()) {
			int rq_id = conn->callShvMethod(agentShvPath(), "dir", "n");
			shv::iotqt::rpc::RpcResponseCallBack *cb = new shv::iotqt::rpc::RpcResponseCallBack(conn, rq_id, this);
			cb->start(5000, this, [this](const cp::RpcResponse &resp) {
				if(resp.isValid()) {
					if(resp.isError()) {
						NodeStatus st{NodeStatus::Level::Error, "Agent connected check error: " + resp.error().toString()};
						//logTest() << "\t setting status to:" << st.toRpcValue().toCpon();
						setStatus(st);
					}
					else {
						NodeStatus st{NodeStatus::Level::Ok, "Agent connected."};
						//logTest() << "\t setting status to:" << st.toRpcValue().toCpon();
						setStatus(st);
					}
				}
				else {
					NodeStatus st{NodeStatus::Level::Error, "Agent connected chect timeout."};
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
