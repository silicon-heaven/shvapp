#include "shvfileproviderapp.h"
#include "appclioptions.h"
#include "sessionprocess.h"

#include <shv/iotqt/rpc/deviceconnection.h>
#include <shv/iotqt/rpc/tunnelhandle.h>
#include <shv/iotqt/node/shvnodetree.h>
#include <shv/iotqt/node/localfsnode.h>
#include <shv/coreqt/log.h>
#include <shv/chainpack/metamethod.h>

#include <shv/core/stringview.h>

#include <QProcess>
#include <QSocketNotifier>
#include <QTimer>
#include <QtGlobal>
#include <QFileInfo>

#ifdef Q_OS_UNIX
#include <unistd.h>
#include <errno.h>
#include <string.h>
#endif

#ifdef HANDLE_UNIX_SIGNALS
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace cp = shv::chainpack;

static std::vector<cp::MetaMethod> meta_methods {
	{cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, false},
	{cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, false},
	{cp::Rpc::METH_APP_NAME, cp::MetaMethod::Signature::RetVoid, false},
	{cp::Rpc::METH_CONNECTION_TYPE, cp::MetaMethod::Signature::RetVoid, false},
	{cp::Rpc::METH_DEVICE_ID, cp::MetaMethod::Signature::RetVoid, !cp::MetaMethod::IsSignal},
	{cp::Rpc::METH_HELP, cp::MetaMethod::Signature::RetParam, false}
};

size_t AppRootNode::methodCount(const StringViewList &shv_path)
{
	return (shv_path.empty()) ? meta_methods.size() : 0;
}

const shv::chainpack::MetaMethod *AppRootNode::metaMethod(const StringViewList &shv_path, size_t ix)
{
	if(shv_path.empty()) {
		if(meta_methods.size() <= ix)
			SHV_EXCEPTION("Invalid method index: " + std::to_string(ix) + " of: " + std::to_string(meta_methods.size()));
		return &(meta_methods[ix]);
	}
	return nullptr;
}

shv::chainpack::RpcValue AppRootNode::callMethod(const StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params)
{
	if(shv_path.empty()) {
		if(method == cp::Rpc::METH_APP_NAME) {
			return QCoreApplication::instance()->applicationName().toStdString();
		}
		if(method == cp::Rpc::METH_CONNECTION_TYPE) {
			return ShvFileProviderApp::instance()->rpcConnection()->connectionType();
		}
		if(method == cp::Rpc::METH_HELP) {
			return params.toString();
		}
	}
	return Super::callMethod(shv_path, method, params);
}

shv::chainpack::RpcValue AppRootNode::processRpcRequest(const shv::chainpack::RpcRequest &rq)
{
	if(rq.shvPath().toString().empty()) {
		if(rq.method() == cp::Rpc::METH_DEVICE_ID) {
			ShvFileProviderApp *app = ShvFileProviderApp::instance();
			cp::RpcValue::Map opts = app->rpcConnection()->connectionOptions().toMap();;
			cp::RpcValue::Map dev = opts.value(cp::Rpc::TYPE_DEVICE).toMap();
			//shvInfo() << dev[cp::Rpc::KEY_DEVICE_ID].toString();
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
	cp::RpcMessage::setMetaTypeExplicit(cli_opts->isMetaTypeExplicit());
	m_rpcConnection = new shv::iotqt::rpc::DeviceConnection(this);

	if(!cli_opts->user_isset()){
		cli_opts->setUser("iot");
	}

	if(!cli_opts->password_isset()){
		cli_opts->setPassword("lub42DUB");
	}

	m_rpcConnection->setCliOptions(cli_opts);

	connect(m_rpcConnection, &shv::iotqt::rpc::ClientConnection::brokerConnectedChanged, this, &ShvFileProviderApp::onBrokerConnectedChanged);
	connect(m_rpcConnection, &shv::iotqt::rpc::ClientConnection::rpcMessageReceived, this, &ShvFileProviderApp::onRpcMessageReceived);

	AppRootNode *root = new AppRootNode();
	m_shvTree = new shv::iotqt::node::ShvNodeTree(root, this);
	connect(m_shvTree->root(), &shv::iotqt::node::ShvRootNode::sendRpcMesage, m_rpcConnection, &shv::iotqt::rpc::ClientConnection::sendMessage);
	//m_shvTree->mkdir("sys/rproc");
	QString sys_fs_root_dir = cli_opts->sysFsRootDir();
	if(!sys_fs_root_dir.isEmpty() && QDir(sys_fs_root_dir).exists()) {
		const char *SYS_FS = "sys/fs";
		shvInfo() << "Exporting" << sys_fs_root_dir << "as" << SYS_FS << "node";
		shv::iotqt::node::LocalFSNode *fsn = new shv::iotqt::node::LocalFSNode(sys_fs_root_dir);
		m_shvTree->mount(SYS_FS, fsn);
	}

	if(cliOptions()->connStatusUpdateInterval() > 0) {
		QTimer *tm = new QTimer(this);
		connect(tm, &QTimer::timeout, this, &ShvFileProviderApp::updateConnStatusFile);
		tm->start(cliOptions()->connStatusUpdateInterval() * 1000);
	}

	QTimer::singleShot(0, m_rpcConnection, &shv::iotqt::rpc::ClientConnection::open);
}

ShvFileProviderApp::~ShvFileProviderApp()
{
	shvInfo() << "destroying shv file provider application";
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
		shvInfo() << "RPC request received:" << rq.toPrettyString();
		if(m_shvTree->root()) {
			m_shvTree->root()->handleRpcRequest(rq);
		}
	}
	else if(msg.isResponse()) {
		cp::RpcResponse rp(msg);
		shvInfo() << "RPC response received:" << rp.toPrettyString();
	}
	else if(msg.isNotify()) {
		cp::RpcNotify nt(msg);
		shvInfo() << "RPC notify received:" << nt.toPrettyString();
		/*
		if(nt.method() == cp::Rpc::NTF_VAL_CHANGED) {
			if(nt.shvPath() == "/test/shv/lublicator2/status") {
				shvInfo() << lublicatorStatusToString(nt.params().toUInt());
			}
		}
		*/
	}
}

void ShvFileProviderApp::updateConnStatusFile()
{
	QString fn = cliOptions()->connStatusFile();
	if(fn.isEmpty())
		return;
	QFile f(fn);
	QDir dir = QFileInfo(f).dir();
	if(!dir.mkpath(dir.absolutePath())) {
		shvError() << "Cannot create directory:" << dir.absolutePath();
		return;
	}
	if(f.open(QFile::WriteOnly)) {
		f.write(m_isBrokerConnected? "1": "0", 1);
	}
	else {
		shvError() << "Cannot write to connection statu file:" << fn;
	}
}


