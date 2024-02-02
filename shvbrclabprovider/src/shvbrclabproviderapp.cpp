#include "shvbrclabproviderapp.h"
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

const std::string AppRootNode::BRCLAB_NODE = ".brclab";

static std::vector<cp::MetaMethod> meta_methods {
	{cp::Rpc::METH_APP_NAME, cp::MetaMethod::Signature::RetVoid, false, cp::Rpc::ROLE_READ},
	{cp::Rpc::METH_DEVICE_ID, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsGetter, cp::Rpc::ROLE_READ},
	{cp::Rpc::METH_DEVICE_TYPE, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsGetter, cp::Rpc::ROLE_BROWSE},
};

AppRootNode::AppRootNode(const QString &root_path, AppRootNode::Super *parent):
	Super(root_path, parent)
{
	m_isRootNode = true;
	m_isRootNodeValid = !root_path.isEmpty() && QDir(root_path).exists();
	m_brclabNode = new BrclabNode(BRCLAB_NODE, this);

	if (!m_isRootNodeValid){
		shvError() << "Invalid root path: " << root_path.toStdString();
	}
	else{
		shvInfo() << "Exporting" << root_path.toStdString() << "as root node";
	}
}

size_t AppRootNode::methodCount(const StringViewList &shv_path)
{
	return (shv_path.empty()) ? meta_methods.size() + Super::methodCount(shv_path) : Super::methodCount(shv_path);
}

const shv::chainpack::MetaMethod *AppRootNode::metaMethod(const StringViewList &shv_path, size_t ix)
{
	if(shv_path.empty()) {
		size_t all_method_count = meta_methods.size() + Super::methodCount(shv_path);

		if (ix < meta_methods.size())
			return &(meta_methods[ix]);
		else if (ix < all_method_count)
			return Super::metaMethod(shv_path, ix - meta_methods.size());
		else
			SHV_EXCEPTION("Invalid method index: " + std::to_string(ix) + " of: " + std::to_string(all_method_count));
	}
	else if (shv_path[0] == BRCLAB_NODE){
		return nullptr;
	}

	return Super::metaMethod(shv_path, ix);
}

shv::iotqt::node::ShvNode::StringList AppRootNode::childNames(const shv::iotqt::node::ShvNode::StringViewList &shv_path)
{
	shv::iotqt::node::ShvNode::StringList child_names = Super::childNames(shv_path);

	if (shv_path.size() == 0){
		child_names.push_back(BRCLAB_NODE);
	}

	return child_names;
}

shv::chainpack::RpcValue AppRootNode::hasChildren(const shv::iotqt::node::ShvNode::StringViewList &shv_path)
{
	if (shv_path.size() > 0 && shv_path[0] == BRCLAB_NODE){
		return true;
	}

	return Super::hasChildren(shv_path);
}

shv::chainpack::RpcValue AppRootNode::callMethod(const StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params, const shv::chainpack::RpcValue &user_id)
{
	if (!m_isRootNodeValid){
		SHV_EXCEPTION("Invalid root path: " + m_rootDir.absolutePath().toStdString());
	}

	if(shv_path.empty()) {
		ShvBrclabProviderApp *app = ShvBrclabProviderApp::instance();

		if(method == cp::Rpc::METH_APP_NAME) {
			return QCoreApplication::instance()->applicationName().toStdString();
		}
		else if(method == cp::Rpc::METH_DEVICE_ID) {
			const cp::RpcValue opts = app->rpcConnection()->connectionOptions();
			const cp::RpcValue::Map &dev = opts.asMap().valref(cp::Rpc::KEY_DEVICE).asMap();
			return dev.value(cp::Rpc::KEY_DEVICE_ID).toString();
		}
		if(method == cp::Rpc::METH_DEVICE_TYPE) {
			return "BrclabProvider";
		}
	}

	return Super::callMethod(shv_path, method, params, user_id);
}

shv::chainpack::RpcValue AppRootNode::processRpcRequest(const shv::chainpack::RpcRequest &rq)
{
	return Super::processRpcRequest(rq);
}

ShvBrclabProviderApp::ShvBrclabProviderApp(int &argc, char **argv, AppCliOptions* cli_opts)
	: Super(argc, argv)
	, m_cliOptions(cli_opts)
{
	m_rpcConnection = new shv::iotqt::rpc::DeviceConnection(this);

	if(!cli_opts->user_isset())
		cli_opts->setUser("shvbrclabprovider");

	m_rpcConnection->setCliOptions(cli_opts);

	connect(m_rpcConnection, &shv::iotqt::rpc::ClientConnection::brokerConnectedChanged, this, &ShvBrclabProviderApp::onBrokerConnectedChanged);
	connect(m_rpcConnection, &shv::iotqt::rpc::ClientConnection::rpcMessageReceived, this, &ShvBrclabProviderApp::onRpcMessageReceived);

	QString root_dir = QString::fromStdString(cli_opts->fsRootDir());
	m_root = new AppRootNode(root_dir);

	connect(m_root, &shv::iotqt::node::ShvNode::sendRpcMessage, m_rpcConnection, &shv::iotqt::rpc::ClientConnection::sendRpcMessage);
	QTimer::singleShot(0, m_rpcConnection, &shv::iotqt::rpc::ClientConnection::open);
}

ShvBrclabProviderApp::~ShvBrclabProviderApp()
{
	shvInfo() << "destroying shv file provider application";

	if (m_root){
		delete m_root;
	}
}

ShvBrclabProviderApp *ShvBrclabProviderApp::instance()
{
	return qobject_cast<ShvBrclabProviderApp *>(QCoreApplication::instance());
}

std::string ShvBrclabProviderApp::brclabUsersFileName()
{
	return cliOptions()->configDir() + "/" +"brclabusers.cpon";
}

void ShvBrclabProviderApp::onBrokerConnectedChanged(bool is_connected)
{
	m_isBrokerConnected = is_connected;
}

void ShvBrclabProviderApp::onRpcMessageReceived(const shv::chainpack::RpcMessage &msg)
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

