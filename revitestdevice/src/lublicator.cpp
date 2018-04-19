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
	//m_properties[S_STATUS] = cp::RpcValue::UInt(0);
	//m_properties[S_NAME] = "";
//	m_properties[S_BATT_HI] = cp::RpcValue::Decimal(245, 1);
//	m_properties[S_BATT_LOW] = cp::RpcValue::Decimal(232, 1);
//	m_properties[S_BATT_LEVSIM] = cp::RpcValue::Decimal(240, 1);
//	m_properties[S_CMD_POS_ON];
//	m_properties[S_CMD_POS_OFF];
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
		emit propertyValueChanged(nodeId() + '/' + PROP_STATUS, stat);
		return true;
	}
	return false;
}

static std::vector<cp::MetaMethod> meta_methods {
	{cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, false},
	{cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, false},
	{Lublicator::METH_DEVICE_ID, cp::MetaMethod::Signature::RetVoid, false},
	{Lublicator::METH_CMD_ON, cp::MetaMethod::Signature::VoidVoid, false},
	{Lublicator::METH_CMD_OFF, cp::MetaMethod::Signature::VoidVoid, false},
	//{Lublicator::METH_GET_LOG, cp::MetaMethod::Signature::RetParam, false},
};

size_t Lublicator::methodCount(const std::string &shv_path)
{
	if(shv_path.empty())
		return 2;
	return meta_methods.size();
}

const shv::chainpack::MetaMethod *Lublicator::metaMethod(size_t ix, const std::string &shv_path)
{
	Q_UNUSED(shv_path)
	return &(meta_methods.at(ix));
}

shv::iotqt::node::ShvNode::StringList Lublicator::childNames(const std::string &shv_path)
{
	if(shv_path.empty())
		return shv::iotqt::node::ShvNode::StringList{PROP_STATUS};
	return shv::iotqt::node::ShvNode::StringList{};
}

shv::chainpack::RpcValue Lublicator::call(const std::string &method, const shv::chainpack::RpcValue &params, const std::string &shv_path)
{
	if(shv_path.empty()) {
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
	}
	return Super::call(method, params, shv_path);
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
		cp::RpcResponse resp = cp::RpcResponse::forRequest(rq.metaData());
		try {
			//shvInfo() << "RPC request received:" << rq.toCpon();
			const cp::RpcValue shv_path = rq.shvPath();
			std::string path_rest;
			shv::iotqt::node::ShvNode *nd = m_devices->cd(shv_path.toString(), &path_rest);
			if(!nd)
				SHV_EXCEPTION("Path not found: " + shv_path.toString());
			rq.setShvPath(path_rest);
			shv::chainpack::RpcValue result = nd->processRpcRequest(rq);
			if(result.isValid())
				resp.setResult(result);
			else
				return;
		}
		catch (shv::core::Exception &e) {
			resp.setError(cp::RpcResponse::Error::create(cp::RpcResponse::Error::MethodInvocationException, e.message()));
		}
		if(resp.requestId().toInt() > 0) // RPC calls with requestID == 0 does not expect response
			emit sendRpcMessage(resp);
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
#if 0
void Revitest::onRpcMessageReceived(const shv::chainpack::RpcMessage &msg)
{
	shvLogFuncFrame() << msg.toCpon();
	if(msg.isRequest()) {
		cp::RpcRequest rq(msg);
		cp::RpcResponse rsp = cp::RpcResponse::forRequest(rq);
		cp::RpcValue result;

		try {
			cp::RpcValue shv_path = rq.shvPath();
			std::string path_rest;
			shv::iotqt::node::ShvNode *nd = m_devices->cd(shv_path.toString(), &path_rest);
			//shvInfo() << "path:" << path << "->" << nd << "path rest:" << path_rest;
			if(!nd)
				SHV_EXCEPTION("invalid path: " + shv_path.toString());
			if(shv_path.toString().empty()) {
				result = nd->processRpcRequest(rq);
			}
			else {
				Lublicator *lubl = qobject_cast<Lublicator*>(nd);
				if(!lubl)
					SHV_EXCEPTION("invalid lublicator: " + shv_path.toString());
				cp::RpcValue method = rq.method();
				if(method.toString() == cp::Rpc::METH_LS) {
					if(path_rest.empty())
						result = lubl->childNames();
				}
				else if(method.toString() == cp::Rpc::METH_DIR) {
					result = lubl->propertyMethods(path_rest);
				}
				else if(method.toString() == cp::Rpc::METH_GET) {
					result = lubl->propertyValue(path_rest);
				}
				else if(method.toString() == cp::Rpc::METH_SET) {
					result = lubl->setPropertyValue(path_rest, rq.params());
				}
				else if(method.toString() == M_CMD_POS_ON) {
					unsigned stat = lubl->status();
					stat &= ~(unsigned)Status::PosOff;
					stat |= (unsigned)Status::PosMiddle;
					lubl->setStatus(stat);
					stat &= ~(unsigned)Status::PosMiddle;
					stat |= (unsigned)Status::PosOn;
					lubl->setStatus(stat);
					result = true;
				}
				else if(method.toString() == M_CMD_POS_OFF) {
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
					SHV_EXCEPTION("Invalid method: " + method.toString() + " called for node: " + shv_path.toString());
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
#endif
void Revitest::onLublicatorPropertyValueChanged(const std::string &property_name, const shv::chainpack::RpcValue &new_val)
{
	cp::RpcNotify ntf;
	ntf.setMethod(cp::Rpc::NTF_VAL_CHANGED);
	ntf.setParams(new_val);
	ntf.setShvPath(property_name);
	//shvInfo() << "LublicatorPropertyValueChanged:" << ntf.toCpon();
	emit sendRpcMessage(ntf);
}

