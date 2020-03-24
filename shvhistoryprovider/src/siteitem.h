#ifndef SITEITEM_H
#define SITEITEM_H

#include <shv/chainpack/rpcvalue.h>

#include <QObject>

class SiteItem : public QObject
{
	Q_OBJECT

public:
	SiteItem(QObject *parent = nullptr);

	const SiteItem *itemByShvPath(const QString &shv_path) const;
	QString shvPath() const;

	void parseRpcValue(const shv::chainpack::RpcValue &value);

protected:
	virtual void parseRpcValue(const shv::chainpack::RpcValue::Map &map);
	virtual void parseMetaRpcValue(const shv::chainpack::RpcValue::Map &meta);

private:
	const SiteItem *itemByShvPath(const QString &shv_path, int offset) const;
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

	bool elesys() const	{ return m_elesys; }

protected:
	void parseMetaRpcValue(const shv::chainpack::RpcValue::Map &meta) override;

private:
	bool m_elesys;
};

#endif // SITEITEM_H
