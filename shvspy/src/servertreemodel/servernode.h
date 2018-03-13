#ifndef SERVERNODE_H
#define SERVERNODE_H

#include "shvnodeitem.h"

namespace shv { namespace iotqt { namespace rpc { class ClientConnection; } } }

class ServerNode : public ShvNodeItem
{
private:
	typedef ShvNodeItem Super;
public:
	enum class OpenStatus {Invalid = 0, Disconnected, Connecting, Connected};
public:
	explicit ServerNode(const std::string &server_name);
	~ServerNode() Q_DECL_OVERRIDE;

	QVariant data(int role = Qt::UserRole + 1) const Q_DECL_OVERRIDE;

	int oid() const {return m_oid;}
	void setOid(int i) {m_oid = i;}

	OpenStatus openStatus() const {return m_openStatus;}
	void open();
	void close();
	//QString connectionErrorString();

	QVariantMap serverProperties() const;
	void setServerProperties(const QVariantMap &props);

	shv::iotqt::rpc::ClientConnection *clientConnection();
	//void loadChildren() Q_DECL_OVERRIDE;
private:
	int m_oid = 0;
	std::string m_serverName;
	shv::iotqt::rpc::ClientConnection *m_clientConnection = nullptr;
	OpenStatus m_openStatus = OpenStatus::Disconnected;
};

#endif // SERVERNODE_H
