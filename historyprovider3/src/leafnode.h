#pragma once

#include "logtype.h"

#include <shv/iotqt/node/shvnode.h>
#include <shv/iotqt/node/localfsnode.h>

class LeafNode : public shv::iotqt::node::ShvNode
{
	Q_OBJECT

	using Super = shv::iotqt::node::ShvNode;

public:
	LeafNode(const std::string& node_id, const LogType log_type, ShvNode* parent = nullptr);

	size_t methodCount(const StringViewList& shv_path) override;
	const shv::chainpack::MetaMethod* metaMethod(const StringViewList& shv_path, size_t ix) override;
	shv::chainpack::RpcValue callMethod(const StringViewList& shv_path, const std::string& method, const shv::chainpack::RpcValue& params, const shv::chainpack::RpcValue& user_id) override;
	void sanitizeSize();
	qint64 calculateCacheDirSize() const;

private:
	std::string m_journalCacheDir;
	LogType m_logType;
};
