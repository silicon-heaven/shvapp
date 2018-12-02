#include "revitestapp.h"
#include "appclioptions.h"
#include "revitestdevice.h"

#include <shv/iotqt/rpc/deviceconnection.h>
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
	m_shvJournal->setDirSizeLimit(cli_opts->shvJournalDirSizeLimit());
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

void RevitestApp::getSnapshot(std::vector<shv::iotqt::utils::ShvJournalEntry> &snapshot)
{
	m_revitest->getSnapshot(snapshot);
}

