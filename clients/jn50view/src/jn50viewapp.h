#pragma once

#include <shv/iotqt/node/shvnode.h>
#include <shv/coreqt/utils.h>

#include <QApplication>
#include <QDateTime>

class AppCliOptions;
class QFileSystemWatcher;

namespace shv { namespace chainpack { class RpcMessage; }}
namespace shv { namespace iotqt { namespace rpc { class DeviceConnection; }}}
namespace shv { namespace iotqt { namespace node { class ShvNodeTree; }}}

class Jn50ViewApp : public QApplication
{
	Q_OBJECT
private:
	using Super = QApplication;

public:
	struct ConvStatus {
		enum Enum {
			RUN = 1 << 0,
			SERVICE_RUN = 1 << 1,
			STAND_BY = 1 << 2,
			AC_READY = 1 << 3,
			OCL_VOLTAGE = 1 << 4,
			ERROR_HW = 1 << 5,
			ERROR_SW = 1 << 6,
			BLOCKED = 1 << 7,
			DOOR_OPEN = 1 << 8,
			OVERVOLTAGE = 1 << 9,
			OVERCURENT = 1 << 10,
			OVERLOAD = 1 << 11,
			RUN_CAN_ENABLE = 1 << 12,
			BATT_LOW  = 1 << 13,
			BATT_HI = 1 << 14,
			ERROR_RTC  = 1 << 15,
		};
	};
	SHV_PROPERTY_BOOL_IMPL2(s, S, hvDeviceConnected, false)
public:
	Jn50ViewApp(int &argc, char **argv, AppCliOptions* cli_opts);
	~Jn50ViewApp() Q_DECL_OVERRIDE;

	static Jn50ViewApp *instance();
	shv::iotqt::rpc::DeviceConnection *rpcConnection() const {return m_rpcConnection;}
	AppCliOptions* cliOptions() {return m_cliOptions;}

	unsigned convStatus() const;
	void setConvStatus(unsigned s);

	void loadSettings();

	static inline unsigned setMask(unsigned val, unsigned mask, bool on)
	{
		unsigned ret = val;
		if(on)
			ret |= mask;
		else
			ret &= ~mask;
		return ret;
	}

	static inline void setBit(unsigned &val, unsigned bit_no, bool b)
	{
		unsigned mask = 1 << bit_no;
		if(b)
			val |= mask;
		else
			val &= ~mask;
	}

	static inline bool isBit(unsigned val, unsigned bit_no)
	{
		unsigned mask = 1 << bit_no;
		return val & mask;
	}

	static const std::string& logFilePath();

	Q_SIGNAL void shvDeviceValueChanged(const std::string &path, const shv::chainpack::RpcValue &val);
	void setShvDeviceValue(const std::string &path, const shv::chainpack::RpcValue &val);
private:
	void onBrokerConnectedChanged(bool is_connected);
	void onRpcMessageReceived(const shv::chainpack::RpcMessage &msg);

	void onBfsStatusChanged(int);

	void checkShvDeviceConnected();
	void sendGetStatusRequest();
private:
	shv::iotqt::rpc::DeviceConnection *m_rpcConnection = nullptr;
	AppCliOptions* m_cliOptions;

	QTimer *m_shvDeviceConnectedCheckTimer = nullptr;
	int m_getStatusRpcId = std::numeric_limits<int>::max();

	QMap<std::string, shv::chainpack::RpcValue> m_deviceSnapshot;
};

