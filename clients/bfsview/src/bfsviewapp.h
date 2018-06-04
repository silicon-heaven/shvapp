#pragma once

#include <shv/iotqt/node/shvnode.h>

#include <QApplication>

class AppCliOptions;
namespace shv { namespace chainpack { class RpcMessage; }}
namespace shv { namespace iotqt { namespace rpc { class DeviceConnection; }}}
namespace shv { namespace iotqt { namespace node { class ShvNodeTree; }}}

class AppRootNode : public shv::iotqt::node::ShvRootNode
{
	using Super = shv::iotqt::node::ShvRootNode;
public:
	explicit AppRootNode(QObject *parent = nullptr) : Super(parent) {}

	size_t methodCount() override;
	const shv::chainpack::MetaMethod* metaMethod(size_t ix) override;

	shv::chainpack::RpcValue call(const std::string &method, const shv::chainpack::RpcValue &params) override;

	//shv::chainpack::RpcValue processRpcRequest(const shv::chainpack::RpcRequest &rq) override;
};

class PwrStatusNode : public shv::iotqt::node::ShvNode
{
	using Super = shv::iotqt::node::ShvNode;
public:
	explicit PwrStatusNode(shv::iotqt::node::ShvNode *parent = nullptr) : Super(parent) {}

	unsigned pwrStatus() const {return m_pwrStatus;}
	void setPwrStatus(unsigned s);
	void emitPwrStatusChanged();

	size_t methodCount() override;
	const shv::chainpack::MetaMethod* metaMethod(size_t ix) override;

	shv::chainpack::RpcValue call(const std::string &method, const shv::chainpack::RpcValue &params) override;
private:
	unsigned m_pwrStatus = 0;
};

class BfsViewApp : public QApplication
{
	Q_OBJECT
private:
	using Super = QApplication;
public:
	BfsViewApp(int &argc, char **argv, AppCliOptions* cli_opts);
	~BfsViewApp() Q_DECL_OVERRIDE;

	static BfsViewApp *instance();
	shv::iotqt::rpc::DeviceConnection *rpcConnection() const {return m_rpcConnection;}
	AppCliOptions* cliOptions() {return m_cliOptions;}

private:
	void onBrokerConnectedChanged(bool is_connected);
	void onRpcMessageReceived(const shv::chainpack::RpcMessage &msg);
	//void onRootNodeSendRpcMesage(const shv::chainpack::RpcMessage &msg);
private:
	shv::iotqt::rpc::DeviceConnection *m_rpcConnection = nullptr;
	AppCliOptions* m_cliOptions;

	shv::iotqt::node::ShvNodeTree *m_shvTree = nullptr;
	PwrStatusNode *m_pwrStatusNode;
};

