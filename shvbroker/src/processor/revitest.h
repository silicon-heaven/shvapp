#pragma once

#include <shv/core/chainpack/rpcvalue.h>
#include <shv/coreqt/utils.h>

#include <QObject>

namespace shv { namespace core { namespace chainpack { class RpcMessage; }}}

namespace processor {

class Lublicator : public QObject
{
	Q_OBJECT
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

	const shv::core::chainpack::RpcValue::List& propertyNames() const;
	shv::core::chainpack::RpcValue propertyValue(const std::string &property_name) const;
	bool setPropertyValue(const std::string &property_name, const shv::core::chainpack::RpcValue &val);
	Q_SIGNAL void propertyValueChanged(const std::string &property_name, const shv::core::chainpack::RpcValue &new_val);
private:
	std::map<std::string, shv::core::chainpack::RpcValue> m_properties;
};

class Revitest : public QObject
{
	Q_OBJECT
public:
	explicit Revitest(QObject *parent = nullptr);

	void onRpcMessageReceived(const shv::core::chainpack::RpcMessage &msg);
	Q_SIGNAL void sendRpcMessage(const shv::core::chainpack::RpcMessage &msg);
private:
	void onLublicatorPropertyValueChanged(const std::string &property_name, const shv::core::chainpack::RpcValue &new_val);
private:
	static constexpr size_t LUB_CNT = 27;
	Lublicator m_lublicators[LUB_CNT];
};

} // namespace processor
