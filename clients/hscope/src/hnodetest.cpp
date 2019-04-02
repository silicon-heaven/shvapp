#include "hnodetest.h"
#include "confignode.h"
#include "hnodeagent.h"
#include "hnodebroker.h"

#include <shv/coreqt/log.h>
#include <shv/core/string.h>
#include <shv/iotqt/utils/shvpath.h>
#include <shv/iotqt/rpc/rpcresponsecallback.h>
#include <shv/iotqt/rpc/deviceconnection.h>

#include <QFile>
#include <QTimer>

#define logTest() shvCDebug("Test")

namespace cp = shv::chainpack;

static const char KEY_RUN_EVERY[] = "runEvery";
static const char KEY_DISABLED[] = "disabled";
static const char KEY_SCRIPT[] = "script";
static const char KEY_ENV[] = "env";

static const char METH_RUN_TEST[] = "runTest";

static std::vector<cp::MetaMethod> meta_methods;
static std::vector<cp::MetaMethod> meta_methods_extra {
	{METH_RUN_TEST, cp::MetaMethod::Signature::RetVoid, 0, cp::Rpc::GRANT_COMMAND},
};

HNodeTest::HNodeTest(const std::string &node_id, HNode *parent)
	: Super(node_id, parent)
{
	shvDebug() << "creating:" << metaObject()->className() << node_id;

	if(meta_methods.empty()) {
		for(const cp::MetaMethod &mm : *m_methods)
			meta_methods.push_back(mm);
		for(const cp::MetaMethod &mm : meta_methods_extra)
			meta_methods.push_back(mm);
	}
	m_methods = &meta_methods;
	m_runTimer = new QTimer(this);
	connect(m_runTimer, &QTimer::timeout, this, &HNodeTest::runTestSafe);

	connect(this, &HNode::statusChanged, [this]() { updateOverallStatus(); });

	HNodeBroker *pbnd = parentBrokerNode();
	connect(pbnd, &HNodeBroker::brokerConnectedChanged, this, &HNodeTest::onParentBrokerConnectedChanged);
}

void HNodeTest::load()
{
	logTest() << shvPath() << "load config.";
	Super::load();

	m_runCount = 0;
	m_disabled = configValueOnPath(KEY_DISABLED).toBool();
	if(m_disabled) {
		logTest() << "\t" << "disabled";
		m_runTimer->stop();
		setStatus(NodeStatus{NodeStatus::Value::Ok, "Test disabled"});
		return;
	}

	shv::chainpack::RpcValue run_every = configValueOnPath(KEY_RUN_EVERY);
	int interval = 0;
	if(run_every.isString()) {
		shv::core::StringView sv = run_every.toString();
		if(sv.endsWith('s'))
			interval = sv.mid(0, sv.length() - 1).toInt();
		else if(sv.endsWith('m'))
			interval = sv.mid(0, sv.length() - 1).toInt() * 60;
		else if(sv.endsWith('h'))
			interval = sv.mid(0, sv.length() - 1).toInt() * 60 * 60;
		else if(sv.endsWith('d'))
			interval = sv.mid(0, sv.length() - 1).toInt() * 60 * 60 * 24;
		else
			interval = sv.toInt();
	}
	else {
		interval = run_every.toInt();
	}
	logTest() << "\t" << "setting interval to:" << interval << "sec";
	if(interval == 0) {
		/// run test just once
		m_runTimer->stop();
	}
	else {
		m_runTimer->start(interval * 1000);
	}
	runTestFirstTime();
}

shv::chainpack::RpcValue HNodeTest::callMethodRq(const shv::chainpack::RpcRequest &rq)
{
	shv::chainpack::RpcValue shv_path = rq.shvPath();
	if(shv_path.toString().empty()) {
		const shv::chainpack::RpcValue::String &method = rq.method().toString();
		if(method == METH_RUN_TEST) {
			runTest(rq);
			return cp::RpcValue();
		}
	}
	return Super::callMethodRq(rq);
}

