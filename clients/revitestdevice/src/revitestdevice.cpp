#include "revitestdevice.h"
#include "lublicator.h"
#include "historynode.h"
#include "revitestapp.h"

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
	for (size_t i = 0; i < RevitestApp::LUB_CNT; ++i) {
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
		{
			shv::iotqt::utils::ShvJournalEntry e;
			e.path = id + '/' + Lublicator::PROP_STATUS;
			e.value = nd->callMethod(shv::iotqt::node::ShvNode::StringViewList{shv::iotqt::node::ShvNode::StringView(Lublicator::PROP_STATUS)}, cp::Rpc::METH_GET, cp::RpcValue());
			snapshot.emplace_back(std::move(e));
		}
		{
			shv::iotqt::utils::ShvJournalEntry e;
			e.path = id + '/' + Lublicator::PROP_BATTERY_VOLTAGE;
			e.value = nd->callMethod(shv::iotqt::node::ShvNode::StringViewList{}, Lublicator::PROP_BATTERY_VOLTAGE, cp::RpcValue());
			snapshot.emplace_back(std::move(e));
		}
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

