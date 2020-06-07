#include "telegram.h"
#include "hscopeapp.h"
#include "appclioptions.h"
#include "hnode.h"
#include "hnodetest.h"
#include "hnodeagent.h"

#include <shv/chainpack/rpcvalue.h>
#include <shv/coreqt/log.h>
#include <shv/core/utils/shvpath.h>
#include <shv/iotqt/node/shvnodetree.h>
#include <shv/iotqt/node/shvnode.h>
#include <shv/iotqt/rpc/rpc.h>

#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QVariantMap>
#include <QTimeZone>

#define logTelegram() shvCMessage("Telegram")
#define logTelegramE() shvCError("Telegram")

namespace cp = shv::chainpack;

static const char PEER_IDS[] = "peerIds";

static std::string format_json_for_log(const QJsonDocument &doc)
{
	return doc.toJson(HScopeApp::instance()->cliOptions()->isTelegramLogIndented()
			   ? QJsonDocument::Indented
			   : QJsonDocument::Compact).toStdString();
}

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

QTimeZone Telegram::peerTimeZone(int peer_id) const
{
	Q_UNUSED(peer_id)
	static QTimeZone tz("Europe/Prague");
	return tz;
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
	logTelegram() << call_cnt << "==>" << url << "params:" << format_json_for_log(doc);
	QNetworkRequest req(url);
	req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
	QNetworkReply *rpl = m_netManager->post(req, params_data);
	auto reply_handler = [rpl, call_cnt, method, call_back]() {
		QJsonDocument doc = QJsonDocument::fromJson(rpl->readAll());
		logTelegram() << call_cnt << "<==" << method << "response:" << format_json_for_log(doc);
		QJsonObject rv = doc.object();
		bool ok = rv.value(QStringLiteral("ok")).toBool();
		if(ok) {
			if(call_back)
				call_back(rv.value(QStringLiteral("result")));
		}
		else {
			logTelegramE() << "TG method call error, method:" << method;
		}
		rpl->deleteLater();
	};
	if(rpl->isFinished()) {
		logTelegramE() << "HTTP reply finished too early" << rpl->errorString();
		reply_handler();
	}
	else {
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
		connect(rpl, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::error), [](QNetworkReply::NetworkError code) {
#else
		connect(rpl, &QNetworkReply::errorOccurred, [](QNetworkReply::NetworkError code) {
#endif
			logTelegramE() << "TG getUpdates network reply error:" << code;
		});
		connect(rpl, &QNetworkReply::finished, this, reply_handler);
	}
}

void Telegram::getUpdates()
{
	if(!m_getUdatesTimer) {
		m_getUdatesTimer = new QTimer(this);
		connect(m_getUdatesTimer, &QTimer::timeout, [this]() {
			shvWarning() << "Get updates timeout after:" << (m_getUdatesTimer->interval() / 1000) << "secs.";
			m_getUdatesTimer->stop();
			QTimer::singleShot(0, this, &Telegram::getUpdates);
		});
	}
	if(m_getUdatesTimer->isActive())
		return;

	QVariantMap params;
	params["offset"] = m_getUpdateId + 1;
	int timeout = 600; // seconds
	params["timeout"] = timeout;
	m_getUdatesTimer->start((timeout + 1) * 1000);
	callTgApiMethod("getUpdates", params, [this](const QJsonValue &result) {
		processUpdates(result);
		m_getUdatesTimer->stop();
		QTimer::singleShot(0, this, &Telegram::getUpdates);
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
			if(text == QLatin1String("/restart")) {
				HScopeApp *app = HScopeApp::instance();
				sendMessage(peer_id, "Restarting service after 3 sec.");
				QTimer::singleShot(3000, app, &QCoreApplication::quit);
			}
			else if(text == QLatin1String("/ls")) {
				TelegramLsState ls_state(peerTimeZone(peer_id));
				QString shv_path = text.section(' ', 1).trimmed();
				SendMessage msg(ls_state.paramsForShvPath(shv_path));
				msg.set_chat_id(peer_id);
				msg.set_reply_to_message_id(message_id);
				sendMessage(msg);
			}
		}
		else if(row.contains("callback_query")) {
			QJsonObject callback_query = row.value("callback_query").toObject();
			const auto key_callback_query_id = QStringLiteral("callback_query_id");
			QString callback_query_id = callback_query.value(QStringLiteral("id")).toString();
			QJsonValue data = callback_query.value("data");
			QJsonObject from = callback_query.value("from").toObject();
			int peer_id = from.value("id").toInt();
			logTelegram() << "callback_query_id:" << callback_query_id << "data:" << data.toString();
			{
				QVariantMap params;
				params[key_callback_query_id] = callback_query_id;
				//params["text"] = "CB resp:" + data.toString(); /// do not show popup message
				callTgApiMethod("answerCallbackQuery", params, nullptr);
			}
			QString command = data.toString();
			auto ls_message = [this, callback_query, peer_id](const QString &shv_path) {
				TelegramLsState ls_state(peerTimeZone(peer_id));
				QVariantMap params = ls_state.paramsForShvPath(shv_path);
				QJsonObject message = callback_query.value("message").toObject();
				QJsonObject chat = message.value("chat").toObject();
				params["message_id"] = message.value("message_id").toInt();
				params["chat_id"] = chat.value("id").toInt();
				return params;
			};
			if(command.startsWith(QLatin1String("/ls "))) {
				QString shv_path = command.mid(4);
				while(shv_path.startsWith('/'))
					shv_path = shv_path.mid(1);
				callTgApiMethod("editMessageText", ls_message(shv_path), nullptr);
			}
			else if(command.startsWith(QLatin1String("/run "))) {
				QString shv_path = command.mid(5);
				while(shv_path.startsWith('/'))
					shv_path = shv_path.mid(1);
				HScopeApp *app = HScopeApp::instance();
				HNodeTest *nd = qobject_cast<HNodeTest*>(app->shvTree()->cd(shv_path.toStdString()));
				if(nd) {
					auto ctx = new QObject();
					connect(nd, &HNodeTest::runTestFinished, ctx, [this, ctx, shv_path, ls_message]() {
						callTgApiMethod("editMessageText", ls_message(shv_path), nullptr);
						ctx->deleteLater();
					});
					nd->runTestSafe();
				}
			}
			else if(command.startsWith(QLatin1String("/reload "))) {
				QString shv_path = command.mid(8);
				while(shv_path.startsWith('/'))
					shv_path = shv_path.mid(1);
				HScopeApp *app = HScopeApp::instance();
				HNodeTest *nd = qobject_cast<HNodeTest*>(app->shvTree()->cd(shv_path.toStdString()));
				if(nd) {
					nd->reload();
					callTgApiMethod("editMessageText", ls_message(shv_path), nullptr);
				}
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
	QString text = QStringLiteral("*[%1]* %2\n_%3_").arg(NodeStatus::levelToStringAbbr(status.level))
				  .arg(QString::fromStdString(status.message))
				  .arg(QString::fromStdString(shv_path)
				  );
	sendMessage(peer_id, text);
}

//================================================================
// LsState
//================================================================
TelegramLsState::TelegramLsState(const QTimeZone &time_zone)
	: m_timeZone(time_zone)
{
	//setObjectName(QString::number(chat_id));
	//m_autoDestroyTimer = new QTimer(this);
	//connect(m_autoDestroyTimer, &QTimer::timeout, this, &QObject::deleteLater);
	//m_autoDestroyTimer->start(60 * 60 * 1000);
}

QVariantMap TelegramLsState::paramsForShvPath(const QString &shv_path)
{
	//m_autoDestroyTimer->start();

	SendMessage ret;
	HScopeApp *app = HScopeApp::instance();
	shv::iotqt::node::ShvNode *nd = qobject_cast<shv::iotqt::node::ShvNode*>(app->shvTree()->cd(shv_path.toStdString()));
	HNode *hnd = qobject_cast<HNode*>(nd);
	//shv::iotqt::node::ShvNode *nd = app->shvTree()->cd("brokers");
	if(!nd) {
		ret.set_text("Invalid path: `" + shv_path + "`");
		return QVariantMap(ret);
	}

	static const auto key_text = QStringLiteral("text");
	static const auto key_callback_data = QStringLiteral("callback_data");

	QVariantList keyboard;
	for(const std::string &name : nd->childNames()) {
		if(name.empty())
			continue;
		if(name[0] == '.')
			continue;

		HNode *chnd = qobject_cast<HNode*>(nd->childNode(name, !shv::core::Exception::Throw));
		if(!chnd)
			continue;

		NodeStatus::Level st = chnd->overallStatus();
		const char *st_abbr = NodeStatus::levelToStringAbbr(st);
		//logTelegram() << chnd->shvPath() << st.toString();

		QVariantList row;
		row << QVariantMap {
					{key_text, QStringLiteral("[%1] %2").arg(st_abbr).arg(name.c_str())},
					{key_callback_data, "/ls " + shv_path + (shv_path.isEmpty()? "": "/") + name.c_str()},
				};
		keyboard.insert(keyboard.length(), row);
	}
	if(qobject_cast<HNodeTest*>(nd)) {
		QVariantList row;
		row << QVariantMap {
					{key_text, QStringLiteral("Run")},
					{key_callback_data, "/run " + shv_path},
				};
		keyboard.insert(keyboard.length(), row);
	}
	if(!shv_path.isEmpty()) {
		QString parent_shv_path = shv_path.section('/', 0, -2);
		QVariantList row;
		row << QVariantMap {
					{key_text, QStringLiteral("..")},
					{key_callback_data, "/ls " + parent_shv_path},
				};
		row << QVariantMap {
					{key_text, QStringLiteral("/")},
					{key_callback_data, "/ls "},
				};
		row << QVariantMap {
					{key_text, QStringLiteral("reload")},
					{key_callback_data, "/reload " + shv_path},
				};
		keyboard.insert(keyboard.length(), row);
	}
	const auto key_inline_keyboard = QStringLiteral("inline_keyboard");
	QVariantMap reply_markup {
		{key_inline_keyboard, keyboard}
	};
	//QJsonDocument doc(QJsonObject::fromVariantMap(reply_markup));
	//ret.set_reply_markup(QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));
	ret.set_reply_markup(reply_markup);
	ret.set_parse_mode(QStringLiteral("Markdown"));

	if(hnd) {
		const NodeStatus node_status = hnd->status();
		if(HNodeTest *ndtst = qobject_cast<HNodeTest*>(hnd)) {
			shv::chainpack::RpcValue rec_run = ndtst->recentRun();
			ret.set_text(tr("Status of test: _%1:_ [%2] %3\nrecent run: %4\nnext run: %5")
						 .arg(shv_path)
						 .arg(NodeStatus::levelToStringAbbr(node_status.level))
						 .arg(node_status.message.c_str())
						 .arg(formatDateTime(ndtst->recentRun()))
						 .arg(formatDateTime(ndtst->nextRun()))
						 );
		}
		else {
			if(node_status.level == NodeStatus::Level::Unknown) {
				ret.set_text(tr("Node: _%1:_")
							 .arg(shv_path)
							 );
			}
			else {
				ret.set_text(tr("Status of node: _%1:_ [%2] %3")
							 .arg(shv_path)
							 .arg(NodeStatus::levelToStringAbbr(node_status.level))
							 .arg(node_status.message.c_str())
							 );
			}
		}
	}
	else {
		ret.set_text(tr("Holy Root"));
	}
	return QVariantMap(ret);
}

QString TelegramLsState::formatDateTime(const shv::chainpack::RpcValue &rpcdt)
{
	if(rpcdt.isDateTime()) {
		shv::chainpack::RpcValue::DateTime rt = rpcdt.toDateTime();
		QDateTime qdt = QDateTime::fromMSecsSinceEpoch(rt.msecsSinceEpoch(), m_timeZone);
		rt.setUtcOffsetMin(qdt.offsetFromUtc() / 60);
		return QString::fromStdString(rt.toIsoString(cp::RpcValue::DateTime::MsecPolicy::Never, true));
	}
	else {
		return QString();
	}
}
