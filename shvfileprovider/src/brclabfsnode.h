#ifndef FILEPROVIDERLOCALFSNODE_H
#define FILEPROVIDERLOCALFSNODE_H

#include <shv/iotqt/node/localfsnode.h>

class AppCliOptions;

class BrclabFsNode : public shv::iotqt::node::LocalFSNode
{
	Q_OBJECT

	using Super = shv::iotqt::node::LocalFSNode;
public:
	BrclabFsNode(const QString &root_path, Super *parent = nullptr);

	shv::chainpack::RpcValue callMethod(const StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params) override;
	size_t methodCount(const StringViewList &shv_path) override;
	const shv::chainpack::MetaMethod* metaMethod(const StringViewList &shv_path, size_t ix) override;

private:
	shv::chainpack::RpcValue ndReadBrclabSummary(const shv::iotqt::node::ShvNode::StringViewList &shv_path, const shv::chainpack::RpcValue &methods_params);
	shv::chainpack::RpcValue ndLsMeta(const shv::iotqt::node::ShvNode::StringViewList &shv_path, const shv::chainpack::RpcValue &methods_params);
	shv::chainpack::RpcValue readMetaData(const QString &file);
};

#endif // FILEPROVIDERLOCALFSNODE_H
