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
		PosOff = 1 << 0, // 1 when PosInternalOff is 1 and PosInternalOn is 0
		PosOn = 1 << 1, // 1 when PosInternalOff is 0 and PosInternalOn is 1
		PosMiddle = 1 << 2, // 1 when both PosOff and PosOn are 1
		PosError = 1 << 3, // 1 when both PosOff and PosOn are 0
		BatteryLow = 1 << 4, // 1 when battery voltage is lower batteryLowTreshold()
		BatteryHigh = 1 << 5, // 1 when battery voltage is higher batteryHighTreshold()
		DoorOpenCabinet = 1 << 6, // DI
		DoorOpenMotor = 1 << 7, // DI
		ModeAuto = 1 << 8, // DI (auto/remote/service)
		ModeRemote = 1 << 9, // DI
		ModeService = 1 << 10, // DI
		MainSwitch = 1 << 11, // DI
		ErrorRtc = 1 << 12, // 1 when RTC problem
		PosInternalOff = 1 << 13, // DI - internal position sensor
		PosInternalOn = 1 << 14, // DI - internal position sensor
	};

	static const char *PROP_STATUS;
	static const char *PROP_BATTERY_VOLTAGE;
	static const char *METH_DEVICE_ID;
	static const char *METH_CMD_ON;
	static const char *METH_CMD_OFF;
	static const char *METH_GET_LOG;

	explicit Lublicator(const std::string &node_id, shv::iotqt::node::ShvNode *parent = nullptr);

	unsigned status() const;
	bool setStatus(unsigned stat);

	Q_SIGNAL void valueChanged(const std::string &key, const shv::chainpack::RpcValue &val);
	Q_SIGNAL void fastValueChanged(const std::string &key, const shv::chainpack::RpcValue &val);
	Q_SIGNAL void propertyValueChanged(const std::string &property_name, const shv::chainpack::RpcValue &new_val);

	size_t methodCount(const StringViewList &shv_path) override;
	const shv::chainpack::MetaMethod* metaMethod(const StringViewList &shv_path, size_t ix) override;

	shv::chainpack::RpcValue hasChildren(const StringViewList &shv_path) override;
	StringList childNames(const StringViewList &shv_path) override;

	shv::chainpack::RpcValue callMethod(const StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params) override;
private:
	shv::chainpack::RpcValue getLog(const shv::chainpack::RpcValue &params);
	void addLogEntry(const std::string &key, const shv::chainpack::RpcValue &value);
	void addFastLogEntry(const std::string &key, const shv::chainpack::RpcValue &value);
	void checkBatteryTresholds();
	void sim_setBateryVoltage(unsigned v);
private:
	unsigned m_status = 0;
	unsigned m_batteryVoltage = 24;
	unsigned m_batteryLowTreshold = 20;
	unsigned m_batteryHighTreshold = 28;
	int m_battSimCnt = 0;
};



