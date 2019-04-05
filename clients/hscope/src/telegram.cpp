#include "telegram.h"
#include "hscopeapp.h"
#include "appclioptions.h"
#include "hnode.h"

#include <shv/chainpack/rpcvalue.h>
#include <shv/coreqt/log.h>

#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QVariantMap>

#define logTelegram() shvCDebug("Telegram")

namespace cp = shv::chainpack;

static const char PEER_IDS[] = "peerIds";

Telegram::Telegram(Super *parent)
	: Super("telegram", parent)
{
	m_netManager = new QNetworkAccessManager(this);
	m_configNode = new shv::iotqt::node::RpcValueConfigNode("config", this);

	HScopeApp *app = HScopeApp::instance();
	m_configNode->setConfigDir(app->cliOptions()->configDir());
	m_configNode->setConfigName("telegram.config");
}

void Telegram::start()
{
	shvInfo() << "Starting Telegram client";
	connect(HScopeApp::instance(), &HScopeApp::alertStatusChanged, this, &Telegram::onAlertStatusChanged, Qt::UniqueConnection);
	getUpdates();
}

void Telegram::onAlertStatusChanged(const std::string &shv_path, const NodeStatus &status)
{
	QString api_token = apiToken();
	if(api_token.isEmpty())
		return;

	QString url = QStringLiteral("https://api.telegram.org/bot%1/sendMessage").arg(api_token);
	const shv::chainpack::RpcValue::List &peer_ids = m_configNode->valueOnPath(PEER_IDS).toList();
	for(const cp::RpcValue &id : peer_ids) {
		sendNodeStatus(id.toInt(), status, shv_path);
	}
}

QString Telegram::apiToken()
{
	return QString::fromStdString(m_configNode->valueOnPath("apiToken", !shv::core::Exception::Throw).toString());
}


void Telegram::callTgApiMethod(QString method, const QVariantMap &_params, Telegram::TgApiMethodCallBack call_back)
{
	QString api_token = apiToken();
	if(api_token.isEmpty())
		return;

	static int s_call_cnt = 0;
	int call_cnt = ++s_call_cnt;

	QString url = QStringLiteral("https://api.telegram.org/bot%1/%2").arg(api_token).arg(method);
	QJsonObject params = QJsonObject::fromVariantMap(_params);
	QJsonDocument doc(params);
	QByteArray params_data = doc.toJson(QJsonDocument::Indented);
	logTelegram() << call_cnt << "==>" << method << "params:" << params_data.toStdString();
	QNetworkRequest req(url);
	req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
	QNetworkReply *rpl = m_netManager->post(req, params_data);
	connect(rpl, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::error), [](QNetworkReply::NetworkError code) {
		shvError() << "TG getUpdates network reply error:" << code;
	});
	connect(rpl, &QNetworkReply::finished, this, [this, rpl, call_cnt, method, call_back]() {
		QJsonDocument doc = QJsonDocument::fromJson(rpl->readAll());
		logTelegram() << call_cnt << "<==" << method << "response:" << doc.toJson(QJsonDocument::Indented).toStdString();//rv.toPrettyString("\t");
		QJsonObject rv = doc.object();
		bool ok = rv.value(QStringLiteral("ok")).toBool();
		if(ok) {
			if(call_back)
				call_back(rv.value(QStringLiteral("result")));
		}
		else {
			shvError() << "TG method call error, method:" << method;
		}
		rpl->deleteLater();
		QTimer::singleShot(0, this, &Telegram::getUpdates);
	});
	/*
	QTimer::singleShot((timeout + 1) * 1000, rpl, [this, rpl]() {
		shvError() << "TG getUpdates network reply timeout!";
		rpl->deleteLater();
		QTimer::singleShot(0, this, &Telegram::getUpdates);
	});
	*/
}

void Telegram::getUpdates()
{
	if(m_isGetUpdateRunning)
		return;
	m_isGetUpdateRunning = true;

	QVariantMap params;
	params["offset"] = m_getUpdateId + 1;
	int timeout = 600; // seconds
	params["timeout"] = timeout;
	callTgApiMethod("getUpdates", params, [this](const QJsonValue &result) {
		processUpdates(result);
		m_isGetUpdateRunning = false;
	});
}

