#include "lublicator.h"

#include <shv/chainpack/rpcmessage.h>
#include <shv/chainpack/metamethod.h>
#include <shv/core/string.h>
#include <shv/core/stringview.h>
#include <shv/core/exception.h>

#include <shv/coreqt/log.h>

#include <shv/iotqt/node//shvnodetree.h>

namespace cp = shv::chainpack;
namespace iot = shv::iotqt;

const char *Lublicator::PROP_STATUS = "status";
const char *Lublicator::METH_DEVICE_ID = "deviceId";
const char *Lublicator::METH_CMD_ON = "cmdOn";
const char *Lublicator::METH_CMD_OFF = "cmdOff";
const char *Lublicator::METH_GET_LOG = "getLog";

namespace {

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

//static const std::string ODPOJOVACE_PATH = "/shv/eu/pl/lublin/odpojovace/";
}

Lublicator::Lublicator(ShvNode *parent)
	: Super(parent)
{
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
};

static std::vector<cp::MetaMethod> meta_methods_status {
	{cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, false},
	{cp::Rpc::METH_GET, cp::MetaMethod::Signature::RetVoid, false},
};

size_t Lublicator::methodCount2(const std::string &shv_path)
{
	if(shv_path.empty())
		return meta_methods_device.size();
	return meta_methods_status.size();
}

const shv::chainpack::MetaMethod *Lublicator::metaMethod2(size_t ix, const std::string &shv_path)
{
	if(shv_path.empty())
		return &(meta_methods_device.at(ix));
	return &(meta_methods_status.at(ix));
}

shv::chainpack::RpcValue Lublicator::hasChildren2(const std::string &shv_path)
{
	return shv_path.empty();
}

shv::iotqt::node::ShvNode::StringList Lublicator::childNames2(const std::string &shv_path)
{
	shvLogFuncFrame() << shvPath() << "for:" << shv_path;
	if(shv_path.empty())
		return shv::iotqt::node::ShvNode::StringList{PROP_STATUS};
	return shv::iotqt::node::ShvNode::StringList{};
}

shv::chainpack::RpcValue Lublicator::call2(const std::string &method, const shv::chainpack::RpcValue &params, const std::string &shv_path)
{
	if(shv_path.empty()) {
		if(method == cp::Rpc::METH_GET) {
			return status();
		}
		if(method == METH_DEVICE_ID) {
			return parentNode()->nodeId();
		}
		if(method == METH_CMD_ON) {
			unsigned stat = status();
			stat &= ~(unsigned)Status::PosOff;
			stat |= (unsigned)Status::PosMiddle;
			setStatus(stat);
			stat &= ~(unsigned)Status::PosMiddle;
			stat |= (unsigned)Status::PosOn;
			setStatus(stat);
			return true;
		}
		if(method == METH_CMD_OFF) {
			unsigned stat = status();
			stat &= ~(unsigned)Status::PosOn;
			stat |= (unsigned)Status::PosMiddle;
			setStatus(stat);
			stat &= ~(unsigned)Status::PosMiddle;
			stat |= (unsigned)Status::PosOff;
			setStatus(stat);
			return true;
		}
		if(method == METH_GET_LOG) {
			const shv::chainpack::RpcValue::Map &m = params.toMap();
			cp::RpcValue::DateTime from = m.value("from").toDateTime();
			cp::RpcValue::DateTime to = m.value("to").toDateTime();
			return getLog(from, to);
		}
	}
	else if(shv_path == "status") {
		if(method == cp::Rpc::METH_GET) {
			return status();
		}
	}	return Super::call2(method, params, shv_path);
}

shv::chainpack::RpcValue Lublicator::getLog(const shv::chainpack::RpcValue::DateTime &from, const shv::chainpack::RpcValue::DateTime &to)
{
	shv::chainpack::RpcValue::List lines;
	auto it1 = m_log.cbegin();
	if(from.isValid())
		it1 = m_log.lower_bound(from);
	auto it2 = m_log.cend();
	if(to.isValid())
		it2 = m_log.upper_bound(to);
	for(auto it = it1; it != it2; ++it) {
		shv::chainpack::RpcValue::List line;
		line.push_back(it->first);
		line.push_back(it->second.key);
		line.push_back(it->second.value);
		lines.push_back(line);
	}
	shv::chainpack::RpcValue ret = lines;
	cp::RpcValue::Map device;
	device["id"] = "DS:" + nodeId();
	cp::RpcValue::MetaData md;
	md.setValue("device", device); // required
	md.setValue("logVersion", 1); // required
	md.setValue("dateTime", cp::RpcValue::DateTime::now());
	md.setValue("tsFrom", from);
	md.setValue("tsTo", to);
	md.setValue("fields", cp::RpcValue::List{
					cp::RpcValue::Map{{"name", "timestamp"}},
					cp::RpcValue::Map{{"name", "path"}},
					cp::RpcValue::Map{{"name", "value"}},
				});
	md.setValue("pathInfo", cp::RpcValue::Map{
					{"status", cp::RpcValue::Map{
							{"description", "PosOff = 0, PosOn = 1, PosMiddle = 2, PosError = 3, BatteryLow = 4, BatteryHigh = 5, DoorOpenCabinet = 6, DoorOpenMotor = 7, ModeAuto = 8, ModeRemote = 9, ModeService = 10, MainSwitch = 11"},
						}
					},
				});
	ret.setMetaData(std::move(md));
	return ret;
}

