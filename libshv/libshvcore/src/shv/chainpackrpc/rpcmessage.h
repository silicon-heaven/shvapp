#pragma once

#include "chainpackvalue.h"

#include "../../shvcoreglobal.h"

namespace shv {
namespace chainpackrpc {

class SHVCORE_DECL_EXPORT RpcMessage
{
public:
	struct Key {
		enum Enum {
			Id = 1,
			Method,
			Params,
			Result,
			Error,
			ErrorCode,
			ErrorMessage,
			MAX_KEY
		};
	};
	struct MetaTypeId {
		enum Enum {
			RpcMessage = 1,
		};
	};
	struct MetaTypeNameSpaceId {
		enum Enum {
			ChainPackRpc = 1,
		};
	};
public:
	RpcMessage() {}
	RpcMessage(const Value &val);
	const Value& value() const {return m_value;}
protected:
	bool hasKey(Value::UInt key) const;
	Value value(Value::UInt key) const;
	void setValue(Value::UInt key, const Value &val);
public:
	Value::UInt id() const;
	void setId(Value::UInt id);
	bool isRequest() const;
	bool isResponse() const;
	bool isNotify() const;
	Value::String toString() const;

	virtual int write(Value::Blob &out) const;
protected:
	void ensureMetaValues();
protected:
	Value m_value;
};

class SHVCORE_DECL_EXPORT RpcRequest : public RpcMessage
{
private:
	using Super = RpcMessage;
public:
	RpcRequest() : Super() {}
	//RpcRequest(const Value &id) : Super(Json()) {setId(id);}
	RpcRequest(const RpcMessage &msg) : Super(msg) {}
public:
	RpcRequest& setMethod(Value::String &&met);
	Value::String method() const;
	RpcRequest& setParams(const Value &p);
	Value params() const;
	RpcRequest& setId(const Value::UInt id) {Super::setId(id); return *this;}

	//int write(Value::Blob &out) const override;
};

class SHVCORE_DECL_EXPORT RpcResponse : public RpcMessage
{
private:
	using Super = RpcMessage;
public:
	class SHVCORE_DECL_EXPORT Error : public Value::IMap
	{
	private:
		using Super = Value::IMap;
		enum {KeyCode = 1, KeyMessage};
	public:
		enum ErrorType {
			NoError = 0,
			InvalidRequest,	// The JSON sent is not a valid Request object.
			MethodNotFound,	// The method does not exist / is not available.
			InvalidParams,		// Invalid method parameter(s).
			InternalError,		// Internal JSON-RPC error.
			ParseError,		// Invalid JSON was received by the server. An error occurred on the server while parsing the JSON text.
			SyncMethodCallTimeout,
			SyncMethodCallCancelled,
			MethodInvocationException,
			Unknown
		};
	public:
		Error(const Super &m = Super()) : Super(m) {}
		Error& setCode(ErrorType c);
		ErrorType code() const;
		Error& setMessage(Value::String &&m);
		Value::String message() const;
		//Error& setData(const Value &data);
		//Value data() const;
		Value::String toString() const {return "RPC ERROR " + std::to_string(code()) + ": " + message();}
	public:
		static Error createError(ErrorType c, Value::String &&msg) {
			Error ret;
			ret.setCode(c).setMessage(std::move(msg));
			return ret;
		}
		/*
		static Error createParseError(const Value::String &msg = Value::String(), const Value &data = Value()) {
			return createError(ParseError,
							   (msg.isEmpty())? "Parse error": msg,
							   (data.isValid())? data: "Invalid JSON was received by the server. An error occurred on the server while parsing the JSON text.");
		}
		static Error createInvalidRequestError(const Value::String &msg = Value::String(), const Value &data = Value()) {
			return createError(InvalidRequest,
							   (msg.isEmpty())? "Invalid Request": msg,
							   (data.isValid())? data: "The JSON sent is not a valid Request object.");
		}
		static Error createMethodNotFoundError(const Value::String &msg = Value::String(), const Value &data = Value()) {
			return createError(MethodNotFound,
							   (msg.isEmpty())? "Method not found": msg,
							   (data.isValid())? data: "The method does not exist / is not available.");
		}
		static Error createInvalidParamsError(const Value::String &msg = Value::String(), const Value &data = Value()) {
			return createError(InvalidParams,
							   (msg.isEmpty())? "Invalid params": msg,
							   (data.isValid())? data: "Invalid method parameter(s).");
		}
		static Error createInternalError(const Value::String &msg = Value::String(), const Value &data = Value()) {
			return createError(InternalError,
							   (msg.isEmpty())? "Internal error": msg,
							   (data.isValid())? data: "Internal JSON-RPC error.");
		}
		static Error createMethodInvocationExceptionError(const Value::String &msg = Value::String(), const Value &data = Value()) {
			return createError(MethodInvocationException, msg, data);
		}
		*/
	};
public:
	//RpcResponse(const Json &json = Json()) : Super(json) {}
	//RpcResponse(const Value &request_id) : Super(Json()) { setId(request_id); }
	RpcResponse() : Super() {}
	RpcResponse(const RpcMessage &msg) : Super(msg) {}
public:
	bool isError() const {return !error().empty();}
	RpcResponse& setError(Error &&err);
	Error error() const;
	RpcResponse& setResult(const Value &res);
	Value result() const;
	RpcResponse& setId(const Value::UInt id) {Super::setId(id); return *this;}
};

} // namespace chainpackrpc
} // namespace shv