void Telegram::processUpdates(const QJsonValue &response)
{
	QJsonArray result = response.toArray();
	for (int i = 0; i < result.count(); ++i) {
		QJsonObject row = result.at(i).toObject();
		m_getUpdateId = row.value("update_id").toInt();
		if(row.contains("message")) {
			QJsonObject message = row.value("message").toObject();
			int message_id = message.value("message_id").toInt();
			QString text = message.value("text").toString();
			QJsonObject from = message.value("from").toObject();
			int peer_id = from.value("id").toInt();
			logTelegram() << "\t from:" << from.value("username").toString() << "id:" << peer_id << "text:" << text;
			const shv::chainpack::RpcValue::List &peer_ids = m_configNode->valueOnPath(PEER_IDS).toList();
			int new_peer_id = peer_id;
			for(const cp::RpcValue &id : peer_ids) {
				if(id == peer_id) {
					new_peer_id = 0;
					break;
				}
			}
			if(new_peer_id > 0) {
				logTelegram() << "new peer id:" << peer_id;
				shv::chainpack::RpcValue::List lst = peer_ids;
				lst.push_back(peer_id);
				m_configNode->setValueOnPath(PEER_IDS, lst);
				m_configNode->commitChanges();
			}
			if(text == QLatin1String("/status")) {
				HScopeApp *app = HScopeApp::instance();
				NodeStatus st = app->overallNodesStatus();
				sendNodeStatus(peer_id, st, "brokers");
			}
			else if(text == QLatin1String("/ls")) {
				//QVariantMap inline_keyboard;
				//QVariantList keyboard;
				//QVariantMap key;
				const auto key_text = QStringLiteral("text");
				const auto key_callback_data = QStringLiteral("callback_data");
				QVariantList keyboard = {
					QVariantList {
						QVariantMap {
							{key_text, "A"},
							{key_callback_data, "dataA"},
						},
						QVariantMap {
							{key_text, "B"},
							{key_callback_data, "dataB"},
						},
					},
					QVariantList {
						QVariantMap {
							{key_text, "Enter"},
							{key_callback_data, "dataE"},
						},
					},
				};
				const auto key_inline_keyboard = QStringLiteral("inline_keyboard");
				QVariantMap reply_markup {
					{key_inline_keyboard, keyboard}
				};
				SendMessage msg;
				msg.set_chat_id(peer_id);
				msg.set_text(QStringLiteral("List of: %1").arg("TBD"));
				msg.set_parse_mode(QStringLiteral("Markdown"));
				msg.set_reply_to_message_id(message_id);
				msg.set_reply_markup(reply_markup);
				sendMessage(msg);
			}
		}
		else if(row.contains("callback_query")) {
			QJsonObject callback_query = row.value("callback_query").toObject();
			const auto key_callback_query_id = QStringLiteral("callback_query_id");
			QString callback_query_id = callback_query.value(QStringLiteral("id")).toString();
			QJsonValue data = callback_query.value("data");
			logTelegram() << "callback_query_id:" << callback_query_id << "data:" << data.toString();
			{
				QVariantMap params;
				params[key_callback_query_id] = callback_query_id;
				params["text"] = "CB resp:" + data.toString();
				callTgApiMethod("answerCallbackQuery", params, nullptr);
			}
			{
				QJsonObject message = callback_query.value("message").toObject();
				QJsonObject chat = message.value("chat").toObject();
				SendMessage params;
				params["message_id"] = message.value("message_id").toInt();
				params["chat_id"] = chat.value("id").toInt();
				params["text"] = "CB resp:" + data.toString();
				callTgApiMethod("editMessageText", params, nullptr);
			}
		}
	}
}

void Telegram::sendMessage(int chat_id, const QString &text)
{
	SendMessage msg;
	msg.set_chat_id(chat_id);
	msg.set_text(text);
	msg.set_parse_mode(QStringLiteral("Markdown"));
	sendMessage(msg);
}

void Telegram::sendMessage(const QVariantMap &msg)
{
	callTgApiMethod("sendMessage", msg, nullptr);
}

void Telegram::sendNodeStatus(int peer_id, const NodeStatus &status, const std::string &shv_path)
{
	QString text = QStringLiteral("*[%1]* %2\n_%3_").arg(NodeStatus::valueToStringAbbr(status.value))
				  .arg(QString::fromStdString(status.message))
				  .arg(QString::fromStdString(shv_path)
				  );
	sendMessage(peer_id, text);
}

