#ifndef HNODE_H
#define HNODE_H

#include <shv/iotqt/node/shvnode.h>

namespace shv { namespace iotqt { namespace rpc { class DeviceConnection; }}}

class ConfigNode;
//class HNodeAgent;
class HNodeBroker;

struct NodeStatus
{
public:
	enum class Value : int {Unknown = -1, Ok, Warning, Error};
	Value value = Value::Unknown;
	std::string message;

	NodeStatus() {}
	NodeStatus(Value val, const std::string &msg) : value(val), message(msg) {}
	bool operator==(const NodeStatus &o) const { return o.value == value && o.message == message; }
	shv::chainpack::RpcValue toRpcValue() const { return shv::chainpack::RpcValue::Map{{"val", (int)value}, {"msg", message}}; }
	std::string toString() const { return toRpcValue().toCpon(); }
	static NodeStatus fromRpcValue(const shv::chainpack::RpcValue &val);
	static const char* valueToString(Value val);
	static const char* valueToStringAbbr(Value val);
};

class HNode : public shv::iotqt::node::MethodsTableNode
{
	Q_OBJECT

	using Super = shv::iotqt::node::MethodsTableNode;
public:
	HNode(const std::string &node_id, HNode *parent);

	void setStatus(const NodeStatus &st);
	const NodeStatus& status() const { return m_status; }
	Q_SIGNAL void statusChanged(const std::string &shv_path, const NodeStatus &status);

	void setOverallStatus(const NodeStatus &st);
	const NodeStatus& overallStatus() const { return m_overallStatus; }
	Q_SIGNAL void overallStatusChanged(const std::string &shv_path, const NodeStatus &status);

	std::string appConfigDir();
	std::string nodeConfigDir();
	//std::string scriptsDir();
	std::string templatesDir();

	//std::string nodeConfigFilePath();
	virtual std::string templateFileName();

	virtual void load();
	void reload();

	shv::chainpack::RpcValue callMethod(const StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params) override;
	//using Super::callMethod;
protected:
	shv::iotqt::rpc::DeviceConnection *appRpcConnection();
	HNodeBroker* parentBrokerNode();
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

	std::vector<std::string> lsConfigDir();

	//virtual void onChildStatusChanged() {}
	virtual void updateOverallStatus();
	NodeStatus overallChildrenStatus();

	shv::chainpack::RpcValue configValueOnPath(const std::string &shv_path, const shv::chainpack::RpcValue &default_val = shv::chainpack::RpcValue()) const;
protected:
	NodeStatus m_status;
	NodeStatus m_overallStatus;
	ConfigNode *m_confignode = nullptr;
};

#endif // HNODE_H
