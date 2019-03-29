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

	shv::chainpack::RpcValue valueOnPath(const StringViewList &shv_path, bool throv_exc = true) override;
	Q_SIGNAL void configSaved();
protected:
	HNode* parentHNode();
	void loadValues() override;
	bool saveValues() override;
	void setValueOnPath(const StringViewList &shv_path, const shv::chainpack::RpcValue &val) override;
	void setValueOnPathR(const StringViewList &shv_path, const shv::chainpack::RpcValue &val);
	void setValueOnPath_helper(const StringViewList &path, size_t key_ix, shv::chainpack::RpcValue parent_map, const shv::chainpack::RpcValue &val);

	size_t methodCount(const StringViewList &shv_path) override;
	const shv::chainpack::MetaMethod *metaMethod(const StringViewList &shv_path, size_t ix) override;
	shv::chainpack::RpcValue callMethod(const StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params) override;
protected:
	shv::chainpack::RpcValue m_newValues;
};

#endif // HNODECONFIGNODE_H
