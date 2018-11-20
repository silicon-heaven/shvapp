#pragma once

#include <shv/iotqt/node/shvnode.h>

#include <QCoreApplication>
#include <QDateTime>
#include <QMap>

#include <shv/chainpack/rpcmessage.h>

class AppCliOptions;
class QTimer;

namespace shv { namespace iotqt { namespace rpc { class DeviceConnection; }}}
namespace shv { namespace iotqt { namespace node { class ShvNodeTree; }}}

class AppRootNode : public shv::iotqt::node::ShvRootNode
{
	Q_OBJECT
	using Super = shv::iotqt::node::ShvRootNode;

public:
	explicit AppRootNode(QObject *parent = nullptr);

	size_t methodCount(const StringViewList &shv_path) override;
	const shv::chainpack::MetaMethod* metaMethod(const StringViewList &shv_path, size_t ix) override;
	shv::chainpack::RpcValue callMethod(const StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params) override;
	void handleRpcRequest(const shv::chainpack::RpcRequest &rq) override;

private:
	const std::vector<shv::chainpack::MetaMethod> &metaMethods(const StringViewList &shv_path);

	shv::chainpack::RpcValue getSites();
	shv::chainpack::RpcValue ls(const shv::core::StringViewList &shv_path, const shv::chainpack::RpcValue &params) override;
	shv::chainpack::RpcValue getConfig(const shv::chainpack::RpcValue &params);
	void saveConfig(const shv::chainpack::RpcValue &params);
	bool hasData(const shv::iotqt::node::ShvNode::StringViewList &shv_path);

	void downloadSites(std::function<void()> callback);
	bool checkSites() const;

	Q_SIGNAL void downloadFinished();

	shv::chainpack::RpcValue ls(const shv::core::StringViewList &shv_path, size_t index, const shv::chainpack::RpcValue::Map &object);

	shv::chainpack::RpcValue leaf(const shv::core::StringViewList &shv_path);
	QByteArray getConfig(const QString &shv_path);
	shv::chainpack::RpcValue get(const shv::core::StringViewList &shv_path);
	void saveConfig(const QString &shv_path, const QByteArray &value);
	QString nodeConfigPath(const QString &shv_path) const;
	QString nodeLocalPath(const QString &shv_path) const;
	QString nodeLocalPath(const std::string &shv_path) const;
	QString nodeLocalPath(const shv::core::StringViewList &shv_path) const;
	bool isFile(const shv::iotqt::node::ShvNode::StringViewList &shv_path);

	std::string m_sitesJsonString;
	shv::chainpack::RpcValue::Map m_sitesValue;
	QDateTime m_sitesTime;
	bool m_downloadingSites = false;
};

class SitesProviderApp : public QCoreApplication
{
	Q_OBJECT
	using Super = QCoreApplication;

public:
	SitesProviderApp(int &argc, char **argv, AppCliOptions* cli_opts);
	~SitesProviderApp() Q_DECL_OVERRIDE;

	static SitesProviderApp *instance();
	shv::iotqt::rpc::DeviceConnection *rpcConnection() const { return m_rpcConnection; }
	AppCliOptions* cliOptions() {return m_cliOptions;}

private:
	void onRpcMessageReceived(const shv::chainpack::RpcMessage &msg);

	shv::iotqt::rpc::DeviceConnection *m_rpcConnection = nullptr;
	AppCliOptions* m_cliOptions;

	shv::iotqt::node::ShvNodeTree *m_shvTree = nullptr;
	AppRootNode *m_rootNode;
};

