#pragma once

#include <shv/iotqt/shvnode.h>
#include <shv/chainpack/rpcvalue.h>
#include <shv/coreqt/utils.h>

#include <QObject>
#include <QMap>

namespace shv { namespace chainpack { class RpcMessage; }}
namespace shv { namespace iotqt { class ShvNodeTree; }}

class Lublicator : public shv::iotqt::ShvNode
{
	Q_OBJECT
private:
	using Super = shv::iotqt::ShvNode;
public:
	enum class Status : unsigned {
			 PosOff      = 1 << 0,
			 PosOn       = 1 << 1,
			 PosMiddle   = 1 << 2,
			 PosError    = 1 << 3,
			 BatteryLow  = 1 << 4,
			 BatteryHigh = 1 << 5,
	};
	explicit Lublicator(QObject *parent = nullptr);

	unsigned status() const;
	bool setStatus(unsigned stat);

	StringList childNodeIds() const override;
	shv::chainpack::RpcValue propertyValue(const String &property_name) const;
	bool setPropertyValue(const String &property_name, const shv::chainpack::RpcValue &val);
private:
	std::map<String, shv::chainpack::RpcValue> m_properties;
};

class Revitest : public QObject
{
	Q_OBJECT
public:
	explicit Revitest(QObject *parent = nullptr);

	void onRpcMessageReceived(const shv::chainpack::RpcMessage &msg);
	Q_SIGNAL void sendRpcMessage(const shv::chainpack::RpcMessage &msg);
private:
	void onLublicatorPropertyValueChanged(const std::string &property_name, const shv::chainpack::RpcValue &new_val);
	void createDevices();
private:
	static constexpr size_t LUB_CNT = 27;
	//Lublicator m_lublicators[LUB_CNT];
	shv::iotqt::ShvNodeTree *m_devices = nullptr;
};

