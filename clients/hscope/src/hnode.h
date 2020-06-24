#ifndef HNODE_H
#define HNODE_H

#include <shv/iotqt/node/shvnode.h>

namespace shv { namespace iotqt { namespace rpc { class DeviceConnection; }}}

class ConfigNode;
class HNodeAgent;
class HNodeBroker;

struct NodeStatus
{
public:
	enum class Level : int {Error = 0, Warning, Ok, Disabled, Unknown};
	Level level = Level::Unknown;
	std::string message;

	NodeStatus() {}
	NodeStatus(Level val, const std::string &msg) : level(val), message(msg) {}

	//bool operator==(const NodeStatus &o) const { return o.value == value && o.message == message; }

	shv::chainpack::RpcValue toRpcValue() const { return shv::chainpack::RpcValue::Map{{"val", (int)level}, {"msg", message}}; }
	std::string toString() const { return levelToStringAbbr(level) + std::string(": ") + message; }
	static NodeStatus fromRpcValue(const shv::chainpack::RpcValue &val);
	static const char* levelToString(Level val);
	static const char* levelToStringAbbr(Level val);
};

class HNode : public shv::iotqt::node::MethodsTableNode
{
	Q_OBJECT

	using Super = shv::iotqt::node::MethodsTableNode;
public:
	HNode(const std::string &node_id, HNode *parent);

	bool isDisabled() const;

	void setStatus(const NodeStatus &st);
	const NodeStatus& status() const { return m_status; }
	Q_SIGNAL void statusChanged(const std::string &shv_path, const NodeStatus &status);

	void setOverallStatus(const NodeStatus::Level &level);
	NodeStatus::Level overallStatus() const { return m_overallStatus; }
	Q_SIGNAL void overallStatusChanged(const std::string &shv_path, NodeStatus::Level level);

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
	HNodeAgent* parentAgentNode();
protected:
	shv::iotqt::rpc::DeviceConnection *appRpcConnection();
	HNodeBroker* parentBrokerNode();
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
	NodeStatus::Level overallChildrenStatus();

	shv::chainpack::RpcValue configValueOnPath(const std::string &shv_path, const shv::chainpack::RpcValue &default_val = shv::chainpack::RpcValue()) const;
protected:
	NodeStatus m_status;
	NodeStatus::Level m_overallStatus = NodeStatus::Level::Unknown;
	ConfigNode *m_configNode = nullptr;
};

#endif // HNODE_H
