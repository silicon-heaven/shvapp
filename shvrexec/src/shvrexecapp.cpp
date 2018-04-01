#include "shvrexecapp.h"
#include "appclioptions.h"

#include <shv/iotqt/rpc/deviceconnection.h>
#include <shv/iotqt/node/shvnodetree.h>
//#include <shv/iotqt/node/localfsnode.h>

#include <shv/coreqt/log.h>

#include <QProcess>
#include <QTimer>

namespace cp = shv::chainpack;

const char METH_STDIN_WRITE[] = "stdinWrite";

shv::chainpack::RpcValue AppRootNode::dir(const shv::chainpack::RpcValue &methods_params)
{
	cp::RpcValue::List ret = Super::dir(methods_params).toList();
	ret.push_back(cp::Rpc::METH_APP_NAME);
	ret.push_back(METH_STDIN_WRITE);
	return ret;
}

shv::chainpack::RpcValue AppRootNode::call(const std::string &method, const shv::chainpack::RpcValue &params)
{
	if(method == cp::Rpc::METH_APP_NAME) {
		return QCoreApplication::instance()->applicationName().toStdString();
	}
	else if(method == METH_STDIN_WRITE) {
		const shv::chainpack::RpcValue::String &data = params.toString();
		qint64 len = ShvRExecApp::instance()->writeProcessStdin(data.data(), data.size());
		return cp::RpcValue{(int64_t)len};
	}
	return Super::call(method, params);
}

ShvRExecApp::ShvRExecApp(int &argc, char **argv, AppCliOptions* cli_opts)
	: Super(argc, argv)
	, m_cliOptions(cli_opts)
{
	cp::RpcMessage::setMetaTypeExplicit(cli_opts->isMetaTypeExplicit());

	m_rpcConnection = new shv::iotqt::rpc::DeviceConnection(this);

	if(!cli_opts->user_isset())
		cli_opts->setUser("iot");
	m_rpcConnection->setCliOptions(cli_opts);
	m_rpcConnection->setCheckBrokerConnectedInterval(0);

	connect(m_rpcConnection, &shv::iotqt::rpc::ClientConnection::socketConnectedChanged, [](bool connected) {
		if(!connected) {
			shvError() << "Socket disconnected, quitting";
			quit();
		}
	});
	connect(m_rpcConnection, &shv::iotqt::rpc::ClientConnection::brokerConnectedChanged, this, &ShvRExecApp::onBrokerConnectedChanged);
	connect(m_rpcConnection, &shv::iotqt::rpc::ClientConnection::rpcMessageReceived, this, &ShvRExecApp::onRpcMessageReceived);

	AppRootNode *root = new AppRootNode();
	m_shvTree = new shv::iotqt::node::ShvNodeTree(root, this);
	/*
	m_shvTree->mkdir("sys");
	QString sys_fs_root_dir = cli_opts->sysFsRootDir();
	if(!sys_fs_root_dir.isEmpty() && QDir(sys_fs_root_dir).exists()) {
		const char *SYS_FS = "sys/fs";
		shvInfo() << "Exporting" << sys_fs_root_dir << "as" << SYS_FS << "node";
		shv::iotqt::node::LocalFSNode *fsn = new shv::iotqt::node::LocalFSNode(sys_fs_root_dir);
		m_shvTree->mount(SYS_FS, fsn);
	}
	*/

	QTimer::singleShot(0, m_rpcConnection, &shv::iotqt::rpc::ClientConnection::open);
}

ShvRExecApp::~ShvRExecApp()
{
	shvInfo() << "destroying shv agent application";
	disconnect(m_rpcConnection, &shv::iotqt::rpc::ClientConnection::socketConnectedChanged, 0, 0);
}

ShvRExecApp *ShvRExecApp::instance()
{
	return qobject_cast<ShvRExecApp*>(QCoreApplication::instance());
}

