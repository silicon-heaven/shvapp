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

#define logTelegram() shvCDebug("Telegram")

namespace cp = shv::chainpack;

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

QString Telegram::apiToken()
{
	return QString::fromStdString(m_configNode->valueOnPath("apiToken", !shv::core::Exception::Throw).toString());
}

void Telegram::getUpdates()
{
	if(m_getUpdateReply)
		return;

	QString api_token = apiToken();
	if(api_token.isEmpty())
		return;

	QString url = QStringLiteral("https://api.telegram.org/bot%1/getUpdates").arg(api_token);
	cp::RpcValue::Map params;
	params["offset"] = m_getUpdateId + 1;
	int timeout = 600; // seconds
	params["timeout"] = timeout;
	std::string params_str = cp::RpcValue(params).toCpon();
	logTelegram() << "getUpdates" << params_str;
	QByteArray params_data(params_str.data(), params_str.size());
	QNetworkRequest req(url);
	req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
	m_getUpdateReply = m_netManager->post(req, params_data);
	connect(m_getUpdateReply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::error), this, [this](QNetworkReply::NetworkError code){
		shvError() << "TG getUpdates network reply error:" << code;
		m_getUpdateReply->deleteLater();
		m_getUpdateReply = nullptr;
		QTimer::singleShot(0, this, &Telegram::getUpdates);
	});
	connect(m_getUpdateReply, &QNetworkReply::finished, this, [this]() {
		processUpdate(m_getUpdateReply->readAll());
		m_getUpdateReply->deleteLater();
		m_getUpdateReply = nullptr;
		QTimer::singleShot(0, this, &Telegram::getUpdates);
	});
	QTimer::singleShot((timeout + 1) * 1000, m_getUpdateReply, [this]() {
		shvError() << "TG getUpdates network reply timeout!";
		m_getUpdateReply->deleteLater();
		m_getUpdateReply = nullptr;
		QTimer::singleShot(0, this, &Telegram::getUpdates);
	});
}

static const char PEER_IDS[] = "peerIds";

void Telegram::processUpdate(const QByteArray data)
{
	QJsonDocument doc = QJsonDocument::fromJson(data);
	QJsonObject rv = doc.object();
	bool ok = rv.value("ok").toBool();
	logTelegram() << "getUpdates data OK:" << ok << data.toStdString();//rv.toPrettyString("\t");
	if(ok) {
		QJsonArray result = rv.value("result").toArray();
		for (int i = 0; i < result.count(); ++i) {
			QJsonObject row = result.at(i).toObject();
			QJsonObject message = row.value("message").toObject();
			QString text = message.value("text").toString();
			QJsonObject from = message.value("from").toObject();
			int peer_id = from.value("id").toInt();
			logTelegram() << "\t from:" << from.value("username").toString() << "id:" << peer_id << "text:" << text;
			m_getUpdateId = row.value("update_id").toInt();
			const shv::chainpack::RpcValue::List &peer_ids = m_configNode->valueOnPath(PEER_IDS).toList();
			for(const cp::RpcValue &id : peer_ids) {
				if(id == peer_id) {
					peer_id = 0;
					break;
				}
			}
			if(peer_id > 0) {
				logTelegram() << "new peer id:" << peer_id;
				shv::chainpack::RpcValue::List lst = peer_ids;
				lst.push_back(peer_id);
				m_configNode->setValueOnPath(PEER_IDS, lst);
				m_configNode->commitChanges();
			}
		}
	}
}

void Telegram::onAlertStatusChanged(const std::string &shv_path, const NodeStatus &status)
{
	QString api_token = apiToken();
	if(api_token.isEmpty())
		return;

	QString url = QStringLiteral("https://api.telegram.org/bot%1/sendMessage").arg(api_token);
	const shv::chainpack::RpcValue::List &peer_ids = m_configNode->valueOnPath(PEER_IDS).toList();
	for(const cp::RpcValue &id : peer_ids) {
		QJsonObject params;
		params.insert("chat_id", id.toInt());
		params.insert("parse_mode", "Markdown");
		QString text = "*[%1]* %2\n_%3_";
		params.insert("text", text
					  .arg(NodeStatus::valueToStringAbbr(status.value))
					  .arg(QString::fromStdString(status.message))
					  .arg(QString::fromStdString(shv_path))
					  );
		QJsonDocument doc(params);
		QByteArray params_data = doc.toJson(QJsonDocument::Compact);
		QNetworkRequest req(url);
		req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
		QNetworkReply *rpl = m_netManager->post(req, params_data);
		connect(rpl, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::error), [rpl](QNetworkReply::NetworkError code){
			shvError() << "TG sendMessage network reply error:" << code;
			rpl->deleteLater();
		});
		connect(rpl, &QNetworkReply::finished, [rpl]() {
			QByteArray data = rpl->readAll();
			logTelegram() << "sendMessage data reply:" << data.toStdString();
			rpl->deleteLater();
		});
	}
}

