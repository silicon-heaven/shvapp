#include "bfsviewapp.h"
#include "appclioptions.h"
#include "settings.h"

#include <shv/iotqt/rpc/deviceconnection.h>
#include <shv/iotqt/rpc/tunnelconnection.h>
#include <shv/iotqt/rpc/tunnelhandle.h>
#include <shv/iotqt/node/shvnodetree.h>
#include <shv/iotqt/node/localfsnode.h>
#include <shv/coreqt/log.h>
#include <shv/chainpack/metamethod.h>

#include <shv/core/stringview.h>

//#include <QProcess>
#include <QSettings>
#include <QTimer>

namespace cp = shv::chainpack;

static const char BFS1_PWR_STATUS[] = "bfs1/pwrStatus";

static const char METH_SIM_SET[] = "sim_set";

static std::vector<cp::MetaMethod> meta_methods_root {
	{cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, false},
	{cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, false},
	{cp::Rpc::METH_DEVICE_ID, cp::MetaMethod::Signature::RetVoid, false},
	{cp::Rpc::METH_MOUNT_POINT, cp::MetaMethod::Signature::RetVoid, false},
	{cp::Rpc::METH_APP_NAME, cp::MetaMethod::Signature::RetVoid, false},
	{cp::Rpc::METH_CONNECTION_TYPE, cp::MetaMethod::Signature::RetVoid, false},
};

size_t AppRootNode::methodCount()
{
	return meta_methods_root.size();
}

const shv::chainpack::MetaMethod *AppRootNode::metaMethod(size_t ix)
{
	if(meta_methods_root.size() <= ix)
		SHV_EXCEPTION("Invalid method index: " + std::to_string(ix) + " of: " + std::to_string(meta_methods_root.size()));
	return &(meta_methods_root[ix]);
}

shv::chainpack::RpcValue AppRootNode::call(const std::string &method, const shv::chainpack::RpcValue &params)
{
	if(method == cp::Rpc::METH_APP_NAME) {
		return QCoreApplication::instance()->applicationName().toStdString();
	}
	if(method == cp::Rpc::METH_DEVICE_ID) {
		return BfsViewApp::instance()->cliOptions()->deviceId().toStdString();
	}
	if(method == cp::Rpc::METH_MOUNT_POINT) {
		return BfsViewApp::instance()->rpcConnection()->brokerMountPoint();
	}
	if(method == cp::Rpc::METH_CONNECTION_TYPE) {
		return BfsViewApp::instance()->rpcConnection()->connectionType();
	}
	return Super::call(method, params);
}
/*
shv::chainpack::RpcValue AppRootNode::processRpcRequest(const shv::chainpack::RpcRequest &rq)
{
	if(rq.method() == METH_OPEN_RSH) {
		BfsViewApp *app = BfsViewApp::instance();
		app->openRsh(rq);
		return cp::RpcValue();
	}
	return Super::processRpcRequest(rq);
}
*/
static std::vector<cp::MetaMethod> meta_methods_pwrstatus {
	{cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, false},
	{cp::Rpc::METH_GET, cp::MetaMethod::Signature::RetVoid, false},
	{cp::Rpc::NTF_VAL_CHANGED, cp::MetaMethod::Signature::VoidParam, true},
	{METH_SIM_SET, cp::MetaMethod::Signature::VoidParam, false},
};

size_t PwrStatusNode::methodCount()
{
	return meta_methods_pwrstatus.size();
}

const shv::chainpack::MetaMethod *PwrStatusNode::metaMethod(size_t ix)
{
	if(meta_methods_pwrstatus.size() <= ix)
		SHV_EXCEPTION("Invalid method index: " + std::to_string(ix) + " of: " + std::to_string(meta_methods_pwrstatus.size()));
	return &(meta_methods_pwrstatus[ix]);
}

shv::chainpack::RpcValue PwrStatusNode::call(const std::string &method, const shv::chainpack::RpcValue &params)
{
	if(method == cp::Rpc::METH_GET) {
		return pwrStatus();
	}
	if(method == METH_SIM_SET) {
		unsigned s = params.toUInt();
		setPwrStatus(s);
		return true;
	}
	return Super::call(method, params);
}

void PwrStatusNode::setPwrStatus(unsigned s)
{
	if(s == m_pwrStatus)
		return;
	m_pwrStatus = s;
	sendPwrStatusChanged();
}

void PwrStatusNode::sendPwrStatusChanged()
{
#ifndef TEST
	if(!BfsViewApp::instance()->rpcConnection()->isBrokerConnected())
		return;
	cp::RpcNotify ntf;
	ntf.setMethod(cp::Rpc::NTF_VAL_CHANGED);
	ntf.setParams(m_pwrStatus);
	ntf.setShvPath(BFS1_PWR_STATUS);
	rootNode()->emitSendRpcMesage(ntf);
#endif
}

