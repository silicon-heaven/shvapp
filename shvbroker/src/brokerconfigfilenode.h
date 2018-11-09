#ifndef BROKERCONFIGFILENODE_H
#define BROKERCONFIGFILENODE_H

#include <shv/iotqt/node/shvnode.h>

class EtcAclNode : public shv::iotqt::node::ShvNode
{
	using Super = shv::iotqt::node::ShvNode;
public:
	EtcAclNode(shv::iotqt::node::ShvNode *parent = nullptr);
	//~EtcAclNode() override;
};

class RpcValueMapNode : public shv::iotqt::node::ShvNode
{
	using Super = shv::iotqt::node::ShvNode;
public:
	RpcValueMapNode(const std::string &node_id, shv::iotqt::node::ShvNode *parent = nullptr);
	//~RpcValueMapNode() override;

	size_t methodCount(const StringViewList &shv_path) override;
	const shv::chainpack::MetaMethod* metaMethod(const StringViewList &shv_path, size_t ix) override;

	StringList childNames(const ShvNode::StringViewList &shv_path) override;
	shv::chainpack::RpcValue hasChildren(const StringViewList &shv_path) override;
protected:
	virtual shv::chainpack::RpcValue loadConfig() = 0;
	const shv::chainpack::RpcValue &config();
	virtual shv::chainpack::RpcValue valueOnPath(const StringViewList &shv_path);
	void setValueOnPath(const StringViewList &shv_path, const shv::chainpack::RpcValue &val);
	bool isDir(const StringViewList &shv_path);
protected:
	bool m_valuesLoaded = false;
	shv::chainpack::RpcValue m_config;
};

class BrokerConfigFileNode : public RpcValueMapNode
{
	using Super = RpcValueMapNode;
public:
	BrokerConfigFileNode(const std::string &config_name, shv::iotqt::node::ShvNode *parent = nullptr);
	//~BrokerConfigFileNode() override;

	shv::chainpack::RpcValue callMethod(const StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params) override;
protected:
	shv::chainpack::RpcValue loadConfig() override;
};

class AclPathsConfigFileNode : public BrokerConfigFileNode
{
	using Super = BrokerConfigFileNode;
public:
	AclPathsConfigFileNode(shv::iotqt::node::ShvNode *parent = nullptr)
		: Super("paths", parent) {}
	//~BrokerConfigFileNode() override;

	//size_t methodCount(const StringViewList &shv_path) override  {return Super::methodCount(rewriteShvPath(shv_path));}
	//const shv::chainpack::MetaMethod* metaMethod(const StringViewList &shv_path, size_t ix) override {return Super::metaMethod(rewriteShvPath(shv_path), ix);}

	StringList childNames(const ShvNode::StringViewList &shv_path) override;
	//shv::chainpack::RpcValue hasChildren(const StringViewList &shv_path) override {return Super::hasChildren(rewriteShvPath(shv_path));}
protected:
	shv::chainpack::RpcValue valueOnPath(const StringViewList &shv_path) override;
private:
	//StringViewList rewriteShvPath(const StringViewList &shv_path);
};

#endif
