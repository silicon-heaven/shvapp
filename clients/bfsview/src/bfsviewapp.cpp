#include "bfsviewapp.h"
#include "appclioptions.h"
#include "settings.h"

#include <shv/iotqt/rpc/deviceconnection.h>
#include <shv/iotqt/node/shvnodetree.h>
#include <shv/iotqt/node/localfsnode.h>
#include <shv/core/utils/shvpath.h>
#include <shv/coreqt/log.h>
#include <shv/chainpack/metamethod.h>

#include <shv/core/stringview.h>
#include <shv/core/assert.h>

#include <QFileSystemWatcher>
#include <QSettings>
#include <QTimer>

#include <fstream>

namespace cp = shv::chainpack;

static const char BFS1_PWR_STATUS[] = "bfs1/pwrStatus";

static const char METH_SIM_SET[] = "sim_set";
static const char METH_APP_LOG[] = "appLog";

static std::vector<cp::MetaMethod> meta_methods_root {
	cp::methods::DIR,
	cp::methods::LS,
	{cp::Rpc::METH_DEVICE_ID, cp::MetaMethod::Flag::IsGetter, {}, "RpcValue"},
	{cp::Rpc::METH_DEVICE_TYPE, cp::MetaMethod::Flag::None, {}, "RpcValue"},
	{cp::Rpc::METH_MOUNT_POINT, cp::MetaMethod::Flag::None, {}, "RpcValue"},
	{cp::Rpc::METH_APP_NAME, cp::MetaMethod::Flag::None, {}, "RpcValue"},
	{METH_APP_LOG, cp::MetaMethod::Flag::None, {}, "RpcValue"},
};

size_t AppRootNode::methodCount(const StringViewList &shv_path)
{
	if(shv_path.empty())
		return meta_methods_root.size();
	return 0;
}

const shv::chainpack::MetaMethod *AppRootNode::metaMethod(const StringViewList &shv_path, size_t ix)
{
	if(!shv_path.empty())
		return nullptr;
	if(meta_methods_root.size() <= ix)
		SHV_EXCEPTION("Invalid method index: " + std::to_string(ix) + " of: " + std::to_string(meta_methods_root.size()));
	return &(meta_methods_root[ix]);
}

shv::chainpack::RpcValue AppRootNode::callMethod(const StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params, const shv::chainpack::RpcValue &user_id)
{
	if(shv_path.empty()) {
		if(method == cp::Rpc::METH_APP_NAME) {
			return QCoreApplication::instance()->applicationName().toStdString();
		}
		if(method == cp::Rpc::METH_DEVICE_ID) {
			return BfsViewApp::instance()->cliOptions()->deviceId();
		}
		if(method == cp::Rpc::METH_DEVICE_TYPE) {
			return "BfsView";
		}
		if(method == METH_APP_LOG) {
			// read entire file into string
			std::ifstream is{BfsViewApp::logFilePath(), std::ios::binary | std::ios::ate};
			if(is) {
				auto size = is.tellg();
				std::string str(size, '\0'); // construct string to stream size
				is.seekg(0);
				is.read(&str[0], size);
				return str;
			}
			return std::string();
		}
	}
	return Super::callMethod(shv_path, method, params, user_id);
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
	cp::methods::DIR,
	cp::methods::LS,
	{cp::Rpc::METH_GET, cp::MetaMethod::Flag::IsGetter, {}, "UInt", cp::MetaMethod::AccessLevel::Browse, {{"chng"}}},
	{cp::Rpc::METH_SET, cp::MetaMethod::Flag::None, "UInt", "Bool", cp::MetaMethod::AccessLevel::Write},
	{METH_SIM_SET, cp::MetaMethod::Flag::None, "RpcValue", {}},
};