void Lublicator::addLogEntry(const std::string &key, const shv::chainpack::RpcValue &value)
{
	shv::chainpack::RpcValue::DateTime ts = cp::RpcValue::DateTime::now();
	LogEntry &le = m_log[ts];
	le.key = key;
	le.value = value;
}
#if 0
shv::iotqt::node::ShvNode::StringList Lublicator::childNames(const std::string &shv_path) const
{
	Q_UNUSED(shv_path)
	static ShvNode::StringList keys;
	if(keys.empty()) for(const auto &imap: m_properties)
		keys.push_back(imap.first);
	return keys;
}

shv::chainpack::RpcValue::List Lublicator::propertyMethods(const shv::iotqt::node::ShvNode::String &prop_name) const
{
	//shvWarning() << prop_name;
	if(prop_name.empty()) {
		return cp::RpcValue::List{cp::Rpc::METH_DIR, cp::Rpc::METH_LS, M_CMD_POS_ON, M_CMD_POS_OFF, "getLog"};
	}
	else if(prop_name == "status") {
		return cp::RpcValue::List{cp::Rpc::METH_DIR, cp::Rpc::METH_GET};
	}
	return cp::RpcValue::List();
}

shv::chainpack::RpcValue Lublicator::propertyValue(const std::string &property_name) const
{
	//if(property_name == S_NAME)
	//	return objectName().toStdString();
	auto it = m_properties.find(property_name);
	if(it == m_properties.end())
		return cp::RpcValue();
	return it->second;
}

bool Lublicator::setPropertyValue(const shv::iotqt::node::ShvNode::String &prop_name, const shv::chainpack::RpcValue &val)
{
	Q_UNUSED(prop_name)
	Q_UNUSED(val)
	/*
	if(property_name == S_STATUS)
		return false;
	if(property_name == S_NAME)
		return false;
	if(property_name == S_CMD_POS_ON) {
		unsigned stat = status();
		stat &= ~(unsigned)Status::PosOff;
		stat |= (unsigned)Status::PosMiddle;
		setStatus(stat);
		stat &= ~(unsigned)Status::PosMiddle;
		stat |= (unsigned)Status::PosOn;
		setStatus(stat);
		return true;
	}
	if(property_name == S_CMD_POS_OFF) {
		unsigned stat = status();
		stat &= ~(unsigned)Status::PosOn;
		stat |= (unsigned)Status::PosMiddle;
		setStatus(stat);
		stat &= ~(unsigned)Status::PosMiddle;
		stat |= (unsigned)Status::PosOff;
		setStatus(stat);
		return true;
	}
	if(property_name == S_BATT_LEVSIM) {
		//shvWarning() << "new bat lev:" << val.toCpon() << val.toDecimal().mantisa();
		m_properties[S_BATT_LEVSIM] = val;
	}
	auto it = m_properties.find(property_name);
	if(it == m_properties.end())
		return false;
	it->second = val;
	{
		unsigned stat = status();
		int batt_hi = m_properties.at(S_BATT_HI).toDecimal().mantisa();
		int batt_lo = m_properties.at(S_BATT_LOW).toDecimal().mantisa();
		int batt_lev = m_properties.at(S_BATT_LEVSIM).toDouble() * 10;
		stat &= ~(unsigned)Status::BatteryHigh;
		stat &= ~(unsigned)Status::BatteryLow;
		if(batt_lev > batt_hi)
			stat |= (unsigned)Status::BatteryHigh;
		else if(batt_lev < batt_lo)
			stat |= (unsigned)Status::BatteryLow;
		setStatus(stat);
	}
	*/
	return true;
}
#endif


