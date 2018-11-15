#pragma once

#include <shv/iotqt/node/shvnode.h>

#include <QCoreApplication>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>

#include <shv/chainpack/rpcmessage.h>

class AppCliOptions;
class QTimer;

namespace shv { namespace chainpack { class RpcMessage; }}
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
	shv::chainpack::RpcValue processRpcRequest(const shv::chainpack::RpcRequest &rq) override;
	void handleRpcRequest(const shv::chainpack::RpcRequest &rq) override;

	Q_SIGNAL void methodFinished(shv::chainpack::RpcResponse &resp);

private:
	static const std::vector<shv::chainpack::MetaMethod> &metaMethods(const StringViewList &shv_path);
};

class SitesProviderApp : public QCoreApplication
{
	Q_OBJECT
private:
	using Super = QCoreApplication;
public:
	SitesProviderApp(int &argc, char **argv, AppCliOptions* cli_opts);
	~SitesProviderApp() Q_DECL_OVERRIDE;

	static SitesProviderApp *instance();
	shv::iotqt::rpc::DeviceConnection *rpcConnection() const {return m_rpcConnection;}
	shv::chainpack::RpcValue getSites(std::function<void(shv::chainpack::RpcValue)> callback);
	shv::chainpack::RpcValue ls(const shv::core::StringViewList &shv_path, std::function<void(shv::chainpack::RpcValue)> callback);
	shv::chainpack::RpcValue getConfig(const shv::chainpack::RpcValue &params);
	shv::chainpack::RpcValue get(const shv::core::StringViewList &shv_path, std::function<void(shv::chainpack::RpcValue)> callback);
	shv::chainpack::RpcValue dir(const shv::core::StringViewList &shv_path, const shv::chainpack::RpcValue &params, std::function<void(shv::chainpack::RpcValue)> callback);
	void saveConfig(const shv::chainpack::RpcValue &params);
	bool hasData(const shv::iotqt::node::ShvNode::StringViewList &shv_path);

	AppCliOptions* cliOptions() {return m_cliOptions;}

	void downloadSites(std::function<void()> callback);
	bool checkSites() const;

	Q_SIGNAL void downloadFinished();

private:
	class Config
	{
	public:
		QByteArray content;
		shv::chainpack::RpcValue chainpack;
		QDateTime time;
	};

	void onRpcMessageReceived(const shv::chainpack::RpcMessage &msg);
	shv::chainpack::RpcValue ls(const shv::core::StringViewList &shv_path, size_t index, const shv::chainpack::RpcValue::Map &object);

	shv::chainpack::RpcValue leaf(const shv::core::StringViewList &shv_path);
	QByteArray getConfig(const QString &node_id);
	shv::chainpack::RpcValue get(const shv::core::StringViewList &shv_path);
	void saveConfig(const QString &node_id, const QByteArray &value);
	QString nodeConfigPath(const QString &node_id);
	void checkConfig(const QString &path);
	void readConfig(const QString &path);

	shv::iotqt::rpc::DeviceConnection *m_rpcConnection = nullptr;
	AppCliOptions* m_cliOptions;

	shv::iotqt::node::ShvNodeTree *m_shvTree = nullptr;
	AppRootNode *m_rootNode;
	std::string m_sitesString;
	shv::chainpack::RpcValue::Map m_sitesCp;
	QDateTime m_sitesTime;
	bool m_downloadingSites = false;
	QMap<QString, Config> m_configs;
};

