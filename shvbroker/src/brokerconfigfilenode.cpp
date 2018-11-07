#include "brokerconfigfilenode.h"
#include "brokerapp.h"

#include <shv/chainpack/metamethod.h>
#include <shv/chainpack/rpc.h>

namespace cp = shv::chainpack;

EtcAclNode::EtcAclNode(shv::iotqt::node::ShvNode *parent)
	: Super("acl", parent)
{
	new BrokerConfigFileNode("fstab", this);
	new BrokerConfigFileNode("users", this);
	new BrokerConfigFileNode("grants", this);
	new BrokerConfigFileNode("paths", this);
}

//static const char M_SIZE[] = "size";
static const char M_LOAD[] = "loadFile";
static const char M_SAVE[] = "saveFile";
static const char M_COMMIT[] = "commitChanges";
//static const char M_RELOAD[] = "serverReload";

static std::vector<cp::MetaMethod> meta_methods {
	{cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam},
	{cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam},
	{cp::Rpc::METH_GET, cp::MetaMethod::Signature::RetVoid, 0, cp::Rpc::GRANT_READ},
	{cp::Rpc::METH_SET, cp::MetaMethod::Signature::RetVoid, 0, cp::Rpc::GRANT_CONFIG},
	//{M_WRITE, cp::MetaMethod::Signature::RetParam, 0, cp::MetaMethod::AccessLevel::Service},
};

enum AclAccessLevel {AclUserView = cp::MetaMethod::AccessLevel::Service, AclUserAdmin = cp::MetaMethod::AccessLevel::Admin};

static std::vector<cp::MetaMethod> meta_methods_root {
	{cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam},
	{cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam},
	//{cp::Rpc::METH_ACCESS_LEVELS, cp::MetaMethod::Signature::RetVoid},
	{M_LOAD, cp::MetaMethod::Signature::RetVoid, 0, cp::Rpc::GRANT_SERVICE},
	{M_SAVE, cp::MetaMethod::Signature::RetVoid, 0, cp::Rpc::GRANT_ADMIN},
	{M_COMMIT, cp::MetaMethod::Signature::RetVoid, 0, cp::Rpc::GRANT_ADMIN},
};

RpcValueMapNode::RpcValueMapNode(const std::string &node_id, shv::iotqt::node::ShvNode *parent)
	: Super(node_id, parent)
{
}

size_t RpcValueMapNode::methodCount(const shv::iotqt::node::ShvNode::StringViewList &shv_path)
{
	if(shv_path.empty()) {
		return meta_methods_root.size();
	}
	else {
		return isDir(shv_path)? 2: 4;
	}
}

const shv::chainpack::MetaMethod *RpcValueMapNode::metaMethod(const shv::iotqt::node::ShvNode::StringViewList &shv_path, size_t ix)
{
	size_t size;
	const std::vector<shv::chainpack::MetaMethod> &methods = shv_path.empty()? meta_methods_root: meta_methods;
	if(shv_path.empty()) {
		size = meta_methods_root.size();
	}
	else {
		size = isDir(shv_path)? 2: 4;
	}
	if(size <= ix)
		SHV_EXCEPTION("Invalid method index: " + std::to_string(ix) + " of: " + std::to_string(size));
	return &(methods[ix]);
}

shv::iotqt::node::ShvNode::StringList RpcValueMapNode::childNames(const shv::iotqt::node::ShvNode::StringViewList &shv_path)
{
	shv::chainpack::RpcValue val = valueOnPath(shv_path);
	ShvNode::StringList lst;
	for(const auto &kv : val.toMap()) {
		lst.push_back(kv.first);
	}
	return lst;
}

shv::chainpack::RpcValue RpcValueMapNode::hasChildren(const shv::iotqt::node::ShvNode::StringViewList &shv_path)
{
	return isDir(shv_path);
}

const shv::chainpack::RpcValue &RpcValueMapNode::config()
{
	if(!m_valuesLoaded) {
		m_valuesLoaded = true;
		m_config = loadConfig();
	}
	return m_config;
}

shv::chainpack::RpcValue RpcValueMapNode::valueOnPath(const shv::iotqt::node::ShvNode::StringViewList &shv_path)
{
	shv::chainpack::RpcValue v = config();
	for(const auto & dir : shv_path) {
		const shv::chainpack::RpcValue::Map &m = v.toMap();
		v = m.value(dir.toString());
		if(!v.isValid())
			SHV_EXCEPTION("Invalid path: " + shv::core::StringView::join(shv_path, '/'));
	}
	return v;
}

void RpcValueMapNode::setValueOnPath(const shv::iotqt::node::ShvNode::StringViewList &shv_path, const shv::chainpack::RpcValue &val)
{
	if(shv_path.empty())
		SHV_EXCEPTION("Invalid path: " + shv::core::StringView::join(shv_path, '/'));
	shv::chainpack::RpcValue v = config();
	for (size_t i = 0; i < shv_path.size()-1; ++i) {
		auto dir = shv_path.at(i);
		const shv::chainpack::RpcValue::Map &m = v.toMap();
		v = m.value(dir.toString());
		if(!v.isValid())
			SHV_EXCEPTION("Invalid path: " + shv::core::StringView::join(shv_path, '/'));
	}
	v.set(shv_path.at(shv_path.size() - 1).toString(), val);
}

bool RpcValueMapNode::isDir(const shv::iotqt::node::ShvNode::StringViewList &shv_path)
{
	return valueOnPath(shv_path).isMap();
}

BrokerConfigFileNode::BrokerConfigFileNode(const std::string &config_name, shv::iotqt::node::ShvNode *parent)
	: Super(config_name, parent)
{
}

shv::chainpack::RpcValue BrokerConfigFileNode::callMethod(const shv::iotqt::node::ShvNode::StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params)
{
	if(shv_path.empty()) {
		if(method == M_LOAD) {
			m_config = BrokerApp::instance()->aclConfig(nodeId(), shv::core::Exception::Throw);
			return m_config;
		}
		if(method == M_SAVE) {
			m_config = cp::RpcValue::fromCpon(params.toString());
			BrokerApp::instance()->setAclConfig(nodeId(), m_config, shv::core::Exception::Throw);
			return true;
		}
		if(method == M_COMMIT) {
			BrokerApp::instance()->setAclConfig(nodeId(), m_config, shv::core::Exception::Throw);
			return true;
		}
	}
	if(method == cp::Rpc::METH_GET) {
		shv::chainpack::RpcValue rv = valueOnPath(shv_path);
		return rv;
	}
	if(method == cp::Rpc::METH_SET) {
		setValueOnPath(shv_path, params);
		return true;
	}
	return Super::callMethod(shv_path, method, params);
}

shv::chainpack::RpcValue BrokerConfigFileNode::loadConfig()
{
	return BrokerApp::instance()->aclConfig(nodeId(), !shv::core::Exception::Throw);
}