void HNodeTest::onParentBrokerConnectedChanged(bool is_connected)
{
	if(is_connected) {
		runTestFirstTime();
	}
}

void HNodeTest::runTestFirstTime()
{
	if(m_disabled)
		return;
	if(m_runCount > 0)
		return;
	HNodeBroker *bnd = brokerNode();
	if(bnd->rpcConnection()->isBrokerConnected()) {
		runTestSafe();
	}
}

void HNodeTest::runTestSafe()
{
	try {
		runTest(cp::RpcRequest());
	}
	catch (std::exception &e) {
		logTest() << "Running test:" << shvPath() << "exception:" << e.what();
	}
}

void HNodeTest::runTest(const shv::chainpack::RpcRequest &rq)
{
	logTest() << "Running test:" << shvPath();
	m_runCount++;
	HNodeAgent *agnd = agentNode();
	if(!agnd)
		SHV_EXCEPTION("Parent agent node missing!");
	HNodeBroker *bnd = brokerNode();
	if(!bnd)
		SHV_EXCEPTION("Parent broker node missing!");

	std::string shv_path = agnd->agentShvPath();
	const std::string &script_fn = templatesDir() + '/' + configValueOnPath(KEY_SCRIPT).toString();
	QFile f(QString::fromStdString(script_fn));
	if(!f.open(QFile::ReadOnly))
		SHV_EXCEPTION("Cannot open test script file " + script_fn + " for reading!");
	QByteArray ba = f.readAll();
	std::string script(ba.constData(), (unsigned)ba.length());
	const shv::chainpack::RpcValue env = configValueOnPath(KEY_ENV);

	int rq_id = bnd->rpcConnection()->nextRequestId();
	cp::RpcResponse resp1 = rq.makeResponse();
	shv::iotqt::rpc::RpcResponseCallBack *cb = new shv::iotqt::rpc::RpcResponseCallBack(brokerNode()->rpcConnection(), rq_id, this);
	cb->start(20000, this, [this, resp1](const cp::RpcResponse &resp) {
		cp::RpcResponse resp2 = resp1;
		if(resp.isValid()) {
			if(resp.isError()) {
				logTest() << "Running test:" << shvPath() << "ERROR:" << resp.error().toString();
				resp2.setError(resp.error());
				NodeStatus st{NodeStatus::Value::Error, resp.error().toString()};
				logTest() << "\t setting status to:" << st.toRpcValue().toCpon();
				setStatus(st);
			}
			else {
				logTest() << "Running test:" << shvPath() << "result:" << resp.result().toCpon();
				resp2.setResult(resp.result());
				const std::string st_str = resp.result().toList().value(1).toString();
				std::string err;
				cp::RpcValue st_rpc = cp::RpcValue::fromCpon(st_str, &err);
				if(err.empty()) {
					NodeStatus st = NodeStatus::fromRpcValue(st_rpc);
					logTest() << "\t setting status to:" << st.toRpcValue().toCpon();
					setStatus(st);
				}
				else {
					shvWarning() << "Malformed test result:" << st_str << "error:" << err;
					NodeStatus st;
					logTest() << "\t setting status to:" << st.toRpcValue().toCpon();
					setStatus(st);
				}
			}
		}
		else {
			logTest() << "Running test:" << shvPath() << "TIMEOUT!";
			resp2.setError(cp::RpcResponse::Error::createSyncMethodCallTimeout("Test invocation timeout!"));
			NodeStatus st{NodeStatus::Value::Error, "Test invocation timeout!"};
			logTest() << "\t setting status to:" << st.toRpcValue().toCpon();
			setStatus(st);
		}
		this->appRpcConnection()->sendMessage(resp2);
	});
	bnd->rpcConnection()->callShvMethod(rq_id, shv_path, cp::Rpc::METH_RUN_SCRIPT, cp::RpcValue::List{script, 1, env});
}

HNodeAgent *HNodeTest::agentNode()
{
	return findParent<HNodeAgent*>();
}

HNodeBroker *HNodeTest::brokerNode()
{
	return findParent<HNodeBroker*>();
}

