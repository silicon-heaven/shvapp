#pragma once

#include <shv/iotqt/node/shvnode.h>

class SiteItem;

class RootNode : public shv::iotqt::node::ShvRootNode
{
    Q_OBJECT
	using Super = shv::iotqt::node::ShvRootNode;

public:
	explicit RootNode(QObject *parent = nullptr);

	size_t methodCount(const StringViewList &shv_path) override;
	const shv::chainpack::MetaMethod* metaMethod(const StringViewList &shv_path, size_t ix) override;

	shv::chainpack::RpcValue callMethod(const StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params) override;
	shv::chainpack::RpcValue ls(const shv::core::StringViewList &shv_path, const shv::chainpack::RpcValue &params) override;

	shv::chainpack::RpcValue getLog(const QString &shv_path, const shv::chainpack::RpcValue &params) const;
	void onRpcMessageReceived(const shv::chainpack::RpcMessage &msg);

private:
	shv::chainpack::RpcValue ls(const shv::core::StringViewList &shv_path, size_t index, const SiteItem *site_item);
	void trimDirtyLog(const QString &shv_path);
	shv::chainpack::RpcValue getStartTS(const QString &shv_path);
	const std::vector<shv::chainpack::MetaMethod> &metaMethods(const StringViewList &shv_path);
};
