#include "jn50viewapp.h"
#include "appclioptions.h"
#include "settings.h"

#include <shv/iotqt/rpc/deviceconnection.h>
#include <shv/iotqt/rpc/rpcresponsecallback.h>
#include <shv/iotqt/node/shvnodetree.h>
#include <shv/iotqt/node/localfsnode.h>
#include <shv/coreqt/log.h>
#include <shv/chainpack/metamethod.h>

#include <shv/core/stringview.h>
#include <shv/core/assert.h>

#include <QFileSystemWatcher>
#include <QSettings>
#include <QTimer>

#include <fstream>

namespace cp = shv::chainpack;

Jn50ViewApp::Jn50ViewApp(int &argc, char **argv, AppCliOptions* cli_opts)
	: Super(argc, argv)
	, m_cliOptions(cli_opts)
{
	loadSettings();

	m_rpcConnection = new shv::iotqt::rpc::DeviceConnection(this);

	m_rpcConnection->setCliOptions(cli_opts);

	connect(m_rpcConnection, &shv::iotqt::rpc::ClientConnection::brokerConnectedChanged, this, &Jn50ViewApp::onBrokerConnectedChanged);
	connect(m_rpcConnection, &shv::iotqt::rpc::ClientConnection::rpcMessageReceived, this, &Jn50ViewApp::onRpcMessageReceived);

	QTimer::singleShot(0, m_rpcConnection, &shv::iotqt::rpc::ClientConnection::open);
}

Jn50ViewApp::~Jn50ViewApp()
{
	shvInfo() << "destroying shv agent application";
}

Jn50ViewApp *Jn50ViewApp::instance()
{
	return qobject_cast<Jn50ViewApp *>(QCoreApplication::instance());
}

unsigned Jn50ViewApp::convStatus() const
{
	return m_deviceSnapshot.value("status").toUInt();
}

void Jn50ViewApp::setConvStatus(unsigned s)
{
	setShvDeviceValue("status", s);
}

void Jn50ViewApp::setShvDeviceConnected(bool on)
{
	bool old_val = isShvDeviceConnected();
	if(old_val == on)
		return;
	if(on) {
		//shvInfo() << "SHV device connected";
		/// todo load all the shv values
	}
	else {
		m_deviceSnapshot.clear();
		shvWarning() << "SHV device disconnected";
	}
	setShvDeviceValue("shvDeviceConnected", on);
	emit shvDeviceConnectedChanged(on);
}

bool Jn50ViewApp::isShvDeviceConnected() const
{
	return shvDeviceValue("shvDeviceConnected").toBool();
}

void Jn50ViewApp::loadSettings()
{
	QSettings qsettings;
	Settings settings(qsettings);
	AppCliOptions *cli_opts = cliOptions();
	if(!cli_opts->serverHost_isset())
		cli_opts->setServerHost(settings.shvBrokerHost().toStdString());
	if(!cli_opts->user_isset())
		cli_opts->setUser("jn50view");
	if(!cli_opts->serverPort_isset())
		cli_opts->setServerPort(settings.shvBrokerPort());
	if(!cli_opts->password_isset()) {
		cli_opts->setPassword("8884a26b82a69838092fd4fc824bbfde56719e02");
		cli_opts->setLoginType("SHA1");
	}
	if(!cli_opts->converterShvPath_isset())
		cli_opts->setConverterShvPath(settings.predatorShvPath().toStdString());
}

const std::string &Jn50ViewApp::logFilePath()
{
	static std::string log_file_path = QDir::tempPath().toStdString() + "/jn50view.log";
	return log_file_path;
}

void Jn50ViewApp::setShvDeviceValue(const std::string &path, const shv::chainpack::RpcValue &val)
{
	cp::RpcValue old_val = m_deviceSnapshot.value(path);
	shvDebug() << __FUNCTION__ << path << old_val.toCpon() << "-->" << val.toCpon() << "ne:" << (old_val != val);
	if(old_val != val) {
		m_deviceSnapshot[path] = val;
		emit shvDeviceValueChanged(path, val);
	}
}

shv::chainpack::RpcValue Jn50ViewApp::shvDeviceValue(const std::string &path) const
{
	return  m_deviceSnapshot.value(path);
}

