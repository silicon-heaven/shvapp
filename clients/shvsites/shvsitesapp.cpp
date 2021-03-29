#include "shvsitesapp.h"

#include "appclioptions.h"
#include "sitesmodel.h"

#include <shv/iotqt/rpc/clientconnection.h>
#include <shv/iotqt/rpc/rpcresponsecallback.h>
#include <shv/coreqt/log.h>
#include <shv/core/string.h>

namespace cp = shv::chainpack;
namespace iot = shv::iotqt;

ShvSitesApp::ShvSitesApp(int &argc, char **argv, AppCliOptions* cli_opts)
	: Super(argc, argv)
	, m_cliOptions(cli_opts)
{
	m_sitesModel = new SitesModel(this);

	m_rpcConnection = new shv::iotqt::rpc::ClientConnection(this);
	m_rpcConnection->setCliOptions(cli_opts);
	connect(m_rpcConnection, &shv::iotqt::rpc::ClientConnection::rpcMessageReceived, this, &ShvSitesApp::onRpcMessageReceived);
	connect(m_rpcConnection, &shv::iotqt::rpc::ClientConnection::brokerConnectedChanged, this, &ShvSitesApp::onBrokerConnected);

	m_rpcConnection->open();
}

ShvSitesApp::~ShvSitesApp()
{
	shvInfo() << "destroying shvsites application";
}

void ShvSitesApp::onRpcMessageReceived(const shv::chainpack::RpcMessage &msg)
{
	logRpcMsg() << cp::RpcDriver::RCV_LOG_ARROW << msg.toPrettyString();
	if(msg.isRequest()) {
		cp::RpcRequest rq(msg);
		shvDebug() << "RPC request received:" << rq.toPrettyString();
	}
	else if(msg.isResponse()) {
		cp::RpcResponse rp(msg);
		shvDebug() << "RPC response received:" << rp.toPrettyString();
	}
	else if(msg.isSignal()) {
		cp::RpcSignal sig(msg);
		shvDebug() << "RPC notify received:" << sig.toPrettyString();
		if(sig.method() == cp::Rpc::SIG_MOUNTED_CHANGED) {
			shvInfo() << "mount changed on" << sig.shvPath().toString() << sig.params().toBool();
			sitesModel()->setSiteBrokerConnected(sig.shvPath().asString(), sig.params().toBool());
		}
	}
}

void ShvSitesApp::onBrokerConnected(bool is_connected)
{
	sitesModel()->clear();
	if(is_connected) {
		shv::iotqt::rpc::ClientConnection *conn = rpcConnection();
		conn->callMethodSubscribe("shv", cp::Rpc::SIG_MOUNTED_CHANGED);
		loadSitesModel();
	}
}

void ShvSitesApp::loadSitesModel()
{
	shv::iotqt::rpc::ClientConnection *conn = rpcConnection();
	int rqid = conn->callShvMethod("/sites", "getSites");
	iot::rpc::RpcResponseCallBack *cb = new iot::rpc::RpcResponseCallBack(conn, rqid, this);
	cb->start([this](const shv::chainpack::RpcResponse &resp) {
		shvDebug() << "Received getSites response:" << resp.toPrettyString();
		if(resp.isError()) {
			shvError() << "getSites error:" << resp.error().toString();
			return;
		}
		std::vector<std::string> paths;
		loadSitesHelper(paths, resp.result());
	});
}

void ShvSitesApp::loadSitesHelper(std::vector<std::string> &paths, const shv::chainpack::RpcValue &value)
{
	if(value.isMap()) {
		const cp::RpcValue::Map &map = value.toMap();
		if(map.size() == 1 && map.hasKey("_meta")) {
			const cp::RpcValue::Map &meta = map.value("_meta").toMap();
			cp::RpcValue::List gpsl = meta.value("gps").toList();
			if(gpsl.size() > 1) {
				cp::RpcValue name = meta.value("name");
				GPS gps(gpsl[0].toDouble(), gpsl[1].toDouble());
				sitesModel()->addSite(
							QString::fromStdString(shv::core::String::join(paths, '/')),
							QString::fromStdString(name.asString()) ,
							gps);
				{
					std::string shv_path = shv::core::String::join(paths, '/');
					shv::iotqt::rpc::ClientConnection *conn = rpcConnection();
					int rqid = conn->callShvMethod(shv_path, "dir");
					iot::rpc::RpcResponseCallBack *cb = new iot::rpc::RpcResponseCallBack(conn, rqid, this);
					cb->start([this, shv_path](const shv::chainpack::RpcResponse &resp) {
						shvDebug() << "Received dir response:" << resp.toPrettyString();
						if(!resp.isError()) {
							sitesModel()->setSiteBrokerConnected(shv_path, true);
						}
					});
				}
			}
		}
		else for(const auto &kv : map) {
			paths.push_back(kv.first);
			const cp::RpcValue::Map &m = kv.second.toMap();
			loadSitesHelper(paths, m);
			paths.pop_back();
		}
	}
}
