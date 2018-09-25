#pragma once

#include <shv/iotqt/node//shvnode.h>
#include <shv/chainpack/rpcvalue.h>
#include <shv/coreqt/utils.h>

#include <QObject>
#include <QMap>

class QSqlQuery;

namespace shv { namespace chainpack { class RpcMessage; }}

class Lublicator : public shv::iotqt::node::ShvNode
{
	Q_OBJECT
private:
	using Super = shv::iotqt::node::ShvNode;
public:
	enum class Status : unsigned
	{
		PosOff      = 1 << 0,
		PosOn       = 1 << 1,
		PosMiddle   = 1 << 2,
		PosError    = 1 << 3,
		BatteryLow  = 1 << 4,
		BatteryHigh = 1 << 5,
		DoorOpenCabinet = 1 << 6,
		DoorOpenMotor = 1 << 7,
		ModeAuto    = 1 << 8,
		ModeRemote  = 1 << 9,
		ModeService = 1 << 10,
		MainSwitch  = 1 << 11
	};

	static const char *PROP_STATUS;
	static const char *METH_DEVICE_ID;
	static const char *METH_CMD_ON;
	static const char *METH_CMD_OFF;
	static const char *METH_GET_LOG;

	explicit Lublicator(const std::string &node_id, shv::iotqt::node::ShvNode *parent = nullptr);

	unsigned status() const;
	bool setStatus(unsigned stat);

	Q_SIGNAL void valueChanged(const std::string &key, const shv::chainpack::RpcValue &val);
	Q_SIGNAL void propertyValueChanged(const std::string &property_name, const shv::chainpack::RpcValue &new_val);

	size_t methodCount(const StringViewList &shv_path) override;
	const shv::chainpack::MetaMethod* metaMethod(const StringViewList &shv_path, size_t ix) override;

	bool hasChildren(const StringViewList &shv_path) override;
	StringList childNames(const StringViewList &shv_path) override;
	//shv::chainpack::RpcValue lsAttributes(const std::string &node_id, unsigned attributes, const std::string &shv_path) override;

	shv::chainpack::RpcValue callMethod(const StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params) override;
private:
	shv::chainpack::RpcValue getLog(const shv::chainpack::RpcValue::DateTime &from, const shv::chainpack::RpcValue::DateTime &to);
	void addLogEntry(const std::string &key, const shv::chainpack::RpcValue &value);
	QSqlQuery logQuery();
	QString sqlConnectionName();
private:
	unsigned m_status = 0;
	/*
	struct LogEntry
	{
		//shv::chainpack::RpcValue::DateTime ts;
		std::string key;
		shv::chainpack::RpcValue value;
	};
	*/
};



