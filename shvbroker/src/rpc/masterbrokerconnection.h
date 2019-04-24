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

	// master broker connection cannot have userName
	// because it is connected in oposite direction than client connections
	std::string loggedUserName() override {return std::string();}

	bool isConnectedAndLoggedIn() const override {return isSocketConnected() && !isInitPhase();}
	bool isSlaveBrokerConnection() const override {return false;}
	bool isMasterBrokerConnection() const override {return true;}

	void sendRawData(const shv::chainpack::RpcValue::MetaData &meta_data, std::string &&data) override;
	void sendMessage(const shv::chainpack::RpcMessage &rpc_msg) override;

	unsigned addSubscription(const std::string &rel_path, const std::string &method) override;
	bool removeSubscription(const std::string &rel_path, const std::string &method) override;
	std::string toSubscribedPath(const Subscription &subs, const std::string &abs_path) const override;

	std::string masterExportedToLocalPath(const std::string &master_path) const;
	std::string localPathToMasterExported(const std::string &local_path) const;
	const std::string& exportedShvPath() const {return m_exportedShvPath;}

	void setOptions(const shv::chainpack::RpcValue &slave_broker_options) override;
	shv::chainpack::RpcValue options() {return m_options;}
protected:
	void onRpcDataReceived(shv::chainpack::Rpc::ProtocolType protocol_type, shv::chainpack::RpcValue::MetaData &&md, const std::string &data, size_t start_pos, size_t data_len) override;
protected:
	std::string m_exportedShvPath;
	shv::chainpack::RpcValue m_options;
};

}

#endif // MASTERBROKERCONNECTION_H
