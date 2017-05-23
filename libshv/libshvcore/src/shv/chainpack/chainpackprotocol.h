#pragma once

#include "rpcvalue.h"

#include <string>

namespace shv {
namespace chainpack {

class SHVCORE_DECL_EXPORT ChainPackProtocol
{
public:
	struct TypeInfo {
		enum Enum {
			INVALID = -1,
			/// auxiliary types used for optimization
			TERM = 128, // first byte of packed Int or UInt cannot be like 0b1000000
			META_TYPE_ID,
			META_TYPE_NAMESPACE_ID,
			FALSE,
			TRUE,
			/// types
			Null = 128+64,
			UInt,
			Int,
			Double,
			Bool,
			Blob,
			String,
			DateTime,
			List,
			Table,
			Map,
			IMap,
			MetaIMap,
		};
		static const char* name(Enum e);
	};
	using Blob = RpcValue::Blob;
public:
	static RpcValue read(const Blob &data, size_t pos = 0, size_t *consumed = nullptr);
	static int write(Blob &out, const RpcValue &pack);
private:
	static TypeInfo::Enum typeToTypeInfo(RpcValue::Type type);
	static TypeInfo::Enum optimizedMetaTagType(RpcValue::Tag::Enum tag);

	static void writeMetaData(Blob &out, const RpcValue &pack);
	static bool writeTypeInfo(Blob &out, const RpcValue &pack);
	static void writeData(Blob &out, const RpcValue &pack);
	static TypeInfo::Enum readTypeInfo(const ChainPackProtocol::Blob &data, ChainPackProtocol::Blob::size_type &pos, RpcValue &meta, int &tiny_uint);
	static RpcValue readData(TypeInfo::Enum type, const ChainPackProtocol::Blob &data, ChainPackProtocol::Blob::size_type &pos);
private:
	static void writeData_List(ChainPackProtocol::Blob &out, const RpcValue &pack);
	static void writeData_Table(ChainPackProtocol::Blob &out, const RpcValue &pack);
	static void writeData_Map(ChainPackProtocol::Blob &out, const RpcValue::Map &map);
	static void writeData_IMap(ChainPackProtocol::Blob &out, const RpcValue::IMap &map);

	static RpcValue::MetaData readMetaData(const ChainPackProtocol::Blob &data, ChainPackProtocol::Blob::size_type &pos);

	static RpcValue::List readData_List(const ChainPackProtocol::Blob &data, ChainPackProtocol::Blob::size_type &pos);
	static RpcValue::Table readData_Table(const ChainPackProtocol::Blob &data, ChainPackProtocol::Blob::size_type &pos);
	static RpcValue::Map readData_Map(const ChainPackProtocol::Blob &data, ChainPackProtocol::Blob::size_type &pos);
	static RpcValue::IMap readData_IMap(const ChainPackProtocol::Blob &data, ChainPackProtocol::Blob::size_type &pos);
};

}}

