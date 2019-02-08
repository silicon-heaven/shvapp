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

static const char M_ADD_USER[] = "addUser";
static const char M_CHANGE_USER_PASSWORD[] = "changeUserPassword";
static const char M_GET_USER_GRANTS[] = "getUserGrants";

static std::vector<cp::MetaMethod> meta_methods {
	{cp::Rpc::METH_APP_NAME, cp::MetaMethod::Signature::RetVoid, false, cp::Rpc::GRANT_READ},
	{cp::Rpc::METH_DEVICE_ID, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsGetter, cp::Rpc::GRANT_READ},
	{M_ADD_USER, cp::MetaMethod::Signature::RetParam, 0, cp::Rpc::GRANT_WRITE},
	{M_CHANGE_USER_PASSWORD, cp::MetaMethod::Signature::RetParam, 0, cp::Rpc::GRANT_WRITE},
	{M_GET_USER_GRANTS, cp::MetaMethod::Signature::RetParam, 0, cp::Rpc::GRANT_WRITE}
};

AppRootNode::AppRootNode(const QString &root_path, AppRootNode::Super *parent):
	Super(root_path, parent)
{
	m_isRootNode = true;
	m_isRootNodeValid = !root_path.isEmpty() && QDir(root_path).exists();

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

	return Super::metaMethod(shv_path, ix);
}

shv::chainpack::RpcValue AppRootNode::callMethod(const StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params)
{
	if (!m_isRootNodeValid){
		SHV_EXCEPTION("Invalid root path: " + m_rootDir.absolutePath().toStdString());
	}

	if(shv_path.empty()) {
		ShvFileProviderApp *app = ShvFileProviderApp::instance();

		if(method == cp::Rpc::METH_APP_NAME) {
			return QCoreApplication::instance()->applicationName().toStdString();
		}
		else if(method == cp::Rpc::METH_DEVICE_ID) {
			const cp::RpcValue::Map& opts = app->rpcConnection()->connectionOptions().toMap();
			const cp::RpcValue::Map& dev = opts.value(cp::Rpc::KEY_DEVICE).toMap();
			return dev.value(cp::Rpc::KEY_DEVICE_ID).toString();
		}
		else if(method == M_ADD_USER) {
			if (!params.isMap() || !params.toMap().hasKey("user") || !params.toMap().hasKey("password")){
				SHV_EXCEPTION("Invalid parameters format. Params must be a map and it must contains keys: user, password.");
			}
			cp::RpcValue::Map map = params.toMap();
			return app->brclabUsers()->addUser(map.value("user").toStdString(), map.value("password").toStdString());
		}
		else if(method == M_CHANGE_USER_PASSWORD) {
			if (!params.isMap() || !params.toMap().hasKey("user") || !params.toMap().hasKey("newPassword")){
				SHV_EXCEPTION("Invalid parameters format. Params must be a map and it must contains keys: user, newPassword.");
			}
			cp::RpcValue::Map map = params.toMap();
			return app->brclabUsers()->changePassword(map.value("user").toStdString(), map.value("newPassword").toStdString());
		}
		else if(method == M_GET_USER_GRANTS) {
			if (!params.isMap() || !params.toMap().hasKey("user") || !params.toMap().hasKey("password")){
				SHV_EXCEPTION("Invalid parameters format. Params must be a map and it must contains keys: user, password.");
			}

			cp::RpcValue::Map map = params.toMap();
			return app->brclabUsers()->getUserGrants(map.value("user").toStdString(), map.value("password").toStdString());
		}
		else{
			return Super::callMethod(shv_path, method, params);
		}
	}

	return Super::callMethod(shv_path, method, params);
}

shv::chainpack::RpcValue AppRootNode::processRpcRequest(const shv::chainpack::RpcRequest &rq)
{
	return Super::processRpcRequest(rq);
}

ShvFileProviderApp::ShvFileProviderApp(int &argc, char **argv, AppCliOptions* cli_opts)
	: Super(argc, argv)
	, m_cliOptions(cli_opts)
	, m_brclabUsers(brclabUsersFileName(), this)
{
#ifdef Q_OS_UNIX
	if(0 != ::setpgid(0, 0))
		shvError() << "Error set process group ID:" << errno << ::strerror(errno);
#endif

	//m_brclabUsers.addUser("test", "8884a26b82a69838092fd4fc824bbfde56719e01");
	//cp::RpcValue grants = m_brclabUsers.getUserGrants("iot", "8884a26b82a69838092fd4fc824bbfde56719e02");

	//shvInfo() << grants.isValid() << grants.isList() << grants.toList().size();

	m_rpcConnection = new shv::iotqt::rpc::DeviceConnection(this);

	if(!cli_opts->user_isset())
		cli_opts->setUser("shvfileprovider");

	m_rpcConnection->setCliOptions(cli_opts);

	connect(m_rpcConnection, &shv::iotqt::rpc::ClientConnection::brokerConnectedChanged, this, &ShvFileProviderApp::onBrokerConnectedChanged);
	connect(m_rpcConnection, &shv::iotqt::rpc::ClientConnection::rpcMessageReceived, this, &ShvFileProviderApp::onRpcMessageReceived);

	QString root_dir = QString::fromStdString(cli_opts->fsRootDir());
	m_root = new AppRootNode(root_dir);

	connect(m_root, &shv::iotqt::node::ShvNode::sendRpcMesage, m_rpcConnection, &shv::iotqt::rpc::ClientConnection::sendMessage);
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

std::string ShvFileProviderApp::brclabUsersFileName()
{
	return cliOptions()->configDir() + "brclabusers.cpon";
}

BrclabUsers *ShvFileProviderApp::brclabUsers()
{
	return &m_brclabUsers;
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

