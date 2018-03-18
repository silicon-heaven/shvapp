#include "lublicator.h"

#include <shv/chainpack/rpcmessage.h>
#include <shv/core/string.h>
#include <shv/core/stringview.h>
#include <shv/core/exception.h>

#include <shv/coreqt/log.h>

#include <shv/iotqt/shvnodetree.h>

namespace cp = shv::chainpack;
namespace iot = shv::iotqt;

namespace {
const iot::ShvNode::String S_STATUS = "status";
const iot::ShvNode::String S_NAME = "name";
const iot::ShvNode::String S_BATT_LOW = "batteryLimitLow";
const iot::ShvNode::String S_BATT_HI = "batteryLimitHigh";
const iot::ShvNode::String S_BATT_LEVSIM = "batteryLevelSimulation";
const iot::ShvNode::String S_CMD_POS_ON = "cmdPosOn";
const iot::ShvNode::String S_CMD_POS_OFF = "cmdPosOff";

//static const std::string ODPOJOVACE_PATH = "/shv/eu/pl/lublin/odpojovace/";
}

Lublicator::Lublicator(QObject *parent)
	: Super(parent)
{
	m_properties[S_STATUS] = cp::RpcValue::UInt(0);
	m_properties[S_NAME] = "";
	m_properties[S_BATT_HI] = cp::RpcValue::Decimal(245, 1);
	m_properties[S_BATT_LOW] = cp::RpcValue::Decimal(232, 1);
	m_properties[S_BATT_LEVSIM] = cp::RpcValue::Decimal(240, 1);
	m_properties[S_CMD_POS_ON];
	m_properties[S_CMD_POS_OFF];
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

iot::ShvNode::StringList Lublicator::childNodeIds() const
{
	static ShvNode::StringList keys;
	if(keys.empty()) for(const auto &imap: m_properties)
		keys.push_back(imap.first);
	return keys;
}

shv::chainpack::RpcValue::List Lublicator::propertyMethods(const shv::iotqt::ShvNode::String &property_name) const
{
	if(property_name == S_CMD_POS_ON || property_name == S_CMD_POS_OFF) {
		return cp::RpcValue::List{cp::Rpc::METH_SET};
	}
	else if(property_name == S_BATT_HI
			|| property_name == S_BATT_LOW
			|| property_name == S_NAME
			|| property_name == S_STATUS) {
		return cp::RpcValue::List{cp::Rpc::METH_GET};
	}
	return cp::RpcValue::List{cp::Rpc::METH_GET, cp::Rpc::METH_SET};
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

bool Lublicator::setPropertyValue(const shv::iotqt::ShvNode::String &property_name, const shv::chainpack::RpcValue &val)
{
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
	m_devices = new iot::ShvNodeTree(this);
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
			shv::iotqt::ShvNode *nd = m_devices->cd(path, &path_rest);
			//shvInfo() << "path:" << path << "->" << nd << "path rest:" << path_rest;
			if(!nd)
				SHV_EXCEPTION("invalid path: " + path);
			if(path_rest.empty()) {
				result = nd->processRpcRequest(rq);
			}
			else {
				Lublicator *lubl = qobject_cast<Lublicator*>(nd);
				if(!lubl)
					SHV_EXCEPTION("invalid lublicator: " + path);
				shv::chainpack::RpcValue::String method = rq.method();
				if(method == "ls") {
					result = cp::RpcValue();
				}
				else if(method == "dir") {
					result = lubl->propertyMethods(path_rest);
				}
				else if(method == cp::Rpc::METH_GET) {
					result = lubl->propertyValue(path_rest);
				}
				else if(method == cp::Rpc::METH_SET) {
					result = lubl->setPropertyValue(path_rest, rq.params());
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
	ntf.setMethod(cp::Rpc::METH_VAL_CHANGED);
	ntf.setParams(new_val);
	ntf.setShvPath(property_name);
	shvInfo() << "LublicatorPropertyValueChanged:" << ntf.toCpon();
	emit sendRpcMessage(ntf);
}

