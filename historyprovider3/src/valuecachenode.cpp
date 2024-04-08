#include <iostream>
#include "valuecachenode.h"
#include "historyapp.h"

#include <shv/iotqt/rpc/deviceconnection.h>
#include <shv/iotqt/rpc/rpccall.h>

#define cacheDebug() shvCDebug("valuecache")
#define cacheInfo() shvCInfo("valuecache")
#define cacheWarning() shvCWarning("valuecache")

namespace {
const auto METH_GET_CACHE = "getCache";
}
namespace cp = shv::chainpack;
const std::vector<cp::MetaMethod> methods {
	{cp::Rpc::METH_GET, cp::MetaMethod::Flag::None, "String", "RpcValue", cp::AccessLevel::Devel},
	{METH_GET_CACHE, cp::MetaMethod::Flag::None, {}, "Map", cp::AccessLevel::Devel},
};

ValueCacheNode::ValueCacheNode(ShvNode* parent)
	: Super("_valuecache", parent)
{
	auto conn = HistoryApp::instance()->rpcConnection();

	connect(conn, &shv::iotqt::rpc::ClientConnection::rpcMessageReceived, this, &ValueCacheNode::onRpcMessageReceived);
}

void ValueCacheNode::saveToCache(const std::string& path, const shv::chainpack::RpcValue& value)
{
	m_cache.insert_or_assign(path, value);
	emit valueChanged(path, value);
}

void ValueCacheNode::onRpcMessageReceived(const cp::RpcMessage &msg)
{
	if (msg.isSignal()) {
		cp::RpcSignal ntf(msg);
		auto path = ntf.shvPath().asString();
		auto method = ntf.method().asString();
		auto params = ntf.params();

		if (method == shv::chainpack::Rpc::SIG_VAL_CHANGED) {
			saveToCache(path, params);
		}

		if (method == shv::chainpack::Rpc::SIG_MOUNTED_CHANGED) {
			std::erase_if(m_cache, [&path] (const auto& key) {
				return key.first.starts_with(path);
			});
		}

	}
}

size_t ValueCacheNode::methodCount(const StringViewList& shv_path)
{
	return Super::methodCount(shv_path) + methods.size();
}

const cp::MetaMethod* ValueCacheNode::metaMethod(const StringViewList& shv_path, size_t index)
{
	static auto super_method_count = Super::methodCount(shv_path);

	if (index >= super_method_count) {
		return &methods.at(index - super_method_count);
	}

	return Super::metaMethod(shv_path, index);
}

cp::RpcValue ValueCacheNode::callMethodRq(const cp::RpcRequest &rq)
{
	auto method = rq.method().asString();
	if (method == cp::Rpc::METH_GET) {
		if (!rq.params().isString()) {
			auto resp = rq.makeResponse();
			resp.setError(shv::chainpack::RpcError::createMethodCallExceptionError("Invalid param. Expected string."));
			HistoryApp::instance()->rpcConnection()->sendRpcMessage(resp);
			return {};
		}

		auto param_shv_path = rq.params().asString();
		if (m_cache.hasKey(param_shv_path)) {
			return m_cache.at(param_shv_path);
		}

		auto call = shv::iotqt::rpc::RpcCall::create(HistoryApp::instance()->rpcConnection())
			->setShvPath(param_shv_path)
			->setMethod(shv::chainpack::Rpc::METH_GET);
		connect(call, &shv::iotqt::rpc::RpcCall::maybeResult, this, [this, rq = rq, param_shv_path] (const shv::chainpack::RpcValue& result, const shv::chainpack::RpcError& error) {
			auto resp = rq.makeResponse();
			if (error.isValid()) {
				resp.setError(error);
				HistoryApp::instance()->rpcConnection()->sendRpcMessage(resp);
				return;
			}

			saveToCache(param_shv_path, result);
			resp.setResult(result);
			HistoryApp::instance()->rpcConnection()->sendRpcMessage(resp);
		});
		call->start();
		return {};
	}

	if (method == METH_GET_CACHE) {
		return m_cache;
	}

	return Super::callMethodRq(rq);
}
