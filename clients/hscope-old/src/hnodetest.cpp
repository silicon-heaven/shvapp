#include "hnodetest.h"
#include "confignode.h"
#include "hnodeagent.h"
#include "hnodebroker.h"

#include <shv/coreqt/log.h>
#include <shv/core/string.h>
#include <shv/core/utils/shvpath.h>
#include <shv/iotqt/rpc/rpcresponsecallback.h>
#include <shv/iotqt/rpc/deviceconnection.h>
#include <shv/iotqt/rpc/rpc.h>

#include <QDateTime>
#include <QCryptographicHash>
#include <QFile>
#include <QTimer>

#define logTest() shvCMessage("Test")

namespace cp = shv::chainpack;

static const char KEY_RUN_EVERY[] = "runEvery";
static const char KEY_DISABLED[] = "disabled";
static const char KEY_SCRIPT[] = "script";
static const char KEY_ENV[] = "env";

static const char METH_RUN_TEST[] = "runTest";
static const char METH_RECENT_RUN[] = "recentRun";
static const char METH_NEXT_RUN[] = "nextRun";

static std::vector<cp::MetaMethod> meta_methods;
static std::vector<cp::MetaMethod> meta_methods_extra {
	{METH_RUN_TEST, cp::MetaMethod::Signature::RetVoid, 0, cp::Rpc::ROLE_COMMAND},
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

	//connect(this, &HNode::statusChanged, [this]() { updateOverallStatus(); });

	HNodeBroker *pbnd = parentBrokerNode();
	connect(pbnd, &HNodeBroker::brokerConnectedChanged, this, &HNodeTest::onParentBrokerConnectedChanged);
}

std::string HNodeTest::templateFileName()
{
	return nodeId() + ".config";
}

void HNodeTest::load()
{
	logTest() << shvPath() << "load config.";
	Super::load();

	m_runCount = 0;
	m_disabled = configValueOnPath(KEY_DISABLED).toBool();
	if(m_disabled) {
		logTest() << "\t" << "disabled";
		m_nextRun = cp::RpcValue();
		m_runTimer->stop();
		setStatus(NodeStatus{NodeStatus::Level::Ok, "Test disabled"});
		return;
	}

	shv::chainpack::RpcValue run_every = configValueOnPath(KEY_RUN_EVERY);
	int interval = 0;
	if(run_every.isString()) {
		shv::core::StringView sv = run_every.asString();
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
		m_nextRun = cp::RpcValue();
		m_runTimer->stop();
	}
	else {
		cp::RpcValue::DateTime dt = cp::RpcValue::fromValue(QDateTime::currentDateTime()).toDateTime();
		dt.setMsecsSinceEpoch(dt.msecsSinceEpoch() + interval * 1000);
		m_nextRun = dt;
		m_runTimer->start(interval * 1000);
	}
	QTimer::singleShot(0, this, &HNodeTest::runTestFirstTime);
}