PwrStatusNode::PwrStatusNode(shv::iotqt::node::ShvNode *parent)
 : Super(parent)
{
	m_sendPwrStatusDeferredTimer = new QTimer(this);
	m_sendPwrStatusDeferredTimer->setSingleShot(true);
	m_sendPwrStatusDeferredTimer->setInterval(50);
	connect(m_sendPwrStatusDeferredTimer, &QTimer::timeout, this, &PwrStatusNode::sendPwrStatusChangedDeferred);
}

size_t PwrStatusNode::methodCount(const StringViewList &shv_path)
{
	if(!shv_path.empty())
		return 0;
	if(BfsViewApp::instance()->cliOptions()->isPwrStatusSimulate()) {
		//shvInfo() << "PwrStatusNode::methodCount sim:" << (meta_methods_pwrstatus.size() - 1);
		return meta_methods_pwrstatus.size();
	}
	else {
		//shvInfo() << "PwrStatusNode::methodCount:" << meta_methods_pwrstatus.size();
		return meta_methods_pwrstatus.size() - 1;
	}
}

const shv::chainpack::MetaMethod *PwrStatusNode::metaMethod(const StringViewList &shv_path, size_t ix)
{
	if(!shv_path.empty())
		return nullptr;
	if(meta_methods_pwrstatus.size() <= ix)
		SHV_EXCEPTION("Invalid method index: " + std::to_string(ix) + " of: " + std::to_string(meta_methods_pwrstatus.size()));
	return &(meta_methods_pwrstatus[ix]);
}

shv::chainpack::RpcValue PwrStatusNode::callMethod(const StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params, const shv::chainpack::RpcValue &user_id)
{
	if(shv_path.empty()) {
		if(method == cp::Rpc::METH_GET) {
			return static_cast<unsigned>(pwrStatus());
		}
		if(method == METH_SIM_SET) {
			unsigned s = params.toUInt();
			BfsViewApp::instance()->setPwrStatus(static_cast<PwrStatus>(s));
			return true;
		}
	}
	return Super::callMethod(shv_path, method, params, user_id);
}

void PwrStatusNode::setPwrStatus(PwrStatus s)
{
	//shvInfo() << "set pwr status to:" << (int)s;
	if(s == m_pwrStatus)
		return;
	m_pwrStatus = s;
	sendPwrStatusChanged();
}

void PwrStatusNode::sendPwrStatusChanged()
{
	// there can be status UNKNOWN drops in pwrStatus
	// due to race condition in pwr status file reading
	// send pwrStatus only if it will not changer for 50 msec
	// to address this problem
	m_pwrStatusToSendDeferred = m_pwrStatus;
	m_sendPwrStatusDeferredTimer->start();
}

void PwrStatusNode::sendPwrStatusChangedDeferred()
{
#ifndef TEST
	if(!BfsViewApp::instance()->rpcConnection()->isBrokerConnected())
		return;
	cp::RpcSignal ntf;
	ntf.setMethod(cp::Rpc::SIG_VAL_CHANGED);
	ntf.setParams(static_cast<unsigned>(m_pwrStatusToSendDeferred));
	ntf.setShvPath(BFS1_PWR_STATUS);
	rootNode()->emitSendRpcMessage(ntf);
#endif
}

