#ifndef HBROKERNODE_H
#define HBROKERNODE_H

#include "hnode.h"

namespace shv { namespace iotqt { namespace rpc { class ClientConnection; }}}

class HNodeBroker : public HNode
{
	Q_OBJECT

	using Super = HNode;
public:
	HNodeBroker(const std::string &node_id, HNode *parent);
public:
	void load() override;

	std::string templateFileName() override;
	shv::iotqt::rpc::ClientConnection *rpcConnection();
private:
	void reconnect();
private:
	shv::iotqt::rpc::ClientConnection *m_rpcConnection = nullptr;
};

#endif // HBROKERNODE_H
