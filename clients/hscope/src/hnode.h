#ifndef HNODE_H
#define HNODE_H

#include <shv/iotqt/node/shvnode.h>

namespace shv { namespace iotqt { namespace rpc { class DeviceConnection; }}}

class ConfigNode;
//class HNodeAgent;
//class HNodeBroker;

class HNode : public shv::iotqt::node::MethodsTableNode
{
	Q_OBJECT

	using Super = shv::iotqt::node::MethodsTableNode;
public:
	struct Status
	{
		enum class Value : int {Unknown = -1, Ok, Warning, Error};
		Value value = Value::Unknown;
		std::string message;

		Status() {}
		Status(Value val, const std::string &msg) : value(val), message(msg) {}
		bool operator==(const Status &o) const { return o.value == value && o.message == message; }
		shv::chainpack::RpcValue toRpcValue() const { return shv::chainpack::RpcValue::Map{{"val", (int)value}, {"msg", message}}; }
		static Status fromRpcValue(const shv::chainpack::RpcValue &val);
	};
public:
	HNode(const std::string &node_id, HNode *parent);

	void setStatus(const Status &st);
	const Status& status() const { return m_status; }
	Q_SIGNAL void statusChanged(const std::string &shv_path, const Status &status);

	void setOverallStatus(const Status &st);
	const Status& overallStatus() const { return m_overallStatus; }
	Q_SIGNAL void overallStatusChanged(const std::string &shv_path, const Status &status);

	virtual void updateOverallStatus();

	std::string appConfigDir();
	std::string nodeConfigDir();
	std::string scriptsDir();
	std::string templatesDir();

	std::string nodeConfigFilePath();
	virtual std::string templateFileName();

	virtual void load();
	void reload();

	shv::chainpack::RpcValue callMethod(const StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params) override;
	//using Super::callMethod;
protected:
	shv::iotqt::rpc::DeviceConnection *appRpcConnection();
	//HNodeBroker* brokerNode();
	//HNodeAgent* agentNode();
	template<class T>
	T findParent()
	{
		QObject *o = parent();
		while(o) {
			T t = qobject_cast<T>(o);
			if(t)
				return t;
			o = o->parent();
		}
		return nullptr;
	}

	virtual void onChildStatusChanged();
	std::vector<std::string> lsConfigDir();
	Status overallChildrenStatus();

	shv::chainpack::RpcValue configValueOnPath(const std::string &shv_path, const shv::chainpack::RpcValue &default_val = shv::chainpack::RpcValue()) const;
protected:
	Status m_status;
	Status m_overallStatus;
	ConfigNode *m_confignode = nullptr;
};

#endif // HNODE_H