BfsViewApp::BfsViewApp(int &argc, char **argv, AppCliOptions* cli_opts)
	: Super(argc, argv)
	, m_cliOptions(cli_opts)
{
	cp::RpcMessage::setMetaTypeExplicit(cli_opts->isMetaTypeExplicit());

	m_rpcConnection = new shv::iotqt::rpc::DeviceConnection(this);

	if(!cli_opts->serverHost_isset())
		cli_opts->setServerHost("nirvana.elektroline.cz");
	if(!cli_opts->user_isset())
		cli_opts->setUser("iot");
	if(!cli_opts->password_isset())
		cli_opts->setPassword("lub42DUB");
	if(!cli_opts->deviceId_isset())
		cli_opts->setDeviceId("bfsview-001");
	m_rpcConnection->setCliOptions(cli_opts);

	connect(m_rpcConnection, &shv::iotqt::rpc::ClientConnection::brokerConnectedChanged, this, &BfsViewApp::onBrokerConnectedChanged);
	connect(m_rpcConnection, &shv::iotqt::rpc::ClientConnection::rpcMessageReceived, this, &BfsViewApp::onRpcMessageReceived);

	AppRootNode *root = new AppRootNode();
	m_shvTree = new shv::iotqt::node::ShvNodeTree(root, this);
	m_pwrStatusNode = new PwrStatusNode();
	m_shvTree->mount(BFS1_PWR_STATUS, m_pwrStatusNode);
	connect(m_shvTree->root(), &shv::iotqt::node::ShvRootNode::sendRpcMesage, m_rpcConnection, &shv::iotqt::rpc::DeviceConnection::sendMessage);

	QTimer::singleShot(0, m_rpcConnection, &shv::iotqt::rpc::ClientConnection::open);
	if(cli_opts->pwrStatusPublishInterval() > 0) {
		shvInfo() << "pwrStatus publish interval set to:" << cli_opts->pwrStatusPublishInterval() << "sec.";
		QTimer *tm = new QTimer(this);
		connect(tm, &QTimer::timeout, [this]() {
			if(rpcConnection()->isBrokerConnected())
				m_pwrStatusNode->sendPwrStatusChanged();
		});
		tm->start(m_cliOptions->pwrStatusPublishInterval() * 1000);
	}
	loadSettings();
}

BfsViewApp::~BfsViewApp()
{
	shvInfo() << "destroying shv agent application";
}

BfsViewApp *BfsViewApp::instance()
{
	return qobject_cast<BfsViewApp *>(QCoreApplication::instance());
}

void BfsViewApp::loadSettings()
{
	QSettings qsettings;
	Settings settings(qsettings);
	//m_powerSwitchName = settings.powerSwitchName();
	m_powerFileName = settings.powerFileName();
	int inter = settings.powerFileCheckInterval();
	if(inter < 1)
		inter = 1;
	if(!m_powerFileCheckTimer) {
		m_powerFileCheckTimer = new QTimer(this);
		connect(m_powerFileCheckTimer, &QTimer::timeout, this, &BfsViewApp::checkPowerSwitchStatusFile);
	}
	shvInfo() << "Starting powerFileCheckTimer, interval:" << inter << "sec";
	m_powerFileCheckTimer->start(inter * 1000);
	checkPowerSwitchStatusFile();
}

void BfsViewApp::checkPowerSwitchStatusFile()
{
	shvDebug() << "Checking pwr staus file:" << m_powerFileName;
	QFile file(m_powerFileName);
	if (!file.open(QIODevice::ReadOnly)) {
		shvError() << "Cannot open file:" << m_powerFileName << "for reading!";
		setPwrStatus(0);
		return;
	}
	QDateTime curr_ts = QDateTime::currentDateTimeUtc();
	int new_status = 0;
	while(!file.atEnd()) {
		QByteArray ba = file.readLine().trimmed();
		if(ba.isEmpty())
			continue;
		QString line = QString::fromUtf8(ba);
		shvDebug() << line;
		QString name = line.section(' ', 0, 0);
		int status = line.section(' ', 1, 1).toInt();
		QDateTime ts = QDateTime::fromString(line.section(' ', 2, 2), QStringLiteral("yyyy-M-dTHH:mm:ss"));
		ts.setTimeSpec(Qt::UTC);
		shvDebug() << name << status << ts.toString(Qt::ISODate) << curr_ts.toString(Qt::ISODate) << ts.secsTo(curr_ts);
		if(status == 1 && ts.isValid()) {
			if(ts.secsTo(curr_ts) < 2*m_powerFileCheckTimer->interval()/1000) {
				new_status = 1;
				shvDebug() << "ts:" << ts.toString(Qt::ISODate) << "pwr:" << new_status;
				break;
			}
		}
	}
	setPwrStatus(new_status);
}

static constexpr int PLC_CONNECTED_TIMOUT_MSEC = 10*1000;

void BfsViewApp::sendGetStatusRequest()
{
	if(!m_plcConnectedCheckTimer) {
		m_plcConnectedCheckTimer = new QTimer(this);
		m_plcConnectedCheckTimer->setInterval(PLC_CONNECTED_TIMOUT_MSEC);
		connect(m_plcConnectedCheckTimer, &QTimer::timeout, this, &BfsViewApp::checkPlcConnected);
		m_plcConnectedCheckTimer->start();
	}
	m_getStatusRpcId = rpcConnection()->callShvMethod("../bfs1/status", cp::Rpc::METH_GET);
	shvDebug() << "Sending get status request id:" << m_getStatusRpcId;
}