shv::chainpack::RpcValue HNodeTest::callMethodRq(const shv::chainpack::RpcRequest &rq)
{
	shv::chainpack::RpcValue shv_path = rq.shvPath();
	if(shv_path.asString().empty()) {
		const shv::chainpack::RpcValue::String &method = rq.method().asString();
		if(method == METH_RUN_TEST) {
			runTest(rq, USE_SCRIPT_CACHE);
			return cp::RpcValue();
		}
		if(method == METH_RECENT_RUN) {
			return m_recentRun.isValid()? m_recentRun: cp::RpcValue(nullptr);
		}
		if(method == METH_NEXT_RUN) {
			return m_nextRun.isValid()? m_nextRun: cp::RpcValue(nullptr);
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
	runTestSafe();
}

void HNodeTest::runTestSafe()
{
	try {
		HNodeBroker *bnd = brokerNode();
		if(bnd->rpcConnection()->isBrokerConnected()) {
			runTest(cp::RpcRequest(), USE_SCRIPT_CACHE);
		}
	}
	catch (std::exception &e) {
		logTest() << "Running test:" << shvPath() << "exception:" << e.what();
	}
}

void HNodeTest::runTest(const shv::chainpack::RpcRequest &rq, bool use_script_cache)
{
	int test_no = m_runCount++;
	m_recentRun = cp::RpcValue::fromValue(QDateTime::currentDateTime());
	logTest() << test_no << "Running test:" << shvPath() << "use_script_cache:" << use_script_cache << "rq:" << rq.toCpon();
	HNodeAgent *agnd = agentNode();
	if(!agnd)
		SHV_EXCEPTION("Parent agent node missing!");
	HNodeBroker *bnd = brokerNode();
	if(!bnd)
		SHV_EXCEPTION("Parent broker node missing!");

	std::string shv_path = agnd->agentShvPath();
	const std::string script_fn = templatesDir() + '/' + configValueOnPath(KEY_SCRIPT).toString();
	QFile f(QString::fromStdString(script_fn));
	if(!f.open(QFile::ReadOnly))
		SHV_EXCEPTION("Cannot open test script file " + script_fn + " for reading!");
	QByteArray script = f.readAll();
	//QByteArray sha1;
	if(use_script_cache) {
		QCryptographicHash ch(QCryptographicHash::Sha1);
		ch.addData(script);
		script = ch.result().toHex();
		logTest() << test_no << "\t script SHA1:" << script.toStdString();
	}
	const shv::chainpack::RpcValue env = configValueOnPath(KEY_ENV);
	logTest() << "\tenv:" << env.toCpon();

	int rq_id = bnd->rpcConnection()->nextRequestId();
	cp::RpcRequest rq1 = rq;
	shv::iotqt::rpc::RpcResponseCallBack *cb = new shv::iotqt::rpc::RpcResponseCallBack(brokerNode()->rpcConnection(), rq_id, this);
	cb->start(20000, this, [this, rq1, test_no](const cp::RpcResponse &resp) {
		cp::RpcResponse resp2;
		if(rq1.isRequest())
			resp2 = rq1.makeResponse();
		if(resp.isValid()) {
			if(resp.isError()) {
				cp::RpcResponse::Error err = resp.error();
				if(err.code() == cp::RpcResponse::Error::UserCode) {
					logTest() << test_no << "Test response:" << shvPath() << "script not found in cache." << err.toString();
					runTest(rq1, !USE_SCRIPT_CACHE);
				}
				else {
					logTest() << test_no << "Test response:" << shvPath() << "ERROR:" << err.toString();
					if(rq1.isRequest())
						resp2.setError(err);
					NodeStatus st{NodeStatus::Level::Error, err.toString()};
					logTest() << test_no << "\t setting status to:" << st.toRpcValue().toCpon();
					setStatus(st);
				}
			}
			else {
				logTest() << test_no << "Test response:" << shvPath() << "result:" << resp.result().toCpon();
				if(rq1.isRequest())
					resp2.setResult(resp.result());
				int exit_code = resp.result().toList().value(0).toInt();
				if(exit_code == 0) {
					const std::string st_str = resp.result().toList().value(1).toString();
					std::string err;
					cp::RpcValue st_rpc = cp::RpcValue::fromCpon(st_str, &err);
					if(err.empty()) {
						NodeStatus st = NodeStatus::fromRpcValue(st_rpc);
						logTest() << test_no << "\t setting status to:" << st.toRpcValue().toCpon();
						setStatus(st);
					}
					else {
						shvWarning() << "Malformed test result:" << st_str << "error:" << err;
						NodeStatus st;
						logTest() << test_no << "\t setting status to:" << st.toRpcValue().toCpon();
						setStatus(st);
					}
				}
				else {
					shvWarning() << "Test returned error exit code:" << exit_code;
					NodeStatus st;
					logTest() << test_no << "\t setting status to:" << st.toRpcValue().toCpon();
					setStatus(st);
				}
			}
		}
		else {
			logTest() << test_no << "Running test:" << shvPath() << "TIMEOUT!";
			if(rq1.isRequest())
				resp2.setError(cp::RpcResponse::Error::createSyncMethodCallTimeout("Test invocation timeout!"));
			NodeStatus st{NodeStatus::Level::Error, "Test invocation timeout!"};
			logTest() << test_no << "\t setting status to:" << st.toRpcValue().toCpon();
			setStatus(st);
		}
		if(rq1.isRequest())
			this->appRpcConnection()->sendMessage(resp2);
		emit runTestFinished();
	});
	logTest() << test_no << "call shv:" << shv_path << cp::Rpc::METH_RUN_SCRIPT << cp::RpcValue(cp::RpcValue::List{script.toStdString(), 1, env}).toCpon();
	bnd->rpcConnection()->callShvMethod(rq_id, shv_path, cp::Rpc::METH_RUN_SCRIPT, cp::RpcValue::List{script.toStdString(), 1, env});
}

HNodeAgent *HNodeTest::agentNode()
{
	return findParent<HNodeAgent*>();
}

HNodeBroker *HNodeTest::brokerNode()
{
	return findParent<HNodeBroker*>();
}

