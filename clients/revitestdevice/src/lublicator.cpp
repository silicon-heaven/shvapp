#include "lublicator.h"
#include "revitestapp.h"
#include "appclioptions.h"

#include <shv/iotqt/node/shvnodetree.h>
#include <shv/iotqt/utils/fileshvjournal.h>

#include <shv/chainpack/rpcmessage.h>
#include <shv/chainpack/metamethod.h>
#include <shv/core/string.h>
#include <shv/core/stringview.h>
#include <shv/core/exception.h>

#include <shv/coreqt/log.h>
#include <shv/coreqt/exception.h>

#include <QDir>
#include <QStandardPaths>
#include <QTimer>
#include <QVariant>

namespace cp = shv::chainpack;
namespace iot = shv::iotqt;

const char *Lublicator::PROP_STATUS = "status";
const char *Lublicator::PROP_BATTERY_VOLTAGE = "batteryVoltage";
const char *Lublicator::METH_DEVICE_ID = "deviceId";
const char *Lublicator::METH_CMD_ON = "cmdOn";
const char *Lublicator::METH_CMD_OFF = "cmdOff";
const char *Lublicator::METH_GET_LOG = "getLog";
const char METH_BATTERY_VOLTAGE[] = "batteryVoltage";
const char METH_SIM_SET_BATTERY_VOLTAGE[] = "sim_setBatteryVoltage";
const char METH_BATTERY_LOW_TRESHOLD[] = "batteryLowTreshold";
const char METH_SET_BATTERY_LOW_TRESHOLD[] = "setBatteryLowTreshold";
const char METH_BATTERY_HIGH_TRESHOLD[] = "batteryHighTreshold";
const char METH_SET_BATTERY_HIGH_TRESHOLD[] = "setBatteryHighTreshold";
const char METH_SIM_SET[] = "sim_set";

Lublicator::Lublicator(const std::string &node_id, ShvNode *parent)
	: Super(parent)
{
	setNodeId(node_id);

	if(RevitestApp::instance()->cliOptions()->isSimBattVoltageDrift()) {
		QTimer *bat_voltage_sim = new QTimer(this);
		int node_no = std::stoi(node_id);
		connect(bat_voltage_sim, &QTimer::timeout, this, [this, node_no]() {
			m_battSimCnt++;
			if((m_battSimCnt % (int)RevitestApp::LUB_CNT) + 1 == node_no) {
				//shvWarning() << node_no << "bv:" << ((m_battSimCnt % (int)RevitestApp::LUB_CNT) + 1);
				unsigned bv = m_batteryVoltage;
				if(++bv > 30)
					bv = 18;
				sim_setBateryVoltage(bv);
			}
		});
		bat_voltage_sim->start(1000);
	}

	connect(this, &Lublicator::valueChanged, this, &Lublicator::addLogEntry);
}

unsigned Lublicator::status() const
{
	return m_status;
}

bool Lublicator::setStatus(unsigned stat)
{
	unsigned old_stat = status();
	if(old_stat != stat) {
		m_status = stat;
		emit valueChanged(PROP_STATUS, stat);
		emit propertyValueChanged(nodeId() + '/' + PROP_STATUS, stat);
		return true;
	}
	return false;
}

static std::vector<cp::MetaMethod> meta_methods_device {
	{cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, false},
	{cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, false},
	{Lublicator::METH_DEVICE_ID, cp::MetaMethod::Signature::RetVoid, false},
	{Lublicator::METH_CMD_ON, cp::MetaMethod::Signature::VoidVoid, false},
	{Lublicator::METH_CMD_OFF, cp::MetaMethod::Signature::VoidVoid, false},
	{Lublicator::METH_GET_LOG, cp::MetaMethod::Signature::RetParam, false},
	{METH_BATTERY_LOW_TRESHOLD, cp::MetaMethod::Signature::RetVoid, false},
	{METH_SET_BATTERY_LOW_TRESHOLD, cp::MetaMethod::Signature::VoidParam, false},
	{METH_BATTERY_HIGH_TRESHOLD, cp::MetaMethod::Signature::RetVoid, false},
	{METH_SET_BATTERY_HIGH_TRESHOLD, cp::MetaMethod::Signature::VoidParam, false},
	{METH_BATTERY_VOLTAGE, cp::MetaMethod::Signature::RetVoid, false},
	{METH_SIM_SET_BATTERY_VOLTAGE, cp::MetaMethod::Signature::VoidParam, false},
};

static std::vector<cp::MetaMethod> meta_methods_status {
	{cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, false},
	{cp::Rpc::METH_GET, cp::MetaMethod::Signature::RetVoid, false},
	{METH_SIM_SET, cp::MetaMethod::Signature::VoidParam, false},
	{cp::Rpc::NTF_VAL_CHANGED, cp::MetaMethod::Signature::VoidParam, true},
};

size_t Lublicator::methodCount(const StringViewList &shv_path)
{
	if(shv_path.empty())
		return meta_methods_device.size();
	if(shv_path.size() == 1)
		return meta_methods_status.size();
	return 0;
}

