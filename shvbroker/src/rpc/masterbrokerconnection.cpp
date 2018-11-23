#include "masterbrokerconnection.h"
#include "../brokerapp.h"

#include <shv/chainpack/rpcmessage.h>
#include <shv/core/string.h>
#include <shv/coreqt/log.h>

#define logRpcMsg() shvCDebug("RpcMsg")

namespace cp = shv::chainpack;

namespace rpc {

MasterBrokerConnection::MasterBrokerConnection(QObject *parent)
	: Super(parent)
{

}

void MasterBrokerConnection::setOptions(const shv::chainpack::RpcValue &slave_broker_options)
{
	Super::setOptions(slave_broker_options);
	m_exportedShvPath = slave_broker_options.toMap().value("exportedShvPath").toString();
}

void MasterBrokerConnection::sendRawData(const shv::chainpack::RpcValue::MetaData &meta_data, std::string &&data)
{
	logRpcMsg() << SND_LOG_ARROW
				<< "client id:" << connectionId()
				<< "protocol_type:" << (int)protocolType() << shv::chainpack::Rpc::protocolTypeToString(protocolType())
				<< BrokerApp::instance()->dataToCpon(shv::chainpack::RpcMessage::protocolType(meta_data), meta_data, data, 0);
	Super::sendRawData(meta_data, std::move(data));
}

void MasterBrokerConnection::sendMessage(const shv::chainpack::RpcMessage &rpc_msg)
{
	logRpcMsg() << SND_LOG_ARROW
				<< "client id:" << connectionId()
				<< "protocol_type:" << (int)protocolType() << shv::chainpack::Rpc::protocolTypeToString(protocolType())
				<< rpc_msg.toPrettyString();
	Super::sendMessage(rpc_msg);
}

void MasterBrokerConnection::addSubscription(const std::string &rel_path, const std::string &method)
{
	if(m_exportedShvPath.empty())
		CommonRpcClientHandle::addSubscription(rel_path, method);
	else
		CommonRpcClientHandle::addSubscription(m_exportedShvPath + '/' + rel_path, method);
}

std::string MasterBrokerConnection::toSubscribedPath(const CommonRpcClientHandle::Subscription &subs, const std::string &abs_path) const
{
	if(m_exportedShvPath.empty())
		return CommonRpcClientHandle::toSubscribedPath(subs, abs_path);
	else
		return abs_path.substr(m_exportedShvPath.size() + 1);
}

void MasterBrokerConnection::onRpcDataReceived(shv::chainpack::Rpc::ProtocolType protocol_type, shv::chainpack::RpcValue::MetaData &&md, const std::string &data, size_t start_pos, size_t data_len)
{
	logRpcMsg() << RpcDriver::RCV_LOG_ARROW
				<< "client id:" << connectionId()
				<< "protocol_type:" << (int)protocol_type << shv::chainpack::Rpc::protocolTypeToString(protocol_type)
				<< BrokerApp::instance()->dataToCpon(protocol_type, md, data, start_pos);
	try {
		if(isInitPhase()) {
			Super::onRpcDataReceived(protocol_type, std::move(md), data, start_pos, data_len);
			return;
		}
		if(cp::RpcMessage::isRequest(md)) {
			shv::core::String shv_path = cp::RpcMessage::shvPath(md).toString();
			if(!shv_path.startsWith(cp::Rpc::DIR_BROKER)) {
				if(!m_exportedShvPath.empty()) {
					shv_path = m_exportedShvPath + '/' + shv_path;
					cp::RpcMessage::setShvPath(md, shv_path);
				}
			}
		}
		else if(cp::RpcMessage::isResponse(md)) {
			if(cp::RpcMessage::requestId(md) == m_connectionState.pingRqId) {
				m_connectionState.pingRqId = 0;
				return;
			}
		}
		std::string msg_data(data, start_pos, data_len);
		BrokerApp::instance()->onRpcDataReceived(connectionId(), protocol_type, std::move(md), std::move(msg_data));
	}
	catch (std::exception &e) {
		shvError() << e.what();
	}
}

}
