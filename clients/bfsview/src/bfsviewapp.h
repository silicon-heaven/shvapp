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

class AppRootNode : public shv::iotqt::node::ShvRootNode
{
	using Super = shv::iotqt::node::ShvRootNode;
public:
	explicit AppRootNode(QObject *parent = nullptr) : Super(parent) {}

	size_t methodCount(const StringViewList &shv_path) override;
	const shv::chainpack::MetaMethod* metaMethod(const StringViewList &shv_path, size_t ix) override;

	shv::chainpack::RpcValue callMethod(const StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params) override;

	//shv::chainpack::RpcValue processRpcRequest(const shv::chainpack::RpcRequest &rq) override;
};

class PwrStatusNode : public shv::iotqt::node::ShvNode
{
	using Super = shv::iotqt::node::ShvNode;
public:
	enum class PwrStatus : unsigned {Unknown = 0, On, Off};

	explicit PwrStatusNode(shv::iotqt::node::ShvNode *parent = nullptr);

	PwrStatus pwrStatus() const {return m_pwrStatus;}
	void setPwrStatus(PwrStatus s);
	void sendPwrStatusChanged();

	size_t methodCount(const StringViewList &shv_path) override;
	const shv::chainpack::MetaMethod* metaMethod(const StringViewList &shv_path, size_t ix) override;

	shv::chainpack::RpcValue callMethod(const StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params) override;
private:
	void sendPwrStatusChangedDeferred();
private:
	PwrStatus m_pwrStatus = PwrStatus::Unknown;
	QTimer *m_sendPwrStatusDeferredTimer = nullptr;
	PwrStatus m_pwrStatusToSendDeferred = PwrStatus::Unknown;
};

class BfsViewApp : public QApplication
{
	Q_OBJECT
private:
	using Super = QApplication;

public:
	enum class BfsStatus : int {
		BfsOn = 0,
		BfsOff,
		Buffering,
		Charging,
		ConvOn,
		OmpagOn,
		OmpagOff,
		Cooling,
		Error,
		Warning,
		ShvDisconnected,
		MswOn,
		MswOff,
		HsswTOn,
		DoorOpen,
		FswOn,
	};
	using PwrStatus = PwrStatusNode::PwrStatus;
	using SwitchStatus = PwrStatus;

	SHV_PROPERTY_IMPL2(int, b, B, fsStatus, 0)
	SHV_PROPERTY_IMPL2(SwitchStatus, o, O, mpagRequiredSwitchStatus, SwitchStatus::Unknown)
	SHV_PROPERTY_IMPL2(SwitchStatus, c, C, onvRequiredSwitchStatus, SwitchStatus::Unknown)
	SHV_PROPERTY_BOOL_IMPL2(p, P, lcConnected, false)
public:
	BfsViewApp(int &argc, char **argv, AppCliOptions* cli_opts);
	~BfsViewApp() Q_DECL_OVERRIDE;

	static BfsViewApp *instance();
	shv::iotqt::rpc::DeviceConnection *rpcConnection() const {return m_rpcConnection;}
	AppCliOptions* cliOptions() {return m_cliOptions;}

	void loadSettings();

	void setPwrStatus(PwrStatus u);
	PwrStatus pwrStatus();
	Q_SIGNAL void pwrStatusChanged(PwrStatus s);

	void setOmpag(bool val);
	void setConv(bool val);

	SwitchStatus ompagSwitchStatus();
	SwitchStatus convSwitchStatus();

	static inline void setBit(int &val, int bit_no, bool b)
	{
		int mask = 1 << bit_no;
		if(b)
			val |= mask;
		else
			val &= ~mask;
	}

	static inline bool isBit(int val, BfsStatus bit_no)
	{
		int mask = 1 << (int)bit_no;
		return val & mask;
	}

	static const std::string& logFilePath();

	static QString switchStatusToString(BfsViewApp::SwitchStatus status);
private:
	void onBrokerConnectedChanged(bool is_connected);
	void onRpcMessageReceived(const shv::chainpack::RpcMessage &msg);

	void onBfsStatusChanged(int);

	void checkPowerSwitchStatusFile();
	void checkPlcConnected();
	void sendGetStatusRequest();

	const std::string& bfsPlcShvPath();
	std::string bfsPlcShvPathForStatus();
private:
	shv::iotqt::rpc::DeviceConnection *m_rpcConnection = nullptr;
	AppCliOptions* m_cliOptions;

	shv::iotqt::node::ShvNodeTree *m_shvTree = nullptr;
	PwrStatusNode *m_pwrStatusNode;

	//QString m_powerSwitchName;
	QString m_powerFileName;
	QFileSystemWatcher *m_powerFileWatcher = nullptr;
	QTimer *m_powerFileCheckTimer = nullptr;
	struct TS
	{
		QDateTime when;
		QString timeStampString;
	};
	QMap<QString, TS> m_powerSwitchStatus;

	QTimer *m_plcConnectedCheckTimer = nullptr;
	int m_getStatusRpcId = std::numeric_limits<int>::max();

};

