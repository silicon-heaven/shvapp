#include "revitestapp.h"
#include "appclioptions.h"
#include "revitestdevice.h"

#include <shv/iotqt/rpc/deviceconnection.h>
#include <shv/iotqt/rpc/rpcresponsecallback.h>
#include <shv/iotqt/utils/fileshvjournal.h>
#include <shv/iotqt/node/shvnodetree.h>
#include <shv/core/log.h>
#include <shv/chainpack/rpcdriver.h>

#include <QFile>
#include <QTimer>

#define logTestCallsD() nCDebug("TestCalls")
#define logTestCallsI() nCInfo("TestCalls")
#define logTestCallsW() nCWarning("TestCalls")
#define logTestCallsE() nCError("TestCalls")
#define logTestPassed(test_no) nCError("TestCalls").color(NecroLog::Color::LightGreen) << "#" << test_no << "[PASSED]"
#define logTestFailed(test_no) nCError("TestCalls").color(NecroLog::Color::LightRed) << "#" << test_no << "[FAILED]"

namespace cp = shv::chainpack;
namespace iot = shv::iotqt;

RevitestApp::RevitestApp(int &argc, char **argv, AppCliOptions* cli_opts)
	: Super(argc, argv)
	, m_cliOptions(cli_opts)
{
	m_shvJournal = new shv::iotqt::utils::FileShvJournal([this](std::vector<shv::iotqt::utils::ShvJournalEntry> &s) { this->getSnapshot(s); });
	if(cli_opts->shvJournalDir_isset())
		m_shvJournal->setJournalDir(cli_opts->shvJournalDir());
	m_shvJournal->setFileSizeLimit(cli_opts->shvJournalFileSizeLimit());
	m_shvJournal->setJournalSizeLimit(cli_opts->shvJournalSizeLimit());
	{
		cp::RpcValue::Map types {
			{"Status", cp::RpcValue::Map{
					{"type", "BitField"},
					{"fields", cp::RpcValue::List{
							cp::RpcValue::Map{{"name", "PosOff"}, {"value", 0}},
							cp::RpcValue::Map{{"name", "PosOn"}, {"value", 1}},
							cp::RpcValue::Map{{"name", "PosMiddle"}, {"value", 2}},
							cp::RpcValue::Map{{"name", "PosError"}, {"value", 3}},
							cp::RpcValue::Map{{"name", "BatteryLow"}, {"value", 4}},
							cp::RpcValue::Map{{"name", "BatteryHigh"}, {"value", 5}},
							cp::RpcValue::Map{{"name", "DoorOpenCabinet"}, {"value", 6}},
							cp::RpcValue::Map{{"name", "DoorOpenMotor"}, {"value", 7}},
							cp::RpcValue::Map{{"name", "ModeAuto"}, {"value", 8}},
							cp::RpcValue::Map{{"name", "ModeRemote"}, {"value", 9}},
							cp::RpcValue::Map{{"name", "ModeService"}, {"value", 10}},
							cp::RpcValue::Map{{"name", "MainSwitch"}, {"value", 11}},
						}
					},
					{"description", "PosOff = 0, PosOn = 1, PosMiddle = 2, PosError= 3, BatteryLow = 4, BatteryHigh = 5, DoorOpenCabinet = 6, DoorOpenMotor = 7, ModeAuto= 8, ModeRemote = 9, ModeService = 10, MainSwitch = 11, ErrorRtc = 12"},
				}
			},
		};
		cp::RpcValue::Map paths {
			{"status", cp::RpcValue::Map{ {"type", "Status"} }
			},
		};
		cp::RpcValue::Map type_info {
			{"types", types},
			{"paths", paths},
		};
		m_shvJournal->setTypeInfo(type_info);
	}

	m_rpcConnection = new shv::iotqt::rpc::DeviceConnection(this);

	if(!cli_opts->user_isset())
		cli_opts->setUser("iot");
	if(!cli_opts->password_isset())
		cli_opts->setPassword("lub42DUB");
	if(!cli_opts->mountPoint_isset())
		cli_opts->setMountPoint("/test/lublin/odpojovace");
	m_rpcConnection->setCliOptions(cli_opts);

	m_revitest = new RevitestDevice(this);
	connect(m_rpcConnection, &shv::iotqt::rpc::ClientConnection::rpcMessageReceived, m_revitest, &RevitestDevice::onRpcMessageReceived);
	connect(m_rpcConnection, &shv::iotqt::rpc::ClientConnection::brokerConnectedChanged, this, &RevitestApp::onBrokerConnectedChanged);
	connect(m_revitest, &RevitestDevice::sendRpcMessage, m_rpcConnection, &shv::iotqt::rpc::ClientConnection::sendMessage);
	connect(m_revitest->shvTree()->root(), &shv::iotqt::node::ShvRootNode::sendRpcMesage, m_rpcConnection, &shv::iotqt::rpc::ClientConnection::sendMessage);

	m_rpcConnection->open();
}

