#include "brokerconfigfilenode.h"
#include "brokerapp.h"

#include <shv/chainpack/metamethod.h>
#include <shv/chainpack/rpc.h>
#include <shv/core/string.h>
#include <shv/core/log.h>

namespace cp = shv::chainpack;

//========================================================
// EtcAclNode
//========================================================
static std::vector<cp::MetaMethod> meta_methods_acl {
	{cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, 0, cp::Rpc::GRANT_CONFIG},
	{cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, 0, cp::Rpc::GRANT_CONFIG},
};

EtcAclNode::EtcAclNode(shv::iotqt::node::ShvNode *parent)
	: Super("acl", meta_methods_acl, parent)
{
	new BrokerConfigFileNode("fstab", this);
	new BrokerConfigFileNode("users", this);
	new BrokerConfigFileNode("grants", this);
	new AclPathsConfigFileNode(this);
}

enum AclAccessLevel {AclUserView = cp::MetaMethod::AccessLevel::Service, AclUserAdmin = cp::MetaMethod::AccessLevel::Admin};

BrokerConfigFileNode::BrokerConfigFileNode(const std::string &config_name, shv::iotqt::node::ShvNode *parent)
	: Super(config_name, parent)
{
}

shv::chainpack::RpcValue BrokerConfigFileNode::loadValues()
{
	return BrokerApp::instance()->aclConfig(nodeId(), !shv::core::Exception::Throw);
}

bool BrokerConfigFileNode::saveValues(const shv::chainpack::RpcValue &vals)
{
	return BrokerApp::instance()->setAclConfig(nodeId(), vals, shv::core::Exception::Throw);
}

//========================================================
// AclPathsConfigFileNode
//========================================================

shv::iotqt::node::ShvNode::StringList AclPathsConfigFileNode::childNames(const shv::iotqt::node::ShvNode::StringViewList &shv_path)
{
	if(shv_path.size() == 1) {
		std::vector<std::string> keys = values().at(shv_path[0].toString()).toMap().keys();
		for (size_t i = 0; i < keys.size(); ++i)
			keys[i] = '"' + keys[i] + '"';
		return keys;

	}
	else {
		return Super::childNames(shv_path);
	}
}

shv::chainpack::RpcValue AclPathsConfigFileNode::valueOnPath(const shv::iotqt::node::ShvNode::StringViewList &shv_path)
{
	//shvInfo() << "valueOnPath:" << shv_path.join('/');
	shv::chainpack::RpcValue v = values();
	for (size_t i = 0; i < shv_path.size(); ++i) {
		shv::iotqt::node::ShvNode::StringView dir = shv_path[i];
		if(i == 1)
			dir = dir.mid(1, dir.length() - 2);
		const shv::chainpack::RpcValue::Map &m = v.toMap();
		std::string key = dir.toString();
		v = m.value(key);
		shvInfo() << "\t i:" << i << "key:" << key << "val:" << v.toCpon();
		if(!v.isValid())
			SHV_EXCEPTION("Invalid path: " + shv_path.join('/'));
	}
	return v;
}

