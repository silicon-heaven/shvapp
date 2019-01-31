#include "shvfileproviderapp.h"
#include "appclioptions.h"
#include "brclabfsnode.h"

#include <shv/iotqt/rpc/deviceconnection.h>
#include <shv/iotqt/node/shvnodetree.h>
#include <shv/coreqt/log.h>
#include <shv/chainpack/metamethod.h>

#include <shv/core/stringview.h>

#include <QSocketNotifier>
#include <QTimer>
#include <QtGlobal>

#ifdef Q_OS_UNIX
#include <unistd.h>
#include <errno.h>
#include <string.h>
#endif

namespace cp = shv::chainpack;

static std::vector<cp::MetaMethod> meta_methods {
	{cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, false, cp::Rpc::GRANT_BROWSE},
	{cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, false, cp::Rpc::GRANT_BROWSE},
	{cp::Rpc::METH_APP_NAME, cp::MetaMethod::Signature::RetVoid, false, cp::Rpc::GRANT_READ},
	{cp::Rpc::METH_DEVICE_ID, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsGetter, cp::Rpc::GRANT_READ}
};

AppRootNode::AppRootNode(const QString &root_path, AppRootNode::Super *parent):
	Super(root_path, parent)
{
	m_isRootNode = true;
}

size_t AppRootNode::methodCount(const StringViewList &shv_path)
{
	return (shv_path.empty()) ? meta_methods.size() : Super::methodCount(shv_path);
}

const shv::chainpack::MetaMethod *AppRootNode::metaMethod(const StringViewList &shv_path, size_t ix)
{
	if(shv_path.empty()) {
		if(meta_methods.size() <= ix)
			SHV_EXCEPTION("Invalid method index: " + std::to_string(ix) + " of: " + std::to_string(meta_methods.size()));
		return &(meta_methods[ix]);
	}

	return Super::metaMethod(shv_path, ix);
}

shv::chainpack::RpcValue AppRootNode::callMethod(const StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params)
{
	if(shv_path.empty()) {
		if(method == cp::Rpc::METH_APP_NAME) {
			return QCoreApplication::instance()->applicationName().toStdString();
		}
	}
	return Super::callMethod(shv_path, method, params);
}

shv::chainpack::RpcValue AppRootNode::processRpcRequest(const shv::chainpack::RpcRequest &rq)
{
	if(rq.shvPath().toString().empty()) {
		if(rq.method() == cp::Rpc::METH_DEVICE_ID) {
			ShvFileProviderApp *app = ShvFileProviderApp::instance();
			const cp::RpcValue::Map& opts = app->rpcConnection()->connectionOptions().toMap();
			const cp::RpcValue::Map& dev = opts.value(cp::Rpc::KEY_DEVICE).toMap();
			return dev.value(cp::Rpc::KEY_DEVICE_ID).toString();
		}
	}
	return Super::processRpcRequest(rq);
}

ShvFileProviderApp::ShvFileProviderApp(int &argc, char **argv, AppCliOptions* cli_opts)
	: Super(argc, argv)
	, m_cliOptions(cli_opts)
{
#ifdef Q_OS_UNIX
	if(0 != ::setpgid(0, 0))
		shvError() << "Error set process group ID:" << errno << ::strerror(errno);
#endif

	m_rpcConnection = new shv::iotqt::rpc::DeviceConnection(this);

	if(!cli_opts->user_isset())
		cli_opts->setUser("shvfileprovider");

	m_rpcConnection->setCliOptions(cli_opts);

	connect(m_rpcConnection, &shv::iotqt::rpc::ClientConnection::brokerConnectedChanged, this, &ShvFileProviderApp::onBrokerConnectedChanged);
	connect(m_rpcConnection, &shv::iotqt::rpc::ClientConnection::rpcMessageReceived, this, &ShvFileProviderApp::onRpcMessageReceived);

	QString root_dir = QString::fromStdString(cli_opts->fsRootDir());
	if(!root_dir.isEmpty() && QDir(root_dir).exists()) {
		shvInfo() << "Exporting" << root_dir << "as root node";
		m_root = new AppRootNode(root_dir);
		connect(m_root, &shv::iotqt::node::ShvNode::sendRpcMesage, m_rpcConnection, &shv::iotqt::rpc::ClientConnection::sendMessage);
	}
	else{
		shvError() << "Invalid param fsRootDir: " + root_dir.toStdString();
	}

	QTimer::singleShot(0, m_rpcConnection, &shv::iotqt::rpc::ClientConnection::open);
}

ShvFileProviderApp::~ShvFileProviderApp()
{
	shvInfo() << "destroying shv file provider application";

	if (m_root){
		delete m_root;
	}
}

ShvFileProviderApp *ShvFileProviderApp::instance()
{
	return qobject_cast<ShvFileProviderApp *>(QCoreApplication::instance());
}

void ShvFileProviderApp::onBrokerConnectedChanged(bool is_connected)
{
	m_isBrokerConnected = is_connected;
}

void ShvFileProviderApp::onRpcMessageReceived(const shv::chainpack::RpcMessage &msg)
{
	shvLogFuncFrame() << msg.toCpon();
	if(msg.isRequest()) {
		cp::RpcRequest rq(msg);
		shvDebug() << "RPC request received:" << rq.toPrettyString();
		if(m_root) {
			m_root->handleRpcRequest(rq);
		}
	}
	else if(msg.isResponse()) {
		cp::RpcResponse rp(msg);
		shvDebug() << "RPC response received:" << rp.toPrettyString();
	}
	else if(msg.isSignal()) {
		cp::RpcSignal nt(msg);
		shvDebug() << "RPC signal received:" << nt.toPrettyString();
	}
}