void BfsViewApp::checkPlcConnected()
{
	shvLogFuncFrame();
	setPlcConnected(m_getStatusRpcId == 0);
	sendGetStatusRequest();
}

void BfsViewApp::setPwrStatus(unsigned u)
{
	if(pwrStatus() == u)
		return;
	m_pwrStatusNode->setPwrStatus(u);
	emit pwrStatusChanged(u);
}

unsigned BfsViewApp::pwrStatus()
{
	return m_pwrStatusNode->pwrStatus();
}

void BfsViewApp::setOmpag(bool val)
{
#ifdef TEST
	int s = bfsStatus();
	setBit(s, BfsStatus::OmpagOn, val);
	setBit(s, BfsStatus::OmpagOff, !val);
	setBfsStatus(s);
#else
	if(rpcConnection()->isBrokerConnected()) {
		rpcConnection()->callShvMethod("../bfs1", "setOmpag", val);
	}
#endif
}

void BfsViewApp::setConv(bool val)
{
#ifdef TEST
	int s = bfsStatus();
	setBit(s, BfsViewApp::BfsStatus::MswOn, val);
	setBit(s, BfsViewApp::BfsStatus::MswOff, !val);
	setBfsStatus(s);
#else
	if(rpcConnection()->isBrokerConnected()) {
		rpcConnection()->callShvMethod("../bfs1", "setConv", val);
	}
#endif
}

void BfsViewApp::onBrokerConnectedChanged(bool is_connected)
{
	if(is_connected) {
		rpcConnection()->createSubscription("../bfs1", cp::Rpc::NTF_VAL_CHANGED);
		m_pwrStatusNode->sendPwrStatusChanged();
		sendGetStatusRequest();
		//shvInfo() << "get status rq id:" << m_getStatusRpcId;
		/*
		QTimer::singleShot(0, [this]() {
			/// wait for PLC to settle up before
			m_getStatusRpcId = rpcConnection()->callShvMethod("../bfs1/status", cp::Rpc::METH_GET);
			shvInfo() << "get status rq id:" << m_getStatusRpcId;
		});
		*/
	}
}

void BfsViewApp::onRpcMessageReceived(const shv::chainpack::RpcMessage &msg)
{
	shvLogFuncFrame() << msg.toCpon();
	if(msg.isRequest()) {
		cp::RpcRequest rq(msg);
		cp::RpcResponse resp = cp::RpcResponse::forRequest(rq.metaData());
		try {
			//shvInfo() << "RPC request received:" << rq.toCpon();
			const cp::RpcValue shv_path = rq.shvPath();
			std::string path_rest;
			shv::iotqt::node::ShvNode *nd = m_shvTree->cd(shv_path.toString(), &path_rest);
			if(!nd)
				SHV_EXCEPTION("Path not found: " + shv_path.toString());
			rq.setShvPath(path_rest);
			shv::chainpack::RpcValue result = nd->processRpcRequest(rq);
			if(result.isValid())
				resp.setResult(result);
			else
				return;
		}
		catch (shv::core::Exception &e) {
			resp.setError(cp::RpcResponse::Error::create(cp::RpcResponse::Error::MethodCallException, e.message()));
		}
		if(resp.requestId().toInt() > 0) // RPC calls with requestID == 0 does not expect response
			m_rpcConnection->sendMessage(resp);
	}
	else if(msg.isResponse()) {
		cp::RpcResponse rsp(msg);
		//shvInfo() << "RPC response received:" << rsp.toCpon();
		if(rsp.requestId() == m_getStatusRpcId) {
			shvDebug() << "Get status response id:" << m_getStatusRpcId;
			if(rsp.isError()) {
				setPlcConnected(false);
			}
			else {
				setBfsStatus(rsp.result().toInt());
				setPlcConnected(true);
				m_getStatusRpcId = 0;
			}
		}
	}
	else if(msg.isNotify()) {
		cp::RpcNotify ntf(msg);
#ifdef TEST
		shvInfo() << "RPC notify received:" << ntf.toCpon();
#else
		if(ntf.method() == cp::Rpc::NTF_VAL_CHANGED) {
			if(ntf.shvPath() == "../bfs1/status") {
				setBfsStatus(ntf.params().toInt());
			}
		}
#endif
	}
}
/*
void ShvAgentApp::onRootNodeSendRpcMesage(const shv::chainpack::RpcMessage &msg)
{
	if(msg.isResponse()) {
		cp::RpcResponse resp(msg);
		if(resp.requestId().toUInt() == 0) // RPC calls with requestID == 0 does not expect response
			return;
		m_rpcConnection->sendMessage(resp);
		return;
	}
	shvError() << "Send message not implemented.";// << msg.toCpon();
}
*/

