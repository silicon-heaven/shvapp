#include "lublicator.h"

#include <shv/chainpack/rpcmessage.h>
//#include <shv/chainpack/rpcdriver.h>
#include <shv/core/string.h>
#include <shv/core/shvexception.h>

#include <shv/coreqt/log.h>

namespace cp = shv::chainpack;

namespace {
const ShvNode::String S_STATUS = "status";
const ShvNode::String S_NAME = "name";
const ShvNode::String S_BATT_LOW = "batteryLimitLow";
const ShvNode::String S_BATT_HI = "batteryLimitHigh";
const ShvNode::String S_BATT_LEVSIM = "batteryLevelSimulation";

static const std::string ODPOJOVACE_PATH = "/shv/eu/pl/lublin/odpojovace/";
}

Lublicator::Lublicator(QObject *parent)
	: Super(parent)
{
	m_properties[S_STATUS] = cp::RpcValue::UInt(0);
	m_properties[S_NAME] = "";
	m_properties[S_BATT_HI] = cp::RpcValue::Decimal(245, 1);
	m_properties[S_BATT_LOW] = cp::RpcValue::Decimal(232, 1);
	m_properties[S_BATT_LEVSIM] = cp::RpcValue::Decimal(240, 1);
	m_properties["cmdPosOn"];
	m_properties["cmdPosOff"];
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

ShvNode::StringList Lublicator::propertyNames() const
{
	static ShvNode::StringList keys;
	if(keys.empty()) for(const auto &imap: m_properties)
		keys.push_back(imap.first);
	return keys;
}

shv::chainpack::RpcValue Lublicator::propertyValue(const std::string &property_name) const
{
	if(property_name == S_NAME)
		return objectName().toStdString();
	auto it = m_properties.find(property_name);
	if(it == m_properties.end())
		return cp::RpcValue();
	return it->second;
}

bool Lublicator::setPropertyValue(const std::string &property_name, const shv::chainpack::RpcValue &val)
{
	if(property_name == S_STATUS)
		return false;
	if(property_name == S_NAME)
		return false;
	if(property_name == "cmdPosOn") {
		unsigned stat = status();
		stat &= ~(unsigned)Status::PosOff;
		stat |= (unsigned)Status::PosMiddle;
		setStatus(stat);
		stat &= ~(unsigned)Status::PosMiddle;
		stat |= (unsigned)Status::PosOn;
		setStatus(stat);
		return true;
	}
	if(property_name == "cmdPosOff") {
		unsigned stat = status();
		stat &= ~(unsigned)Status::PosOn;
		stat |= (unsigned)Status::PosMiddle;
		setStatus(stat);
		stat &= ~(unsigned)Status::PosMiddle;
		stat |= (unsigned)Status::PosOff;
		setStatus(stat);
		return true;
	}
	auto it = m_properties.find(property_name);
	if(it == m_properties.end())
		return false;
	it->second = val;
	{
		unsigned stat = status();
		int batt_hi = m_properties.at(S_BATT_HI).toDecimal().mantisa();
		int batt_lo = m_properties.at(S_BATT_LOW).toDecimal().mantisa();
		int batt_lev = m_properties.at(S_BATT_LEVSIM).toDecimal().mantisa();
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
	for (size_t i = 0; i < LUB_CNT; ++i) {
		m_lublicators[i].setObjectName(QString::number(i+1));
		connect(&m_lublicators[i], &Lublicator::propertyValueChanged, this, &Revitest::onLublicatorPropertyValueChanged);
	}
}

// /shv/eu/pl/lublin/odpojovace/1..27/properties
void Revitest::onRpcMessageReceived(const shv::chainpack::RpcMessage &msg)
{
	shvLogFuncFrame() << msg.toStdString();
	if(msg.isRequest()) {
		cp::RpcRequest rq(msg);
		std::string method = rq.method();
		//const cp::RpcValue::Map params = rq.params().toMap();
		cp::RpcResponse rsp{};
		rsp.setId(rq.id());
		cp::RpcValue result;

		try {
			bool is_get = method == cp::RpcMessage::METHOD_GET;
			bool is_set = method == cp::RpcMessage::METHOD_SET;

			const cp::RpcValue::String str_path = rq.shvPath().toString();

			std::vector<std::string> path = shv::core::String::split(str_path, '/');
			static const std::vector<std::string> odpojovace_path = shv::core::String::split(ODPOJOVACE_PATH, '/');

			auto lublicator_for_path = [this](const std::vector<std::string> &path) -> Lublicator&
			{
				auto ix = QString::fromStdString(path[odpojovace_path.size()]).toUInt();
				if(ix <= 0)
					SHV_EXCEPTION("invalid lublicator path:" + shv::core::String::join(path, '/'));
				ix--;
				if(ix >= LUB_CNT)
					SHV_EXCEPTION("invalid lublicator path:" + shv::core::String::join(path, '/'));
				return m_lublicators[ix];
			};

			if(is_set) {
				if(!shv::core::String::startsWith(str_path, ODPOJOVACE_PATH))
					SHV_EXCEPTION("cannot write on path:" + str_path);
				if(path.size() != odpojovace_path.size() + 2)
					SHV_EXCEPTION("cannot write on path:" + str_path);
				Lublicator &lubl = lublicator_for_path(path);
				cp::RpcValue val = rq.params();
				std::string property_name = path[odpojovace_path.size() + 1];
				bool ok = lubl.setPropertyValue(property_name, val);
				if(!ok)
					SHV_EXCEPTION("cannot write property value on path:" + str_path);
				result = true;
			}
			else if(is_get) do {
				if(path.empty()) {
					result = cp::RpcValue::List{odpojovace_path[0]};
					break;
				}
				size_t i;
				for (i = 0; i < path.size(); ++i) {
					if(i < odpojovace_path.size()) {
						if(path[i] != odpojovace_path[i])
							SHV_EXCEPTION("invalid path:" + str_path);
						if(i == path.size() - 1) {
							if(i == odpojovace_path.size() - 1) {
								if(is_get) {
									auto lst = cp::RpcValue::List{};
									for (size_t i = 0; i < LUB_CNT; ++i)
										lst.push_back(std::to_string(i + 1));
									result = lst;
									break;
								}
								SHV_EXCEPTION("cannot write on path:" + str_path);
							}
							else {
								result = cp::RpcValue::List{odpojovace_path[i+1]};
								break;
							}
						}
					}
					else {
						auto ix = QString::fromStdString(path[odpojovace_path.size()]).toUInt();
						if(ix == 0)
							SHV_EXCEPTION("invalid path:" + str_path);
						ix--;
						if(ix >= LUB_CNT)
							SHV_EXCEPTION("invalid path:" + str_path);
						auto &lubl = m_lublicators[ix];
						if(path.size() == odpojovace_path.size() + 1) {
							if(is_get) {
								result = lubl.propertyNames();
								break;
							}
							SHV_EXCEPTION("cannot write on path:" + str_path);
						}
						else if(path.size() == odpojovace_path.size() + 2) {
							std::string property_name = path[odpojovace_path.size() + 1];
							Lublicator &lubl = lublicator_for_path(path);
							cp::RpcValue val = lubl.propertyValue(property_name);
							if(!val.isValid())
								SHV_EXCEPTION("cannot read property on path:" + str_path);
							result = val;
							break;
						}
						SHV_EXCEPTION("invalid path:" + str_path);
					}
				}
			} while(false);
			else
				SHV_EXCEPTION("invalid method name:" + method);
		}
		catch(shv::core::Exception &e) {
			rsp.setError(cp::RpcResponse::Error::createError(cp::RpcResponse::Error::MethodInvocationException, e.message()));
		}
		if(!rsp.isError()) {
			rsp.setResult(result);
		}
		shvDebug() << "sending RPC response:" << rsp.toStdString();
		emit sendRpcMessage(rsp);
	}
	else if(msg.isResponse()) {
		cp::RpcResponse rp(msg);
		shvInfo() << "RPC response received:" << rp.toStdString();
	}
	else if(msg.isNotify()) {
		cp::RpcNotify nt(msg);
		//if(nt.method() == shv::chainpack::Rpc::KNOCK_KNOCK) {
			//const shv::chainpack::RpcValue::Map m = ntf.params().toMap();
		//}
		shvInfo() << "RPC notify received:" << nt.toStdString();
}
}

void Revitest::onLublicatorPropertyValueChanged(const std::string &property_name, const shv::chainpack::RpcValue &new_val)
{
	cp::RpcNotify ntf;
	ntf.setMethod(cp::RpcMessage::METHOD_VALUE_CHANGED);
	ntf.setParams(new_val);
	ntf.setShvPath(ODPOJOVACE_PATH + property_name);
	shvInfo() << "LublicatorPropertyValueChanged:" << ntf.toStdString();
	emit sendRpcMessage(ntf);
}

