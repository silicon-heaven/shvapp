#ifndef TELEGRAM_H
#define TELEGRAM_H

#include <shv/iotqt/node/shvnode.h>
#include <shv/coreqt/utils.h>

#include <QObject>

class QNetworkAccessManager;
class QNetworkReply;
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
	SHV_VARIANTMAP_FIELD(QVariantMap, r, set_r, eply_markup)
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
	bool m_isGetUpdateRunning = false;
	//QNetworkReply *m_getUpdateReply = nullptr;
	shv::iotqt::node::RpcValueConfigNode *m_configNode = nullptr;
};

#endif // TELEGRAM_H
