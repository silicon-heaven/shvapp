#include "shvclientnode.h"
#include "brokerapp.h"

#include "rpc/serverconnection.h"

#include <shv/coreqt/log.h>
#include <shv/iotqt/rpc/tunnelhandle.h>

namespace cp = shv::chainpack;

ShvClientNode::ShvClientNode(rpc::ServerConnection *connection, ShvNode *parent)
	: Super(parent)
	, m_connection(connection)
{
	shvInfo() << "Creating client node:" << this << "connection:" << connection->connectionId();
}

ShvClientNode::~ShvClientNode()
{
	shvInfo() << "Destroying client node:" << this << "connection:" << m_connection->connectionId();
}

void ShvClientNode::processRawData(const shv::chainpack::RpcValue::MetaData &meta, std::string &&data)
{
	/*
	const cp::RpcValue tun_h = shv::chainpack::RpcMessage::tunnelHandle(meta);
	if(tun_h.isIMap()) {
		shv::iotqt::rpc::TunnelHandle th(tun_h.toIMap());
		using THMT = shv::iotqt::rpc::TunnelHandle::MetaType;
		const cp::RpcValue callers = th.value(THMT::Key::CallerClientIds);
		if(callers == cp::RpcMessage::callerIds(meta)) {
			unsigned tunnel_client_id = th.value(THMT::Key::TunnelClientId).toUInt();
			rpc::ServerConnection *conn = BrokerApp::instance()->clientById(tunnel_client_id);
			if(conn) {
				conn->sendRawData(meta, std::move(data));
			}
			else {
				shvError() << "Tunnel:" << th.toRpcValue().toPrettyString() << "doesn't exist.";
				return;
			}
		}
	}
	else {
		rpc::ServerConnection *conn = connection();
		conn->sendRawData(meta, std::move(data));
	}
	*/
	rpc::ServerConnection *conn = connection();
	if(cp::RpcMessage::isOpenTunnelFlag(meta)) {
		cp::RpcValue::MetaData m2(meta);
		cp::RpcMessage::pushRevCallerId(m2, conn->connectionId());
		conn->sendRawData(m2, std::move(data));
	}
	else {
		conn->sendRawData(meta, std::move(data));
	}
}

shv::chainpack::RpcValue ShvClientNode::hasChildren()
{
	//shvWarning() << "ShvClientNode::hasChildren path:" << shv_path << "ret:" << true;
	return nullptr;
}


