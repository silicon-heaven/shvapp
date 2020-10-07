#pragma once

#include <shv/iotqt/node/shvnode.h>

#include <QElapsedTimer>

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
	shv::chainpack::RpcValue getConfig(const QString &shv_path) const;
	shv::chainpack::RpcValue get(const shv::core::StringViewList &shv_path);
	shv::chainpack::RpcValue readFile(const shv::core::StringViewList &shv_path);
	void saveConfig(const QString &shv_path, const QByteArray &value);
	QString nodeFilesPath(const QString &shv_path) const;
	QString nodeConfigPath(const QString &shv_path) const;
	QString nodeLocalPath(const QString &shv_path) const;
	QString nodeLocalPath(const std::string &shv_path) const { return nodeLocalPath(QString::fromStdString(shv_path)); }
	bool isFile(const shv::iotqt::node::ShvNode::StringViewList &shv_path);

	shv::chainpack::RpcValue lsDir(const shv::core::StringViewList &shv_path);

	shv::chainpack::RpcValue::Map m_sites;
	QElapsedTimer m_sitesSyncedBefore;
	bool m_downloadingSites = false;
	std::string m_downloadSitesError;
};