void Jn50ViewApp::reloadShvDeviceValue(const std::string &path)
{
	if(path == "shvDeviceConnected")
		return;
	shvDebug() << "GET" << (cliOptions()->converterShvPath() + '/' + path);
	shv::iotqt::rpc::ClientConnection *conn = rpcConnection();
	int rq_id = conn->callShvMethod(cliOptions()->converterShvPath() + '/' + path, cp::Rpc::METH_GET);
	shv::iotqt::rpc::RpcResponseCallBack *cb = new shv::iotqt::rpc::RpcResponseCallBack(conn, rq_id, this);
	connect(cb, &shv::iotqt::rpc::RpcResponseCallBack::finished, this, [this, path](const cp::RpcResponse &resp) {
		if(resp.isValid()) {
			if(resp.isError())
				shvWarning() << "GET" << path << "RPC request error:" << resp.error().toString();
			else
				this->setShvDeviceValue(path, resp.result());
		}
		else {
			shvWarning() << "RPC request timeout";
		}
	});
}

static constexpr int PLC_CONNECTED_TIMOUT_MSEC = 10*1000;

void Jn50ViewApp::sendGetStatusRequest()
{
	auto *conn = rpcConnection();
	if(conn->isBrokerConnected()) {
		m_getStatusRpcId = conn->callShvMethod(cliOptions()->converterShvPath() + "/status", cp::Rpc::METH_GET);
		shvDebug() << "Sending get status request id:" << m_getStatusRpcId;
	}
}

void Jn50ViewApp::checkShvDeviceConnected()
{
	shvLogFuncFrame() << (m_getStatusRpcId == 0);
	setShvDeviceConnected(m_getStatusRpcId == 0);
	sendGetStatusRequest();
}

void Jn50ViewApp::onBrokerConnectedChanged(bool is_connected)
{
	if(is_connected) {
		rpcConnection()->callMethodSubscribe(cliOptions()->converterShvPath(), cp::Rpc::SIG_VAL_CHANGED);
		sendGetStatusRequest();

		if(!m_shvDeviceConnectedCheckTimer) {
			m_shvDeviceConnectedCheckTimer = new QTimer(this);
			m_shvDeviceConnectedCheckTimer->setInterval(PLC_CONNECTED_TIMOUT_MSEC);
			connect(m_shvDeviceConnectedCheckTimer, &QTimer::timeout, this, &Jn50ViewApp::checkShvDeviceConnected);
		}
		m_shvDeviceConnectedCheckTimer->start();
	}
	else {
		if(m_shvDeviceConnectedCheckTimer)
			m_shvDeviceConnectedCheckTimer->stop();

	}
}

void Jn50ViewApp::onRpcMessageReceived(const shv::chainpack::RpcMessage &msg)
{
	shvLogFuncFrame() << msg.toCpon();
	if(msg.isRequest()) {
		cp::RpcRequest rq(msg);
		cp::RpcResponse resp = cp::RpcResponse::forRequest(rq.metaData());
		try {
			//shvInfo() << "RPC request received:" << rq.toCpon();
			SHV_EXCEPTION("unexpected SHV request!");
		}
		catch (shv::core::Exception &e) {
			resp.setError(cp::RpcResponse::Error::create(cp::RpcResponse::Error::MethodCallException, e.message()));
		}
		if(resp.requestId().toInt() > 0) // RPC calls with requestID == 0 does not expect response
			m_rpcConnection->sendMessage(resp);
	}
	else if(msg.isResponse()) {
		cp::RpcResponse rsp(msg);
		if(rsp.isError())
			shvError() << "RPC error response received:" << rsp.toCpon();

		if(rsp.requestId() == m_getStatusRpcId) {
			shvDebug() << "Get status response id:" << m_getStatusRpcId;
			if(rsp.isError()) {
				setShvDeviceConnected(false);
			}
			else {
				setShvDeviceValue("status", rsp.result());
				setShvDeviceConnected(true);
				m_getStatusRpcId = 0;
			}
		}
	}
	else if(msg.isSignal()) {
		cp::RpcSignal ntf(msg);
		shvDebug() << "RPC notify received:" << ntf.toCpon();
		if(ntf.method() == cp::Rpc::SIG_VAL_CHANGED) {
			const shv::chainpack::RpcValue::String shv_path = ntf.shvPath().toString();
			std::string base_path = cliOptions()->converterShvPath() + '/';
			if(shv::core::String::startsWith(shv_path, base_path)) {
				std::string path = shv_path.substr(base_path.size());
				cp::RpcValue new_val = ntf.params();
				setShvDeviceValue(path, new_val);
			}
		}
	}
}

