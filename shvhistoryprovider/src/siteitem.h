#ifndef SITEITEM_H
#define SITEITEM_H

#include <shv/chainpack/rpcvalue.h>

#include <QObject>

class SiteItem : public QObject
{
	Q_OBJECT

public:
	SiteItem(QObject *parent = nullptr);

	const SiteItem *itemBySitePath(const QString &site_path) const;
	QString sitePath() const;

	void parseRpcValue(const shv::chainpack::RpcValue &value);

protected:
	virtual void parseRpcValue(const shv::chainpack::RpcValue::Map &map);
	virtual void parseMetaRpcValue(const shv::chainpack::RpcValue::Map &meta);

private:
	const SiteItem *itemBySitePath(const QString &site_path, int offset) const;
};

class SitesDevice : public SiteItem
{
	Q_OBJECT

public:
	SitesDevice(QObject *parent) : SiteItem(parent)	{}
};

class SitesHPDevice : public SitesDevice
{
	Q_OBJECT

public:
	SitesHPDevice(QObject *parent) : SitesDevice(parent), m_elesys(false) {}

	bool isElesys() const	{ return m_elesys; }
	bool isPushLog() const { return m_pushLog; }
	const QString &syncLogSource() const { return m_syncLogSource; }

protected:
	void parseMetaRpcValue(const shv::chainpack::RpcValue::Map &meta) override;

private:
	bool m_elesys;
	bool m_pushLog;
	QString m_syncLogSource;
};

#endif // SITEITEM_H
