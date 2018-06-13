#pragma once

#include <shv/iotqt/rpc/serverconnection.h>

#include <set>

class QTimer;

namespace shv { namespace core { class StringView; }}

namespace rpc {

class ServerConnection : public shv::iotqt::rpc::ServerConnection
{
	Q_OBJECT

	using Super = shv::iotqt::rpc::ServerConnection;

	SHV_FIELD_IMPL(std::string, m, M, ountPoint)
public:
	struct Subscription
	{
		std::string absolutePath;
		std::string relativePath;
		std::string method;

		Subscription() {}
		Subscription(const std::string &ap, const std::string &rp, const std::string &m) : absolutePath(ap), relativePath(rp), method(m) {}

		std::string toRelativePath(const std::string &abs_path, bool &changed) const;
		static std::string toAbsolutePath(const std::string &mount_point, const std::string &rel_path);

		bool operator<(const Subscription &o) const;
		bool operator==(const Subscription &o) const;
		bool match(const shv::core::StringView &shv_path, const shv::core::StringView &shv_method) const;
		std::string toString() const {return absolutePath + ':' + method;}
	};
public:
	ServerConnection(QTcpSocket* socket, QObject *parent = 0);

	shv::chainpack::RpcValue deviceId() const;

	void setIdleWatchDogTimeOut(unsigned sec);

	void createSubscription(const std::string &rel_path, const std::string &method);
	int isSubscribed(const std::string &path, const std::string &method) const;
	//std::vector<std::string> subscriptionKeys() const;
	size_t subscriptionCount() const {return m_subscriptions.size();}
	const Subscription& subscriptionAt(size_t ix) const {return m_subscriptions.at(ix);}

	void sendMessage(const shv::chainpack::RpcMessage &rpc_msg) override;
	void sendRawData(const shv::chainpack::RpcValue::MetaData &meta_data, std::string &&data) override;
	static std::string dataToCpon(shv::chainpack::Rpc::ProtocolType protocol_type, const shv::chainpack::RpcValue::MetaData &md, const std::string &data, size_t start_pos);
private:
	void onRpcDataReceived(shv::chainpack::Rpc::ProtocolType protocol_type, shv::chainpack::RpcValue::MetaData &&md, const std::string &data, size_t start_pos, size_t data_len) override;
	bool checkPassword(const shv::chainpack::RpcValue::Map &login) override;
	std::string passwordHash(PasswordHashType type, const std::string &user) override;
	shv::chainpack::RpcValue login(const shv::chainpack::RpcValue &auth_params) override;
private:
	QTimer *m_idleWatchDogTimer = nullptr;
	/*
	struct SubsKeyLess
	{
		bool operator()(const SubsKey &k1, const SubsKey &k2) const
		{
			if(k1.pathPattern == k2.pathPattern)
				return k1.method < k2.method;
			return k1.pathPattern < k2.pathPattern;
		}
	};
	*/
	std::vector<Subscription> m_subscriptions;
};

} // namespace rpc
