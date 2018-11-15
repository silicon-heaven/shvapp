#ifndef MASTERBROKERCONNECTION_H
#define MASTERBROKERCONNECTION_H

#include "commonrpcclienthandle.h"

#include <shv/iotqt/rpc/brokerconnection.h>

namespace rpc {

class MasterBrokerConnection : public shv::iotqt::rpc::BrokerConnection, public CommonRpcClientHandle
{
	Q_OBJECT

	using Super = shv::iotqt::rpc::BrokerConnection;
public:
	MasterBrokerConnection(QObject *parent = nullptr);

	int connectionId() const override {return Super::connectionId();}

	bool isConnectedAndLoggedIn() const override {return isSocketConnected() && !isInitPhase();}

	void setOptions(const shv::chainpack::RpcValue &slave_broker_options) override;

	void sendRawData(const shv::chainpack::RpcValue::MetaData &meta_data, std::string &&data) override;
	void sendMessage(const shv::chainpack::RpcMessage &rpc_msg) override;
protected:
	void onRpcDataReceived(shv::chainpack::Rpc::ProtocolType protocol_type, shv::chainpack::RpcValue::MetaData &&md, const std::string &data, size_t start_pos, size_t data_len) override;
protected:
	std::string m_exportedShvPath;
};

}

#endif // MASTERBROKERCONNECTION_H
