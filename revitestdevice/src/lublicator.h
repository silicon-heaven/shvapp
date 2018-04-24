#pragma once

#include <shv/iotqt/node//shvtreenode.h>
#include <shv/chainpack/rpcvalue.h>
#include <shv/coreqt/utils.h>

#include <QObject>
#include <QMap>

namespace shv { namespace chainpack { class RpcMessage; }}

class Lublicator : public shv::iotqt::node::ShvTreeNode
{
	Q_OBJECT
private:
	using Super = shv::iotqt::node::ShvTreeNode;
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

	explicit Lublicator(shv::iotqt::node::ShvNode *parent = nullptr);

	unsigned status() const;
	bool setStatus(unsigned stat);

	Q_SIGNAL void propertyValueChanged(const std::string &property_name, const shv::chainpack::RpcValue &new_val);

	size_t methodCount2(const std::string &shv_path = std::string()) override;
	const shv::chainpack::MetaMethod* metaMethod2(size_t ix, const std::string &shv_path = std::string()) override;

	shv::chainpack::RpcValue hasChildren2(const std::string &shv_path) override;
	StringList childNames2(const std::string &shv_path = std::string()) override;
	//shv::chainpack::RpcValue lsAttributes(const std::string &node_id, unsigned attributes, const std::string &shv_path) override;

	shv::chainpack::RpcValue call2(const std::string &method, const shv::chainpack::RpcValue &params, const std::string &shv_path = std::string()) override;
private:
	unsigned m_status = 0;
};



