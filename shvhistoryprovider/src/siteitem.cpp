#include "application.h"
#include "appclioptions.h"
#include "siteitem.h"

#include <shv/coreqt/exception.h>
#include <shv/coreqt/log.h>

namespace cp = shv::chainpack;

SiteItem::SiteItem(QObject *parent)
	: QObject(parent)
{
}

void SiteItem::parseRpcValue(const cp::RpcValue &value)
{
	if (!value.isMap()) {
		SHV_QT_EXCEPTION("Bad chainpack value type in siteitem");
	}
	parseRpcValue(value.asMap());
}

void SiteItem::parseRpcValue(const cp::RpcValue::Map &map)
{
	for (auto it = map.cbegin(); it != map.cend(); ++it) {
		if (it->first == "_meta") {
			const cp::RpcValue &meta = it->second;
			if (meta.isValid()) {
				if (!meta.isMap()) {
					SHV_EXCEPTION("Bad value type in siteitem meta");
				}
				parseMetaRpcValue(meta.asMap());
			}
		}
		else {
			const cp::RpcValue &child = it->second;
			if (!child.isMap()) {
				shvWarning() << it->first << cp::RpcValue::typeToName(child.type());
				SHV_EXCEPTION("Bad value type in siteitem child");
			}
			SiteItem *c;
			if (child.at("_meta").at("type").isValid()) {
				if (child.at("_meta").at("HP").isValid()) {
					c = new SitesHPDevice(this);
				}
				else {
					c = new SitesDevice(this);
				}
			}
			else {
				c = new SiteItem(this);
			}

			c->setObjectName(QString::fromStdString(it->first));
			if (qobject_cast<SitesDevice*>(c) &&
				!c->sitePath().startsWith(QString::fromStdString(Application::instance()->cliOptions()->sitesRootPath()))) {
				delete c;
			}
			else {
				c->parseRpcValue(child);
				if (!qobject_cast<SitesHPDevice*>(c) && c->findChildren<SitesHPDevice*>().count() == 0) {
					delete c;
				}
			}
		}
	}
}

void SiteItem::parseMetaRpcValue(const shv::chainpack::RpcValue::Map &meta)
{
	Q_UNUSED(meta);
}

const SiteItem *SiteItem::itemBySitePath(const QString &site_path) const
{
	if (site_path.isEmpty()) {
		return this;
	}
	return itemBySitePath(site_path, 0);
}

const SiteItem *SiteItem::itemBySitePath(const QString &site_path, qsizetype offset) const
{
	qsizetype slash = site_path.indexOf('/', offset);
	QString part = site_path.mid(offset, slash - offset);

	SiteItem *item = findChild<SiteItem *>(part, Qt::FindDirectChildrenOnly);
	if (!item || slash == -1 || slash == site_path.length() - 1) {
		return item;
	}
	return item->itemBySitePath(site_path, slash + 1);
}

QString SiteItem::sitePath() const
{
	QStringList result;
	const SiteItem *item = this;
	while (true) {
		SiteItem *parent = qobject_cast<SiteItem*>(item->parent());
		if (!parent) {
			break;
		}
		result.prepend(item->objectName());
		item = parent;
	}
	return result.join('/');
}

void SitesHPDevice::parseMetaRpcValue(const shv::chainpack::RpcValue::Map &meta)
{
	shv::chainpack::RpcValue::Map hp_meta = meta.at("HP").asMap();
	if (!hp_meta.hasKey("elesys")) {
		m_elesys = false;
	}
	else {
		m_elesys = hp_meta.at("elesys").toBool();
	}
	if (!hp_meta.hasKey("pushLog")) {
		m_pushLog = false;
	}
	else {
		m_pushLog = hp_meta.at("pushLog").toBool();
	}
	if (hp_meta.hasKey("syncLog")) {
		for (auto sync_log : hp_meta.at("syncLog").asList()) {
			if (QString::fromStdString(sync_log.at("target").toString()) == Application::instance()->brokerId()) {
				m_syncLogSource = QString::fromStdString(sync_log.at("source").toString());
			}
		}
	}
}