RevitestApp::~RevitestApp()
{
	shvInfo() << "destroying shv agent application";
	delete m_shvJournal;
}

RevitestApp *RevitestApp::instance()
{
	return qobject_cast<RevitestApp *>(QCoreApplication::instance());
}

void RevitestApp::onBrokerConnectedChanged(bool is_connected)
{
	if(is_connected) {
		std::string cpon;
		if(cliOptions()->callFile_isset()) {
			QFile f(cliOptions()->callFile().data());
			if(!f.open(QFile::ReadOnly))
				SHV_EXCEPTION("Cannot oprn file " + f.fileName().toStdString() + " for reading");
			QByteArray ba = f.readAll();
			cpon = std::string(ba.data(), ba.length());
		}
		else if(cliOptions()->callFile_isset()) {
			cpon = cliOptions()->callMethods();
		}
		if(!cpon.empty()) {
			std::string err;
			cp::RpcValue rv = cp::RpcValue::fromCpon(cpon, &err);
			if(rv.isMap())
				m_shvCalls.push_back(rv);
			else
				m_shvCalls = rv.toList();
			processShvCalls();
		}
	}
}

void RevitestApp::getSnapshot(std::vector<shv::iotqt::utils::ShvJournalEntry> &snapshot)
{
	m_revitest->getSnapshot(snapshot);
}

void RevitestApp::processShvCalls()
{
	if(!m_rpcConnection->isBrokerConnected()) {
		m_shvCalls.clear();
		return;
	}
	if(m_shvCalls.empty())
		return;

	cp::RpcValue rv = m_shvCalls.front();
	m_shvCalls.erase(m_shvCalls.begin());
	const shv::chainpack::RpcValue::Map &task = rv.toMap();
	std::string descr = task.value("descr").toString();
	std::string cmd = task.value("cmd").toString();
	bool process_next_on_exec = task.value("processNextOnExec", false).toBool();
	bool process_next_on_success = task.value("processNextOnSuccess", true).toBool();
	static int s_test_no = 0;
	int test_no = ++s_test_no;
	logTestCallsI() << "=========================================";
	logTestCallsI() << "#" << test_no << "COMMAND:" << cmd;
	logTestCallsI() << "DESCR:" << descr;
	if(cmd == "call") {
		std::string shv_path = task.value("shvPath").toString();
		std::string method = task.value("method").toString();
		cp::RpcValue params = task.value("params");
		cp::RpcValue result = task.value("result");

		int rq_id = m_rpcConnection->nextRequestId();
		logTestCallsD() << "CALL rqid:" << rq_id << "msg:" << rv.toCpon();
		shv::iotqt::rpc::RpcResponseCallBack *cb = new shv::iotqt::rpc::RpcResponseCallBack(m_rpcConnection, rq_id, this);
		cb->start(this, [this, rq_id, result, process_next_on_success, test_no](const cp::RpcResponse &resp) {
			bool ok = false;
			if(resp.isValid()) {
				if(resp.isError()) {
					logTestCallsW() << "ERROR rqid:" << rq_id << "request error:" << resp.error().toString();
				}
				else {
					logTestCallsD() << "RESULT rqid:" << rq_id << "result:" << resp.toCpon();
					if(resp.result() == result) {
						logTestPassed(test_no);
						ok = true;
					}
					else {
						logTestFailed(test_no) << "wrong result" << resp.result().toCpon();
					}
				}
			}
			else {
				logTestCallsW() << "TIMEOUT rqid:" << rq_id;
			}
			if(ok && process_next_on_success)
				this->processShvCalls();
		});
		m_rpcConnection->callShvMethod(rq_id, shv_path, method, params);
		if(process_next_on_exec)
			this->processShvCalls();
	}
	else if(cmd == "sigTrap") {
		std::string shv_path = task.value("shvPath").toString();
		std::string method = task.value("method").toString();
		cp::RpcValue params = task.value("params");
		int timeout = task.value("timeout").toInt();
		QObject *ctx = new QObject();
		QTimer::singleShot(timeout, ctx, [ctx, timeout, test_no]() {
			logTestFailed(test_no) << "timeout after:" << timeout;
			ctx->deleteLater();
		});
		connect(m_rpcConnection
				, &shv::iotqt::rpc::DeviceConnection::rpcMessageReceived
				, ctx
				, [ctx, this, shv_path, method, params, process_next_on_success, test_no](const shv::chainpack::RpcMessage &msg) {
			logTestCallsD() << "MSG:" << msg.toCpon();
			if(msg.isSignal()) {
				cp::RpcSignal sig(msg);
				if(sig.shvPath() == shv_path && sig.method() == method && sig.params() == params) {
					logTestPassed(test_no);
					ctx->deleteLater();
					if(process_next_on_success)
						this->processShvCalls();
				}
			}
		});
		if(process_next_on_exec)
			this->processShvCalls();
	}
}

