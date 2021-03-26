#include "sitesproviderapp.h"
#include "appclioptions.h"
#include "nodes.h"

#include <shv/iotqt/node/shvnodetree.h>
#include <shv/iotqt/rpc/rpc.h>
#include <shv/coreqt/log.h>
#include <shv/coreqt/exception.h>
#include <shv/chainpack/metamethod.h>
#include <shv/core/stringview.h>
#include <shv/iotqt/rpc/deviceconnection.h>

#include <QDir>
#include <QFileInfo>
#include <QNetworkReply>
#include <QTimer>

#ifdef Q_OS_UNIX
#include <unistd.h>
#include <errno.h>
#include <string.h>
#endif

namespace cp = shv::chainpack;

SitesProviderApp::SitesProviderApp(int &argc, char **argv, AppCliOptions* cli_opts)
	: Super(argc, argv)
	, m_cliOptions(cli_opts)
{
	//cp::RpcMessage::setMetaTypeExplicit(cli_opts->isMetaTypeExplicit());
	m_rpcConnection = new shv::iotqt::rpc::DeviceConnection(this);

	if (!cli_opts->deviceId_isset()) {
		cli_opts->setDeviceId("shvsitesprovider");
	}
	m_rpcConnection->setCliOptions(cli_opts);

	connect(m_rpcConnection, &shv::iotqt::rpc::ClientConnection::rpcMessageReceived, this, &SitesProviderApp::onRpcMessageReceived);

	m_rootNode = new AppRootNode();
	m_shvTree = new shv::iotqt::node::ShvNodeTree(m_rootNode, this);

	connect(m_shvTree->root(), &shv::iotqt::node::ShvRootNode::sendRpcMessage, m_rpcConnection, &shv::iotqt::rpc::ClientConnection::sendMessage);

	QTimer::singleShot(0, m_rpcConnection, &shv::iotqt::rpc::ClientConnection::open);
}

SitesProviderApp::~SitesProviderApp()
{
	shvInfo() << "destroying shvsitesprovider application";
}

SitesProviderApp *SitesProviderApp::instance()
{
	return qobject_cast<SitesProviderApp *>(QCoreApplication::instance());
}

QString SitesProviderApp::remoteSitesUrl() const
{
	return QString::fromStdString(m_cliOptions->remoteSitesUrl());
}

void SitesProviderApp::onRpcMessageReceived(const cp::RpcMessage &msg)
{
	shvLogFuncFrame() << msg.toCpon();
	if (msg.isRequest()) {
		cp::RpcRequest rq(msg);
		if(m_shvTree->root()) {
			m_shvTree->root()->handleRpcRequest(rq);
		}
	}
}
