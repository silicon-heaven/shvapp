#pragma once

#include "rpcvalue.h"

#include <string>

namespace shv {
namespace chainpack {

class SHVCORE_DECL_EXPORT ChainPackProtocol
{
	static constexpr uint8_t ARRAY_FLAG_MASK = 64;
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
			Null,
			UInt,
			Int,
			Double,
			Bool,
			Blob,
			String,
			DateTime,
			List,
			Map,
			IMap,
			MetaIMap,
			/// arrays
			Null_Array = Null | ARRAY_FLAG_MASK, // if bit 6 is set, then packed value is an Array of corresponding values
			UInt_Array = UInt | ARRAY_FLAG_MASK,
			Int_Array = Int | ARRAY_FLAG_MASK,
			Double_Array = Double | ARRAY_FLAG_MASK,
			Bool_Array = Bool | ARRAY_FLAG_MASK,
			Blob_Array = Blob | ARRAY_FLAG_MASK,
			String_Array = String | ARRAY_FLAG_MASK,
			DateTime_Array = DateTime | ARRAY_FLAG_MASK,
			List_Array = List | ARRAY_FLAG_MASK,
			Map_Array = Map | ARRAY_FLAG_MASK,
			IMap_Array = IMap | ARRAY_FLAG_MASK,
			MetaIMap_Array = MetaIMap | ARRAY_FLAG_MASK,
		};
		static const char* name(Enum e);
	};
	using Blob = RpcValue::Blob;
public:
	static RpcValue read(const Blob &data, size_t pos = 0, size_t *consumed = nullptr);
	static int write(Blob &out, const RpcValue &pack);
private:
	static TypeInfo::Enum typeToTypeInfo(RpcValue::Type type);
	static RpcValue::Type typeInfoToType(TypeInfo::Enum type_info);
	static TypeInfo::Enum optimizedMetaTagType(RpcValue::Tag::Enum tag);

	static void writeMetaData(Blob &out, const RpcValue &pack);
	static bool writeTypeInfo(Blob &out, const RpcValue &pack);
	static void writeData(Blob &out, const RpcValue &pack);
	static TypeInfo::Enum readTypeInfo(const ChainPackProtocol::Blob &data, ChainPackProtocol::Blob::size_type &pos, RpcValue &meta, int &tiny_uint);
	static RpcValue readData(TypeInfo::Enum type, bool is_array, const ChainPackProtocol::Blob &data, ChainPackProtocol::Blob::size_type &pos);
private:
	static void writeData_List(ChainPackProtocol::Blob &out, const RpcValue::List &list);
	static void writeData_Array(ChainPackProtocol::Blob &out, const RpcValue &pack);
	static void writeData_Map(ChainPackProtocol::Blob &out, const RpcValue::Map &map);
	static void writeData_IMap(ChainPackProtocol::Blob &out, const RpcValue::IMap &map);

	static RpcValue::MetaData readMetaData(const ChainPackProtocol::Blob &data, ChainPackProtocol::Blob::size_type &pos);

	static RpcValue::List readData_List(const ChainPackProtocol::Blob &data, ChainPackProtocol::Blob::size_type &pos);
	static RpcValue::Array readData_Array(TypeInfo::Enum type_info, const ChainPackProtocol::Blob &data, ChainPackProtocol::Blob::size_type &pos);
	static RpcValue::Map readData_Map(const ChainPackProtocol::Blob &data, ChainPackProtocol::Blob::size_type &pos);
	static RpcValue::IMap readData_IMap(const ChainPackProtocol::Blob &data, ChainPackProtocol::Blob::size_type &pos);
};

}}

