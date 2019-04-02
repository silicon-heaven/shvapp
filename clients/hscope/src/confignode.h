#ifndef HNODECONFIGNODE_H
#define HNODECONFIGNODE_H

#include <shv/iotqt/node/shvnode.h>

class HNode;

class ConfigNode : public shv::iotqt::node::RpcValueMapNode
{
	Q_OBJECT

	using Super = shv::iotqt::node::RpcValueMapNode;
public:
	ConfigNode(HNode *parent);

	//shv::chainpack::RpcValue valueOnPath(const StringViewList &shv_path, bool throv_exc = true) override;
	Q_SIGNAL void configSaved();
protected:
	HNode* parentHNode();

	shv::chainpack::RpcValue loadConfigTemplate(const std::string &file_name);

	void loadValues() override;
	void saveValues() override;

	size_t methodCount(const StringViewList &shv_path) override;
	const shv::chainpack::MetaMethod *metaMethod(const StringViewList &shv_path, size_t ix) override;
	shv::chainpack::RpcValue callMethod(const StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params) override;
protected:
	shv::chainpack::RpcValue m_templateValues;
};

#endif // HNODECONFIGNODE_H
