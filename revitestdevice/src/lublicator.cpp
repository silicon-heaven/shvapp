#include "lublicator.h"

#include <shv/chainpack/rpcmessage.h>
#include <shv/core/string.h>
#include <shv/core/stringview.h>
#include <shv/core/exception.h>

#include <shv/coreqt/log.h>

#include <shv/iotqt/node//shvnodetree.h>

namespace cp = shv::chainpack;
namespace iot = shv::iotqt;

namespace {
const iot::node::ShvNode::String S_STATUS = "status";
//const iot::ShvNode::String S_NAME = "name";
//const iot::ShvNode::String S_BATT_LOW = "batteryLimitLow";
//const iot::ShvNode::String S_BATT_HI = "batteryLimitHigh";
//const iot::ShvNode::String S_BATT_LEVSIM = "batteryLevelSimulation";
const iot::node::ShvNode::String M_CMD_POS_ON = "cmdOn";
const iot::node::ShvNode::String M_CMD_POS_OFF = "cmdOff";

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
	m_properties[S_STATUS] = cp::RpcValue::UInt(0);
	//m_properties[S_NAME] = "";
//	m_properties[S_BATT_HI] = cp::RpcValue::Decimal(245, 1);
//	m_properties[S_BATT_LOW] = cp::RpcValue::Decimal(232, 1);
//	m_properties[S_BATT_LEVSIM] = cp::RpcValue::Decimal(240, 1);
//	m_properties[S_CMD_POS_ON];
//	m_properties[S_CMD_POS_OFF];
}

unsigned Lublicator::status() const
{
	return m_properties.at(S_STATUS).toUInt();
}

bool Lublicator::setStatus(unsigned stat)
{
	unsigned old_stat = status();
	if(old_stat != stat) {
		m_properties[S_STATUS] = stat;
		emit propertyValueChanged(objectName().toStdString() + '/' + S_STATUS, stat);
		return true;
	}
	return false;
}

shv::iotqt::node::ShvNode::StringList Lublicator::childNodeIds() const
{
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

Revitest::Revitest(QObject *parent)
	: QObject(parent)
{
	shvLogFuncFrame();
	createDevices();
	/*
	for (size_t i = 0; i < LUB_CNT; ++i) {
		m_lublicators[i].setObjectName(QString::number(i+1));
		connect(&m_lublicators[i], &Lublicator::propertyValueChanged, this, &Revitest::onLublicatorPropertyValueChanged);
	}
	*/
}

void Revitest::createDevices()
{
	m_devices = new shv::iotqt::node::ShvNodeTree(this);
	static constexpr size_t LUB_CNT = 27;
	for (size_t i = 0; i < LUB_CNT; ++i) {
		auto *nd = new Lublicator(m_devices->root());
		nd->setNodeId(std::to_string(i+1));
		connect(nd, &Lublicator::propertyValueChanged, this, &Revitest::onLublicatorPropertyValueChanged);
	}
}

void Revitest::onRpcMessageReceived(const shv::chainpack::RpcMessage &msg)
{
	shvLogFuncFrame() << msg.toCpon();
	if(msg.isRequest()) {
		cp::RpcRequest rq(msg);
		cp::RpcResponse rsp = cp::RpcResponse::forRequest(rq);
		cp::RpcValue result;

		try {
			const std::string path = rq.shvPath();
			std::string path_rest;
			shv::iotqt::node::ShvNode *nd = m_devices->cd(path, &path_rest);
			//shvInfo() << "path:" << path << "->" << nd << "path rest:" << path_rest;
			if(!nd)
				SHV_EXCEPTION("invalid path: " + path);
			if(path.empty()) {
				result = nd->processRpcRequest(rq);
			}
			else {
				Lublicator *lubl = qobject_cast<Lublicator*>(nd);
				if(!lubl)
					SHV_EXCEPTION("invalid lublicator: " + path);
				shv::chainpack::RpcValue::String method = rq.method();
				if(method == cp::Rpc::METH_LS) {
					if(path_rest.empty())
						result = lubl->childNodeIds();
				}
				else if(method == cp::Rpc::METH_DIR) {
					result = lubl->propertyMethods(path_rest);
				}
				else if(method == cp::Rpc::METH_GET) {
					result = lubl->propertyValue(path_rest);
				}
				else if(method == cp::Rpc::METH_SET) {
					result = lubl->setPropertyValue(path_rest, rq.params());
				}
				else if(method == M_CMD_POS_ON) {
					unsigned stat = lubl->status();
					stat &= ~(unsigned)Status::PosOff;
					stat |= (unsigned)Status::PosMiddle;
					lubl->setStatus(stat);
					stat &= ~(unsigned)Status::PosMiddle;
					stat |= (unsigned)Status::PosOn;
					lubl->setStatus(stat);
					result = true;
				}
				else if(method == M_CMD_POS_OFF) {
					unsigned stat = lubl->status();
					stat &= ~(unsigned)Status::PosOn;
					stat |= (unsigned)Status::PosMiddle;
					lubl->setStatus(stat);
					stat &= ~(unsigned)Status::PosMiddle;
					stat |= (unsigned)Status::PosOff;
					lubl->setStatus(stat);
					result = true;
				}
				else {
					SHV_EXCEPTION("Invalid method: " + method + " called for node: " + path);
				}
			}
		}
		catch(shv::core::Exception &e) {
			rsp.setError(cp::RpcResponse::Error::create(cp::RpcResponse::Error::MethodInvocationException, e.message()));
		}
		if(!rsp.isError()) {
			rsp.setResult(result);
		}
		shvDebug() << "sending RPC response:" << rsp.toCpon();
		emit sendRpcMessage(rsp);
	}
	else if(msg.isResponse()) {
		cp::RpcResponse rp(msg);
		shvInfo() << "RPC response received:" << rp.toCpon();
	}
	else if(msg.isNotify()) {
		cp::RpcNotify nt(msg);
		shvInfo() << "RPC notify received:" << nt.toCpon();
	}
}

void Revitest::onLublicatorPropertyValueChanged(const std::string &property_name, const shv::chainpack::RpcValue &new_val)
{
	cp::RpcNotify ntf;
	ntf.setMethod(cp::Rpc::NTF_VAL_CHANGED);
	ntf.setParams(new_val);
	ntf.setShvPath(property_name);
	//shvInfo() << "LublicatorPropertyValueChanged:" << ntf.toCpon();
	emit sendRpcMessage(ntf);
}