BfsViewApp::BfsViewApp(int &argc, char **argv, AppCliOptions* cli_opts)
	: Super(argc, argv)
	, m_cliOptions(cli_opts)
{
	//cp::RpcMessage::setMetaTypeExplicit(cli_opts->isMetaTypeExplicit());

	m_rpcConnection = new shv::iotqt::rpc::DeviceConnection(this);

	QSettings qsettings;
	Settings settings(qsettings);

	if(!cli_opts->serverHost_isset())
		cli_opts->setServerHost(settings.shvBrokerUrl().toStdString());
	if(!cli_opts->user_isset())
		cli_opts->setUser("bfsview");
	//if(!cli_opts->serverPort_isset())
	//	cli_opts->setServerPort(settings.shvBrokerPort());
	if(!cli_opts->password_isset()) {
		cli_opts->setPassword("8884a26b82a69838092fd4fc824bbfde56719e02");
		cli_opts->setLoginType("SHA1");
	}

	if(!cli_opts->deviceId_isset())
		cli_opts->setDeviceId("bfsview-001");
	m_rpcConnection->setCliOptions(cli_opts);

	connect(m_rpcConnection, &shv::iotqt::rpc::ClientConnection::brokerConnectedChanged, this, &BfsViewApp::onBrokerConnectedChanged);
	connect(m_rpcConnection, &shv::iotqt::rpc::ClientConnection::rpcMessageReceived, this, &BfsViewApp::onRpcMessageReceived);

	connect(this, &BfsViewApp::bfsStatusChanged, this, &BfsViewApp::onBfsStatusChanged);

	AppRootNode *root = new AppRootNode();
	m_shvTree = new shv::iotqt::node::ShvNodeTree(root, this);
	m_pwrStatusNode = new PwrStatusNode();
	m_shvTree->mount(BFS1_PWR_STATUS, m_pwrStatusNode);
	connect(m_shvTree->root(), &shv::iotqt::node::ShvRootNode::sendRpcMessage, m_rpcConnection, &shv::iotqt::rpc::DeviceConnection::sendRpcMessage);

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
	QTimer::singleShot(0, m_rpcConnection, &shv::iotqt::rpc::ClientConnection::open);
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
	shvInfo() << "powerFileName:" << m_powerFileName;

	int inter = settings.powerFileCheckInterval();
	if(inter > 0) {
		if(!m_powerFileCheckTimer) {
			m_powerFileCheckTimer = new QTimer(this);
			connect(m_powerFileCheckTimer, &QTimer::timeout, this, &BfsViewApp::checkPowerSwitchStatusFile);
		}
		shvInfo() << "Starting powerFileCheckTimer, interval:" << inter << "sec";
		m_powerFileCheckTimer->start(inter * 1000);
	}
	else if(m_powerFileCheckTimer) {
		shvInfo() << "Stopping powerFileCheckTimer";
		m_powerFileCheckTimer->stop();
	}

	if(m_powerFileWatcher)
		delete m_powerFileWatcher;
	m_powerFileWatcher = new QFileSystemWatcher(this);
	connect(m_powerFileWatcher, &QFileSystemWatcher::fileChanged, this, &BfsViewApp::checkPowerSwitchStatusFile);
	m_powerFileWatcher->addPath(m_powerFileName);

	checkPowerSwitchStatusFile();
}

