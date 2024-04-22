#pragma once

#include <shv/iotqt/node/shvnode.h>

#include <QElapsedTimer>
#include <QTimer>

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

	QString nodeLocalPath(const QString &shv_path = QString()) const;
	shv::chainpack::RpcValue readFile(const QString &shv_path);
	shv::chainpack::RpcValue writeFile(const QString &shv_path, const std::string &content);

	shv::chainpack::RpcValue readFileCompressed(const shv::chainpack::RpcRequest &request);

private:
	const std::vector<shv::chainpack::MetaMethod> &metaMethods(const StringViewList &shv_path);

	shv::chainpack::RpcValue getSites(const QString &shv_path);
	shv::chainpack::RpcValue getSitesInfo() const;
	shv::chainpack::RpcValue getSitesTgz() const;
	void updateSitesTgz();
	void createSitesTgz(std::function<void(const QByteArray &, const QString &)> callback);
	void saveSitesTgz(const QByteArray &data) const;
	void findDevicesToSync(const QString &shv_path, QStringList &result);
	QStringList lsNode(const QString &shv_path);
	bool isDevice(const QString &shv_path);
	shv::chainpack::RpcValue metaValue(const QString &shv_path);

	void downloadSites();
	void onSitesDownloaded();
	void mergeSitesDirs(const QString &files_path, const QString &git_path);

	Q_SIGNAL void sitesDownloaded();

	shv::chainpack::RpcValue mkFile(const QString &shv_path, const shv::chainpack::RpcValue &params);
	QString nodeFilesPath(const QString &shv_path) const;
	QString nodeMetaPath(const QString &shv_path) const;
	bool isFile(const QString &shv_path);
	bool isDir(const QString &shv_path);

	QStringList lsDir(const QString &shv_path);
	shv::chainpack::RpcValue readConfig(const QString &filename);
	shv::chainpack::RpcValue readAndMergeConfig(QFile &file);

	QElapsedTimer m_sitesSyncedBefore;
	QTimer m_downloadSitesTimer;
	bool m_downloadingSites = false;
};



