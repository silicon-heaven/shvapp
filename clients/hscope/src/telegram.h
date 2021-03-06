#ifndef TELEGRAM_H
#define TELEGRAM_H

#include <shv/iotqt/node/shvnode.h>
#include <shv/coreqt/utils.h>

#include <QObject>
#include <QCoreApplication>
#include <QTimeZone>

class QNetworkAccessManager;
class QNetworkReply;
class QTimer;

class NodeStatus;

namespace shv { namespace iotqt { namespace node { class RpcValueConfigNode; } } }

// see https://core.telegram.org/bots/api#making-requests
class SendMessage : public QVariantMap
{
	SHV_VARIANTMAP_FIELD(int, c, set_c, hat_id)
	SHV_VARIANTMAP_FIELD(QString, t, set_t, ext)
	SHV_VARIANTMAP_FIELD(QString, p, set_p, arse_mode) //  Markdown or HTML
	SHV_VARIANTMAP_FIELD(bool, d, set_d, isable_web_page_preview)
	SHV_VARIANTMAP_FIELD(bool, d, set_d, isable_notification)
	SHV_VARIANTMAP_FIELD(int, r, set_r, eply_to_message_id)
	//SHV_VARIANTMAP_FIELD(QString, r, set_r, eply_markup)
	SHV_VARIANTMAP_FIELD(QVariantMap, r, set_r, eply_markup)

public:
	SendMessage() : QVariantMap() {}
	SendMessage(const QVariantMap &o) : QVariantMap(o) {}
};

class Telegram : public shv::iotqt::node::ShvNode
{
	Q_OBJECT
	using Super = shv::iotqt::node::ShvNode;
public:
	explicit Telegram(Super *parent = nullptr);
public:
	void start();

	void onAlertStatusChanged(const std::string &shv_path, const NodeStatus &status);
private:
	QString apiToken();

	QTimeZone peerTimeZone(int peer_id) const;

	void getUpdates();
	void processUpdates(const QJsonValue &response);

	void sendMessage(int chat_id, const QString &text);
	void sendMessage(const QVariantMap &msg);

	using TgApiMethodCallBack = std::function<void (const QJsonValue &result)>;
	void callTgApiMethod(QString method, const QVariantMap &params, TgApiMethodCallBack call_back);

	void sendNodeStatus(int peer_id, const NodeStatus &status, const std::string &shv_path);
private:
	QNetworkAccessManager *m_netManager = nullptr;
	int m_getUpdateId = 0;
	QTimer *m_getUdatesTimer = nullptr;
	shv::iotqt::node::RpcValueConfigNode *m_configNode = nullptr;
};

class TelegramLsState
{
	Q_DECLARE_TR_FUNCTIONS(TelegramLsState)
public:
	explicit TelegramLsState(const QTimeZone &time_zone);

	QVariantMap paramsForShvPath(const QString &shv_path);
private:
	QString formatDateTime(const shv::chainpack::RpcValue &rpcdt);
private:
	QTimeZone m_timeZone;
};

#endif // TELEGRAM_H
