#ifndef TELEGRAM_H
#define TELEGRAM_H

#include <shv/iotqt/node/shvnode.h>

#include <QObject>

class QNetworkAccessManager;
class QNetworkReply;
class NodeStatus;

namespace shv { namespace iotqt { namespace node { class RpcValueConfigNode; } } }

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

	void processUpdate(const QByteArray data);
private:
	QNetworkAccessManager *m_netManager = nullptr;
	int m_getUpdateId = 0;
	QNetworkReply *m_getUpdateReply = nullptr;
	shv::iotqt::node::RpcValueConfigNode *m_configNode = nullptr;
};

#endif // TELEGRAM_H
