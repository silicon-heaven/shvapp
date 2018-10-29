#include "revitestdevice.h"
#include "lublicator.h"
#include "historynode.h"

#include <shv/iotqt/utils/fileshvjournal.h>
#include <shv/iotqt/node//shvnodetree.h>
#include <shv/coreqt/log.h>

#include <shv/chainpack/rpcmessage.h>
#include <shv/chainpack/metamethod.h>
#include <shv/core/string.h>
#include <shv/core/stringview.h>
#include <shv/core/exception.h>

namespace cp = shv::chainpack;
namespace iot = shv::iotqt;

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
RevitestDevice::RevitestDevice(QObject *parent)
	: QObject(parent)
{
	shvLogFuncFrame();
	m_shvTree = new shv::iotqt::node::ShvNodeTree(this);
	m_shvTree->root()->setSortedChildren(false);
	new HistoryNode(m_shvTree->root());
	createDevices();
}

void RevitestDevice::createDevices()
{
	static constexpr size_t LUB_CNT = 27;
	for (size_t i = 0; i < LUB_CNT; ++i) {
		auto *nd = new Lublicator(std::to_string(i+1), m_shvTree->root());
		connect(nd, &Lublicator::propertyValueChanged, this, &RevitestDevice::onLublicatorPropertyValueChanged);
	}
}

void RevitestDevice::onRpcMessageReceived(const shv::chainpack::RpcMessage &msg)
{
	shvLogFuncFrame() << msg.toCpon();
	if(msg.isRequest()) {
		cp::RpcRequest rq(msg);
		shvInfo() << "RPC request received:" << rq.toPrettyString();
		if(m_shvTree->root()) {
			m_shvTree->root()->handleRpcRequest(rq);
		}
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

void RevitestDevice::getSnapshot(std::vector<shv::iotqt::utils::ShvJournalEntry> &snapshot)
{
	for(const auto &id : m_shvTree->root()->childNames()) {
		Lublicator *nd = qobject_cast<Lublicator*>(m_shvTree->root()->childNode(id));
		if(!nd)
			continue;
		shv::iotqt::utils::ShvJournalEntry e;
		e.path = id + "/status";
		e.value = nd->callMethod(shv::iotqt::node::ShvNode::StringViewList{shv::iotqt::node::ShvNode::StringView("status")}, cp::Rpc::METH_GET, cp::RpcValue());
		snapshot.emplace_back(std::move(e));
	}
}

void RevitestDevice::onLublicatorPropertyValueChanged(const std::string &property_name, const shv::chainpack::RpcValue &new_val)
{
	cp::RpcNotify ntf;
	ntf.setMethod(cp::Rpc::NTF_VAL_CHANGED);
	ntf.setParams(new_val);
	ntf.setShvPath(property_name);
	//shvInfo() << "LublicatorPropertyValueChanged:" << ntf.toCpon();
	emit sendRpcMessage(ntf);
}

