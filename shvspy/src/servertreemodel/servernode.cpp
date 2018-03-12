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
		static QIcon connected = QIcon(QStringLiteral(":/qfopcuaspy/images/connected.png"));
		static QIcon disconnected = QIcon(QStringLiteral(":/qfopcuaspy/images/disconnected.png"));
		ret = isConnected()? connected: disconnected;
	}
	else
		ret = Super::data(role);
	return ret;
}

bool ServerNode::isConnected() const
{
	return m_clientConnection && m_clientConnection->isBrokerConnected();
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

bool ServerNode::setConnected(bool b)
{
	if(b == isConnected())
		return true;
	bool ret = true;
	if(b) {
		clientConnection()->open();
		if(ret) {
			//m_nodeId = clientConnection()->getRootNode();
			//loadChildren(true);
		}
	}
	else {
		m_nodeId.clear();
		clientConnection()->close();
		removeRows(0, rowCount());
	}
	emitDataChanged();
	return ret;
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
		//m_clientConnection->setDebug(QApplication::instance()->arguments().contains(QStringLiteral("--opcua-debug")));
	}
	return m_clientConnection;
}


