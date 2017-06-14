#include "rpcmessage.h"
#include "chainpackprotocol.h"
#include "metatypes.h"

#include <cassert>

namespace shv {
namespace core {
namespace chainpack {

RpcMessage::RpcMessage(const RpcValue &val)
{
	assert(val.isIMap());
	m_value = val;
}

bool RpcMessage::hasKey(RpcValue::UInt key) const
{
	return m_value.toIMap().count(key);
}

RpcValue RpcMessage::value(RpcValue::UInt key) const
{
	return m_value.at(key);
}

void RpcMessage::setValue(RpcValue::UInt key, const RpcValue &val)
{
	assert(key >= Key::Id && key < Key::MAX_KEY);
	ensureMetaValues();
	m_value.set(key, val);
}

RpcValue::UInt RpcMessage::id() const
{
	return value(Key::Id).toUInt();
}

void RpcMessage::setId(RpcValue::UInt id)
{
	setValue(Key::Id, RpcValue{id});
}

bool RpcMessage::isRequest() const
{
	return hasKey(Key::Method);
}

bool RpcMessage::isNotify() const
{
	return isRequest() && !hasKey(Key::Id);
}

bool RpcMessage::isResponse() const
{
	return hasKey(Key::Id) && (hasKey(Key::Result) || hasKey(Key::Error));
}

int RpcMessage::write(std::ostream &out) const
{
	assert(m_value.isValid());
	return ChainPackProtocol::write(out, m_value);
}

void RpcMessage::ensureMetaValues()
{
	if(!m_value.isValid()) {
		m_value = RpcValue::IMap();
		m_value.setMetaValue(RpcValue::Tag::MetaTypeId, GlobalMetaTypeId::ChainPackRpcMessage);
		/// not needed, Global is default name space
		//m_value.setMetaValue(Value::Tag::MetaTypeNameSpaceId, MetaTypeNameSpaceId::Global);
	}
}

RpcValue::String RpcRequest::method() const
{
	return value(Key::Method).toString();
}

RpcRequest &RpcRequest::setMethod(RpcValue::String &&met)
{
	setValue(Key::Method, RpcValue{std::move(met)});
	return *this;
}

RpcValue RpcRequest::params() const
{
	return value(Key::Params);
}

RpcRequest& RpcRequest::setParams(const RpcValue& p)
{
	setValue(Key::Params, p);
	return *this;
}

RpcResponse::Error RpcResponse::error() const
{
	return Error{value(Key::Error).toIMap()};
}

RpcResponse &RpcResponse::setError(RpcResponse::Error &&err)
{
	setValue(Key::Error, std::move(err));
	return *this;
}

RpcValue RpcResponse::result() const
{
	return value(Key::Result);
}

RpcResponse& RpcResponse::setResult(const RpcValue& res)
{
	setValue(Key::Result, res);
	return *this;
}

RpcResponse::Error::ErrorType RpcResponse::Error::code() const
{
	auto iter = find(KeyCode);
	return (iter == end()) ? NoError : (ErrorType)(iter->second.toUInt());
}

RpcResponse::Error& RpcResponse::Error::setCode(ErrorType c)
{
	(*this)[KeyCode] = RpcValue{(RpcValue::UInt)c};
	return *this;
}

RpcResponse::Error& RpcResponse::Error::setMessage(RpcValue::String &&mess)
{
	(*this)[KeyMessage] = RpcValue{std::move(mess)};
	return *this;
}

RpcValue::String RpcResponse::Error::message() const
{
	auto iter = find(KeyMessage);
	return (iter == end()) ? RpcValue::String{} : iter->second.toString();
}

RpcValue::String RpcMessage::toString() const
{
	return m_value.dumpText();
}

} // namespace chainpackrpc
}
} // namespace shv