const shv::chainpack::MetaMethod *Lublicator::metaMethod(const StringViewList &shv_path, size_t ix)
{
	if(shv_path.empty())
		return &(meta_methods_device.at(ix));
	if(shv_path.size() == 1)
		return &(meta_methods_status.at(ix));
	return nullptr;
}

shv::chainpack::RpcValue Lublicator::hasChildren(const StringViewList &shv_path)
{
	return shv_path.empty();
}

shv::iotqt::node::ShvNode::StringList Lublicator::childNames(const StringViewList &shv_path)
{
	//shvLogFuncFrame() << shvPath() << "for:" << shv_path;
	if(shv_path.empty())
		return shv::iotqt::node::ShvNode::StringList{PROP_STATUS};
	return shv::iotqt::node::ShvNode::StringList{};
}

shv::chainpack::RpcValue Lublicator::callMethod(const StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params)
{
	if(shv_path.empty()) {
		if(method == cp::Rpc::METH_GET) {
			return status();
		}
		if(method == METH_DEVICE_ID) {
			//shvError() << "device id:" << nodeId();
			return nodeId();
		}
		if(method == METH_CMD_ON) {
			unsigned stat = status();
			unsigned new_stat = stat;
			new_stat &= ~(unsigned)Status::PosMiddle;
			new_stat |= (unsigned)Status::PosInternalOn;
			new_stat |= (unsigned)Status::PosOn;
			if(stat != new_stat) {
				stat &= ~(unsigned)Status::PosInternalOff;
				stat &= ~(unsigned)Status::PosOff;
				stat |= (unsigned)Status::PosMiddle;
				setStatus(stat);
				stat &= ~(unsigned)Status::PosMiddle;
				stat |= (unsigned)Status::PosInternalOn;
				stat |= (unsigned)Status::PosOn;
				setStatus(stat);
			}
			return true;
		}
		if(method == METH_CMD_OFF) {
			unsigned stat = status();
			unsigned new_stat = stat;
			new_stat &= ~(unsigned)Status::PosMiddle;
			new_stat &= ~(unsigned)Status::PosInternalOff;
			new_stat |= (unsigned)Status::PosOff;
			if(stat != new_stat) {
				stat &= ~(unsigned)Status::PosInternalOn;
				stat &= ~(unsigned)Status::PosOn;
				stat |= (unsigned)Status::PosMiddle;
				setStatus(stat);
				stat &= ~(unsigned)Status::PosMiddle;
				stat &= ~(unsigned)Status::PosInternalOff;
				stat |= (unsigned)Status::PosOff;
				setStatus(stat);
			}
			return true;
		}
		if(method == METH_GET_LOG) {
			return getLog(params);
		}
		if(method == METH_BATTERY_VOLTAGE) {
			return m_batteryVoltage;
		}
		if(method == METH_SIM_SET_BATTERY_VOLTAGE) {
			unsigned d = params.toUInt();
			sim_setBateryVoltage(d);
			return true;
		}
		if(method == METH_BATTERY_LOW_TRESHOLD) {
			return m_batteryLowTreshold;
		}
		if(method == METH_SET_BATTERY_LOW_TRESHOLD) {
			unsigned d = params.toUInt();
			m_batteryLowTreshold = d;
			checkBatteryTresholds();
			return true;
		}
		if(method == METH_BATTERY_HIGH_TRESHOLD) {
			return m_batteryHighTreshold;
		}
		if(method == METH_SET_BATTERY_HIGH_TRESHOLD) {
			unsigned d = params.toUInt();
			m_batteryHighTreshold = d;
			checkBatteryTresholds();
			return true;
		}
	}
	else if(shv_path[0] == "status") {
		if(method == cp::Rpc::METH_GET) {
			return status();
		}
		if(method == METH_SIM_SET) {
			unsigned st = params.toUInt();
			setStatus(st);
			return true;
		}
	}
	return Super::callMethod(shv_path, method, params);
}

shv::chainpack::RpcValue Lublicator::getLog(const shv::chainpack::RpcValue &params)
{
	shv::iotqt::utils::ShvJournalGetLogParams p(params);
	if(p.pathPattern.empty())
		p.pathPattern = nodeId() + "/**";
	else
		p.pathPattern = nodeId() + "/" + p.pathPattern;
	return RevitestApp::instance()->shvJournal()->getLog(p);

}

void Lublicator::addLogEntry(const std::string &key, const shv::chainpack::RpcValue &value)
{
	RevitestApp::instance()->shvJournal()->append({nodeId() + '/' + key, value});
}

void Lublicator::checkBatteryTresholds()
{
	unsigned s = status();
	if(m_batteryVoltage >= m_batteryHighTreshold)
		s |= (unsigned)Status::BatteryHigh;
	else
		s &= ~((unsigned)Status::BatteryHigh);
	if(m_batteryVoltage <= m_batteryLowTreshold)
		s |= (unsigned)Status::BatteryLow;
	else
		s &= ~((unsigned)Status::BatteryLow);
	setStatus(s);
}

void Lublicator::sim_setBateryVoltage(unsigned v)
{
	if(m_batteryVoltage == v)
		return;
	m_batteryVoltage = v;
	emit valueChanged(PROP_BATTERY_VOLTAGE, v);
	checkBatteryTresholds();
}

