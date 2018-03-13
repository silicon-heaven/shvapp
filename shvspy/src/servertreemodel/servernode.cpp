#include "servernode.h"

#include <shv/iotqt/rpc/clientconnection.h>

#include <shv/core/utils.h>

#include <QApplication>

ServerNode::ServerNode(const std::string &server_name)
	: Super(server_name)
{

}

ServerNode::~ServerNode()
{
	SHV_SAFE_DELETE(m_clientConnection);
}

QVariant ServerNode::data(int role) const
{
	QVariant ret;
	if(role == Qt::DisplayRole) {
		ret = QString::fromStdString(nodeId());
		//if(m_clientConnection) {
		//	ret = m_clientConnection->serverName();
		//}
	}
	else if(role == Qt::DecorationRole) {
		static QIcon ico_connected = QIcon(QStringLiteral(":/shvspy/images/connected.png"));
		static QIcon ico_connecting = QIcon(QStringLiteral(":/shvspy/images/connecting.png"));
		static QIcon ico_disconnected = QIcon(QStringLiteral(":/shvspy/images/disconnected.png"));
		switch (openStatus()) {
		case OpenStatus::Connected: return ico_connected;
		case OpenStatus::Connecting: return ico_connecting;
		case OpenStatus::Disconnected: return ico_disconnected;
		default: return QIcon();
		}
	}
	else
		ret = Super::data(role);
	return ret;
}

QVariantMap ServerNode::serverProperties() const
{
	QVariantMap ret;
	if(m_clientConnection) {
		ret["oid"] = oid();
		ret["name"] = QString::fromStdString(nodeId());
		ret["host"] = QString::fromStdString(m_clientConnection->host());
		ret["port"] = m_clientConnection->port();
		ret["user"] = QString::fromStdString(m_clientConnection->user());
		ret["password"] = QString::fromStdString(m_clientConnection->password());
	}
	return ret;
}

void ServerNode::setServerProperties(const QVariantMap &props)
{
	setOid(props.value("oid").toInt());
	shv::iotqt::rpc::ClientConnection *cli = clientConnection();
	//cli->setServerName(props.value("name").toString());
	cli->setHost(props.value("host").toString().toStdString());
	cli->setPort(props.value("port").toInt());
	cli->setUser(props.value("user").toString().toStdString());
	cli->setPassword(props.value("password").toString().toStdString());
	emitDataChanged();
}

void ServerNode::open()
{
	close();
	clientConnection()->open();
	m_openStatus = OpenStatus::Connecting;
	emitDataChanged();
}

void ServerNode::close()
{
	if(openStatus() == OpenStatus::Disconnected)
		return;
	if(m_clientConnection)
		m_clientConnection->close();
	m_openStatus = OpenStatus::Disconnected;
	emitDataChanged();
}
/*
QString ServerNode::connectionErrorString()
{
	QString ret;
	if(m_clientConnection)
		ret = m_clientConnection->errorString();
	return ret;
}
*/
shv::iotqt::rpc::ClientConnection *ServerNode::clientConnection()
{
	if(!m_clientConnection) {
		m_clientConnection = new shv::iotqt::rpc::ClientConnection(nullptr);
		m_clientConnection->setCheckBrokerConnectedInterval(0);
		connect(m_clientConnection, &shv::iotqt::rpc::ClientConnection::brokerConnectedChanged, this, [this](bool is_connected) {
			m_openStatus = is_connected? OpenStatus::Connected: OpenStatus::Disconnected;
			emitDataChanged();
		});
	}
	return m_clientConnection;
}


