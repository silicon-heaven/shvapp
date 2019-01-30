#ifndef FILEPROVIDERLOCALFSNODE_H
#define FILEPROVIDERLOCALFSNODE_H

#include <shv/iotqt/node/localfsnode.h>
class AppCliOptions;

class FileProviderLocalFsNode : public shv::iotqt::node::LocalFSNode
{
	Q_OBJECT

	using Super = shv::iotqt::node::LocalFSNode;
public:
	FileProviderLocalFsNode(const QString &root_path, Super *parent = nullptr);

	shv::chainpack::RpcValue callMethod(const StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params) override;
	size_t methodCount(const StringViewList &shv_path) override;
	const shv::chainpack::MetaMethod* metaMethod(const StringViewList &shv_path, size_t ix) override;
	shv::chainpack::RpcValue processRpcRequest(const shv::chainpack::RpcRequest &rq) override;

private:
	shv::chainpack::RpcValue ndReadBrclab(const shv::iotqt::node::ShvNode::StringViewList &shv_path, const shv::chainpack::RpcValue &methods_params);
	shv::chainpack::RpcValue ndLsMeta(const shv::iotqt::node::ShvNode::StringViewList &shv_path, const shv::chainpack::RpcValue &methods_params);
	shv::chainpack::RpcValue readMetaData(const QString &file);
};

#endif // FILEPROVIDERLOCALFSNODE_H