void BfsViewApp::checkPowerSwitchStatusFile()
{
	if(cliOptions()->isPwrStatusSimulate())
		return;
	shvDebug() << "Checking pwr status file:" << m_powerFileName;
	QFile file(m_powerFileName);
	if (!file.open(QIODevice::ReadOnly)) {
		shvError() << "Cannot open file:" << m_powerFileName << "for reading!";
		setPwrStatus(PwrStatus::Unknown);
		return;
	}
	QDateTime curr_ts = QDateTime::currentDateTimeUtc();
	PwrStatus overall_pwr_status = PwrStatus::Unknown;
	int number_of_valid_lines = 0;
	while(!file.atEnd()) {
		QByteArray ba = file.readLine().trimmed();
		if(ba.isEmpty())
			continue;

		QString line = QString::fromUtf8(ba);
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
		auto skip_empty_parts = QString::SkipEmptyParts;
#else
		auto skip_empty_parts = Qt::SkipEmptyParts;
#endif
		QStringList fields = line.split(' ', skip_empty_parts);
		shvDebug() << line;
		QString name = fields.value(0);
		bool ok;
		int pwr_val = fields.value(1).toInt(&ok);
		QString ts_str = fields.value(2);
		if(fields.length() != 3) {
			shvWarning() << "possible race condition, invalid pwr file line format:" << line;
			overall_pwr_status = PwrStatus::Unknown;
			break;
		}

		PwrStatus pwr_status = ok? ((pwr_val == 1)? PwrStatus::On: PwrStatus::Off): PwrStatus::Unknown;
		if(pwr_status == PwrStatus::Unknown) {
			// ignore 'X' status
			// line like: Bory-40 X 2018-6-4T12:01:12
			continue;
		}
		number_of_valid_lines++;
		TS &ts = m_powerSwitchStatus[name];
		//shvDebug() << name << pwr_status << ts_str << "vs" << ts.timeStampString << "curr:" << curr_ts.toString(Qt::ISODateWithMs);
		if(ts.timeStampString != ts_str) {
			ts.when = QDateTime::currentDateTimeUtc();
			ts.timeStampString = ts_str;
		}
		SHV_ASSERT(ts.when.isValid(), "when should be valid", continue);
		int status_age = static_cast<int>(ts.when.secsTo(curr_ts));
		static constexpr int LIMIT_SEC = 45;
		if(status_age < LIMIT_SEC) {
			// Andrejsek generuje soubor kazdych 30 sekund, 45 je s rezervou
			if((pwr_status == PwrStatus::On)
					|| (pwr_status == PwrStatus::Off && overall_pwr_status == PwrStatus::Unknown))
				overall_pwr_status = pwr_status;
		}
		else {
			shvWarning() << "line not updated for more than" << LIMIT_SEC << "sec, we cannot deduce pwr status:" << line;
			overall_pwr_status = PwrStatus::Unknown;
			break;
		}
		//shvDebug() << "ON:" << name << ts_str << ts.when.toUTC().toString(Qt::ISODateWithMs);
	}
	if(number_of_valid_lines == 0)
		shvWarning() << "File:" << file.fileName() << "does not contain any valid line, we cannot deduce pwr status, setting to UNKNOWN.";
	setPwrStatus(overall_pwr_status);
}

static constexpr int PLC_CONNECTED_TIMOUT_MSEC = 10*1000;

void BfsViewApp::sendGetStatusRequest()
{
	auto *conn = rpcConnection();
	if(conn->isBrokerConnected()) {
		m_getStatusRpcId = conn->callShvMethod(bfsPlcShvPathForStatus(), cp::Rpc::METH_GET);
		shvDebug() << "Sending get status request id:" << m_getStatusRpcId;
	}
}

const std::string &BfsViewApp::bfsPlcShvPath()
{
	static std::string shv_path;
	if(shv_path.empty()) {
		QSettings qsettings;
		Settings settings(qsettings);
		shv_path = settings.bfsShvPath().toStdString();
	}
	return shv_path;
}

std::string BfsViewApp::bfsPlcShvPathForStatus()
{
	std::string shv_path = shv::core::utils::ShvPath(bfsPlcShvPath()).appendDir(std::string("status"));
	return shv_path;
}

void BfsViewApp::checkPlcConnected()
{
	shvLogFuncFrame() << (m_getStatusRpcId == 0);
	setPlcConnected(m_getStatusRpcId == 0);
	sendGetStatusRequest();
}

void BfsViewApp::setPwrStatus(PwrStatus u)
{
	if(pwrStatus() == u)
		return;
	shvInfo() << "PWR status changed to:" << static_cast<unsigned>(u) << switchStatusToString(u);
	m_pwrStatusNode->setPwrStatus(u);
	emit pwrStatusChanged(u);
}

BfsViewApp::PwrStatus BfsViewApp::pwrStatus()
{
	return m_pwrStatusNode->pwrStatus();
}

void BfsViewApp::setOmpag(bool val)
{
#ifdef TESTING
	int s = bfsStatus();
	setBit(s, BfsStatus::OmpagOn, val);
	setBit(s, BfsStatus::OmpagOff, !val);
	setBfsStatus(s);
#else
	if(rpcConnection()->isBrokerConnected()) {
		setOmpagRequiredSwitchStatus(val? SwitchStatus::On: SwitchStatus::Off);
		rpcConnection()->callShvMethod(bfsPlcShvPath(), "setOmpag", val);
	}
#endif
}

