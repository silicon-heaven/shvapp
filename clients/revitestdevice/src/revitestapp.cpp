#include "revitestapp.h"
#include "appclioptions.h"
#include "revitestdevice.h"

#include <shv/iotqt/rpc/deviceconnection.h>
#include <shv/iotqt/rpc/rpcresponsecallback.h>
#include <shv/iotqt/utils/fileshvjournal.h>
#include <shv/iotqt/node/shvnodetree.h>
#include <shv/core/log.h>
#include <shv/chainpack/rpcdriver.h>

namespace cp = shv::chainpack;
namespace iot = shv::iotqt;

RevitestApp::RevitestApp(int &argc, char **argv, AppCliOptions* cli_opts)
	: Super(argc, argv)
	, m_cliOptions(cli_opts)
{
	//cp::RpcMessage::setMetaTypeExplicit(cli_opts->isMetaTypeExplicit());

	m_shvJournal = new shv::iotqt::utils::FileShvJournal([this](std::vector<shv::iotqt::utils::ShvJournalEntry> &s) { this->getSnapshot(s); });
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
		std::string err;
		cp::RpcValue rv = cp::RpcValue::fromCpon(cliOptions()->callMethods(), &err);
		if(rv.isList()) {
			if(rv.toList().value(0).isList())
				m_shvCalls = rv.toList();
			else
				m_shvCalls.push_back(rv.toList());
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
	const shv::chainpack::RpcValue::List &lst = rv.toList();
	std::string shv_path = lst.value(0).toString();
	std::string method = lst.value(1).toString();
	cp::RpcValue params = lst.value(2);

	int rq_id = m_rpcConnection->callShvMethod(shv_path, method, params);
	shvInfo() << "CALL rqid:" << rq_id << "method:" << rv.toCpon();
	shv::iotqt::rpc::RpcResponseCallBack *cb = new shv::iotqt::rpc::RpcResponseCallBack(m_rpcConnection, rq_id, this);
	connect(cb, &shv::iotqt::rpc::RpcResponseCallBack::finished, this, [this, rq_id](const cp::RpcResponse &resp) {
		if(resp.isValid()) {
			if(resp.isError())
				shvWarning() << "ERROR rqid:" << rq_id << "request error:" << resp.error().toString();
			else
				shvInfo() << "RESULT rqid:" << rq_id << "result:" << resp.toCpon();
		}
		else {
			shvWarning() << "TIMEOUT rqid:" << rq_id;
		}
		this->processShvCalls();
	});
}