void ShvRExecApp::onBrokerConnectedChanged(bool is_connected)
{
	if(m_cmdProc) {
		delete m_cmdProc;
		m_cmdProc = nullptr;
	}
	if(is_connected) {
		QString exec_cmd = m_cliOptions->execCommand();
		shvInfo() << "Starting process:" << exec_cmd;
		QStringList sl = exec_cmd.split(' ', QString::SkipEmptyParts);
		QString program = sl.value(0);
		QStringList arguments = sl.mid(1);
		m_cmdProc = new QProcess(this);
		connect(m_cmdProc, &QProcess::readyReadStandardOutput, this, &ShvRExecApp::onReadyReadProcessStandardOutput);
		connect(m_cmdProc, &QProcess::readyReadStandardError, this, &ShvRExecApp::onReadyReadProcessStandardError);
		connect(m_cmdProc, &QProcess::errorOccurred, [](QProcess::ProcessError error) {
			shvError() << "Exec process error:" << error;
			quit();
		});
		connect(m_cmdProc, &QProcess::stateChanged, [](QProcess::ProcessState state) {
			shvDebug() << "Exec process new state:" << state;
		});
		connect(m_cmdProc, QOverload<int>::of(&QProcess::finished), this, [this](int exit_code) {
			shvInfo() << "Process" << m_cmdProc->program() << "finished with exit code:" << exit_code;
			quit();
		});
		m_cmdProc->start(program, arguments);
	}
}

void ShvRExecApp::onRpcMessageReceived(const shv::chainpack::RpcMessage &msg)
{
	shvLogFuncFrame() << msg.toCpon();
	if(msg.isRequest()) {
		cp::RpcRequest rq(msg);
		cp::RpcResponse resp = cp::RpcResponse::forRequest(rq);
		try {
			//shvInfo() << "RPC request received:" << rq.toCpon();
			const std::string shv_path = rq.shvPath();
			std::string path_rest;
			shv::iotqt::node::ShvNode *nd = m_shvTree->cd(shv_path, &path_rest);
			if(!nd)
				SHV_EXCEPTION("Path not found: " + shv_path);
			rq.setShvPath(path_rest);
			resp.setResult(nd->processRpcRequest(rq));
		}
		catch (shv::core::Exception &e) {
			resp.setError(cp::RpcResponse::Error::create(cp::RpcResponse::Error::MethodInvocationException, e.message()));
		}
		m_rpcConnection->sendMessage(resp);
	}
	else if(msg.isResponse()) {
		cp::RpcResponse rp(msg);
		shvInfo() << "RPC response received:" << rp.toCpon();
	}
	else if(msg.isNotify()) {
		cp::RpcNotify ntf(msg);
		std::string shv_path = ntf.shvPath();
		if(shv_path == "in") {
			const shv::chainpack::RpcValue::Blob &data = ntf.params().toBlob();
			writeProcessStdin(data.data(), data.size());
			return;
		}
		shvInfo() << "RPC notify received:" << ntf.toCpon();
	}
}

qint64 ShvRExecApp::writeProcessStdin(const char *data, size_t len)
{
	if(!m_cmdProc) {
		shvError() << "Attempt to write to not existing process.";
		return -1;
	}
	qint64 n = m_cmdProc->write(data, len);
	if(n != (qint64)len) {
		shvError() << "Write process stdin error, only" << n << "of" << len << "bytes written.";
	}
	return n;
}

void ShvRExecApp::onReadyReadProcessStandardOutput()
{
	m_cmdProc->setReadChannel(QProcess::StandardOutput);
	QByteArray ba = m_cmdProc->readAll();
	if(!m_rpcConnection->isBrokerConnected()) {
		shvError() << "Broker is not connected, throwing away process stdout:" << ba;
	}
	else {
		m_rpcConnection->sendShvNotify("out", cp::Rpc::NTF_VAL_CHANGED, cp::RpcValue::Blob(ba.constData(), ba.size()));
	}
}

void ShvRExecApp::onReadyReadProcessStandardError()
{
	m_cmdProc->setReadChannel(QProcess::StandardError);
	QByteArray ba = m_cmdProc->readAll();
	if(!m_rpcConnection->isBrokerConnected()) {
		shvError() << "Broker is not connected, throwing away process stderr:" << ba;
	}
	else {
		m_rpcConnection->sendShvNotify("err", cp::Rpc::NTF_VAL_CHANGED, cp::RpcValue::Blob(ba.constData(), ba.size()));
	}
}