void BfsViewApp::setConv(bool val)
{
#ifdef TESTING
	int s = bfsStatus();
	setBit(s, BfsViewApp::BfsStatus::MswOn, val);
	setBit(s, BfsViewApp::BfsStatus::MswOff, !val);
	setBfsStatus(s);
#else
	if(rpcConnection()->isBrokerConnected()) {
		setConvRequiredSwitchStatus(val? SwitchStatus::On: SwitchStatus::Off);
		rpcConnection()->callShvMethod(bfsPlcShvPath(), "setConv", val);
	}
#endif
}

BfsViewApp::SwitchStatus BfsViewApp::ompagSwitchStatus()
{
	unsigned ps = bfsStatus();
	int status = (ps & ((1 << static_cast<int>(BfsStatus::OmpagOn)) | (1 << static_cast<int>(BfsStatus::OmpagOff)))) >> static_cast<int>(BfsStatus::OmpagOn);
	return static_cast<SwitchStatus>(status);
}

BfsViewApp::SwitchStatus BfsViewApp::convSwitchStatus()
{
	unsigned ps = bfsStatus();
	int status = (ps & ((1 << static_cast<int>(BfsStatus::MswOn)) | (1 << static_cast<int>(BfsStatus::MswOff)))) >> static_cast<int>(BfsStatus::MswOn);
	return static_cast<SwitchStatus>(status);
}

const std::string &BfsViewApp::logFilePath()
{
	static std::string log_file_path = QDir::tempPath().toStdString() + "/bfsview.log";
	return log_file_path;
}

QString BfsViewApp::switchStatusToString(BfsViewApp::SwitchStatus status)
{
	switch (status) {
	case BfsViewApp::SwitchStatus::On: return QStringLiteral("On");
	case BfsViewApp::SwitchStatus::Off: return QStringLiteral("Off");
	default: return QStringLiteral("Unknown");
	}
}

void BfsViewApp::onBrokerConnectedChanged(bool is_connected)
{
	if(is_connected) {
		rpcConnection()->callMethodSubscribe(bfsPlcShvPathForStatus(), cp::Rpc::SIG_VAL_CHANGED);
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
		if(!m_plcConnectedCheckTimer) {
			m_plcConnectedCheckTimer = new QTimer(this);
			m_plcConnectedCheckTimer->setInterval(PLC_CONNECTED_TIMOUT_MSEC);
			connect(m_plcConnectedCheckTimer, &QTimer::timeout, this, &BfsViewApp::checkPlcConnected);
		}
		m_plcConnectedCheckTimer->start();
	}
	else {
		if(m_plcConnectedCheckTimer)
			m_plcConnectedCheckTimer->stop();

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
			shv::iotqt::node::ShvNode *nd = m_shvTree->cd(shv_path.asString(), &path_rest);
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
			m_rpcConnection->sendRpcMessage(resp);
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
				shvDebug() << "\tBfs status:" << bfsStatus();
				setPlcConnected(true);
				m_getStatusRpcId = 0;
			}
		}
	}
	else if(msg.isSignal()) {
		cp::RpcSignal ntf(msg);
#ifdef TESTING
		shvInfo() << "RPC notify received:" << ntf.toCpon();
#else
		if(ntf.method() == cp::Rpc::SIG_VAL_CHANGED) {
			if(ntf.shvPath() == bfsPlcShvPathForStatus()) {
				setBfsStatus(ntf.params().toInt());
			}
		}
#endif
	}
}

void BfsViewApp::onBfsStatusChanged(int )
{
	if(ompagRequiredSwitchStatus() == ompagSwitchStatus())
		setOmpagRequiredSwitchStatus(SwitchStatus::Unknown);
	if(convRequiredSwitchStatus() == convSwitchStatus())
		setConvRequiredSwitchStatus(SwitchStatus::Unknown);
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

