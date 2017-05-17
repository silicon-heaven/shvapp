#include "rpcmessage.h"

#include <cassert>

namespace shv {
namespace chainpackrpc {

RpcMessage::RpcMessage(const Value &val)
{
	assert(val.isMap());
	m_value = val;
}

bool RpcMessage::hasKey(Value::UInt key) const
{
	return m_value.toIMap().count(key);
}

Value RpcMessage::value(Value::UInt key) const
{
	return m_value.at(key);
}

void RpcMessage::setValue(Value::UInt key, const Value &val)
{
	assert(key >= Key::Id && key < Key::MAX_KEY);
	m_value.set(key, val);
}

Value::UInt RpcMessage::id() const
{
	return value(Key::Id).toUInt();
}

void RpcMessage::setId(Value::UInt id)
{
	setValue(Key::Id, Value{id});
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
	return hasKey(Key::Id) && (hasKey(Key::Result) || hasKey(Key::Result));
}

Value::String RpcRequest::method() const
{
	return value(Key::Method).toString();
}

RpcRequest &RpcRequest::setMethod(Value::String &&met)
{
	setValue(Key::Method, Value{std::move(met)});
	return *this;
}

Value RpcRequest::params() const
{
	return value(Key::Params);
}

RpcRequest& RpcRequest::setParams(const Value& p)
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

Value RpcResponse::result() const
{
	return value(Key::Result);
}

RpcResponse& RpcResponse::setResult(const Value& res)
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
	(*this)[KeyCode] = Value{(Value::UInt)c};
	return *this;
}

RpcResponse::Error& RpcResponse::Error::setMessage(Value::String &&mess)
{
	(*this)[KeyMessage] = Value{std::move(mess)};
	return *this;
}

Value::String RpcResponse::Error::message() const
{
	auto iter = find(KeyMessage);
	return (iter == end()) ? Value::String{} : iter->second.toString();
}

Value::String RpcMessage::toString() const
{
	return m_value.dumpText();
}

} // namespace chainpackrpc
} // namespace shv
