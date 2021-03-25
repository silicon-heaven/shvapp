#pragma once

#include <shv/iotqt/node/shvnode.h>

#include <QElapsedTimer>

class QFile;

class AppRootNode : public shv::iotqt::node::ShvRootNode
{
	Q_OBJECT
	using Super = shv::iotqt::node::ShvRootNode;

public:
	explicit AppRootNode(QObject *parent = nullptr);

	size_t methodCount(const StringViewList &shv_path) override;
	const shv::chainpack::MetaMethod* metaMethod(const StringViewList &shv_path, size_t ix) override;
	shv::chainpack::RpcValue callMethodRq(const shv::chainpack::RpcRequest &rq) override;
	void handleRpcRequest(const shv::chainpack::RpcRequest &rq) override;

	QString nodeLocalPath(const QString &shv_path) const;
	shv::chainpack::RpcValue readFile(const QString &shv_path);
	shv::chainpack::RpcValue writeFile(const QString &shv_path, const std::string &content);

private:
	const std::vector<shv::chainpack::MetaMethod> &metaMethods(const StringViewList &shv_path);

	shv::chainpack::RpcValue getSites(const std::string &path);
	void findDevicesToSync(const StringViewList &shv_path, QStringList &result);
	shv::chainpack::RpcValue ls(const shv::core::StringViewList &shv_path, const shv::chainpack::RpcValue &params) override;
	bool hasData(const shv::iotqt::node::ShvNode::StringViewList &shv_path);
	bool isDevice(const shv::iotqt::node::ShvNode::StringViewList &shv_path);

	void downloadSites(std::function<void()> callback);
	void downloadSitesByNetworkManager(std::function<void(const shv::chainpack::RpcValue &)> callback);
	void downloadSitesFromShv(std::function<void(const shv::chainpack::RpcValue &)> callback);
	bool checkSites() const;

	Q_SIGNAL void downloadFinished();

	shv::chainpack::RpcValue ls_helper(const shv::core::StringViewList &shv_path, size_t index, const shv::chainpack::RpcValue::Map &sites_node);
	QString sitesFileName() const;

	shv::chainpack::RpcValue findSitesTreeValue(const shv::core::StringViewList &shv_path);
	shv::chainpack::RpcValue get(const shv::core::StringViewList &shv_path);
	shv::chainpack::RpcValue mkFile(const shv::core::StringViewList &shv_path, const shv::chainpack::RpcValue &params);
	QString nodeFilesPath(const QString &shv_path) const;
	QString nodeLocalPath(const std::string &shv_path) const { return nodeLocalPath(QString::fromStdString(shv_path)); }
	bool isFile(const shv::iotqt::node::ShvNode::StringViewList &shv_path);
	bool isDir(const shv::iotqt::node::ShvNode::StringViewList &shv_path);

	QStringList lsDir(const QString &shv_path);
	shv::chainpack::RpcValue readConfig(const QString &filename);
	shv::chainpack::RpcValue readAndMergeConfig(QFile &file);

	shv::chainpack::RpcValue::Map m_sites;
	QElapsedTimer m_sitesSyncedBefore;
	bool m_downloadingSites = false;
	std::string m_downloadSitesError;
};



