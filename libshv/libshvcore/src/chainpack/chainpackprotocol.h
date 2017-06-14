#pragma once

#include "rpcvalue.h"

#include <string>
#include <streambuf>

namespace shv {
namespace core {
namespace chainpack {

class SHVCORE_DECL_EXPORT CharDataStreamBuffer : public std::streambuf
{
	using Super = std::streambuf;
public:
	CharDataStreamBuffer(const char *data, int len);
protected:
	pos_type seekoff(off_type off, std::ios_base::seekdir dir, std::ios_base::openmode mode) override;
};

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
			CHUNK_HEADER,
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
public:
	static uint64_t readUIntData(std::istream &data, bool *ok = nullptr);
	static uint64_t readUIntData(const char *data, size_t len, size_t *read_len = nullptr);
	static void writeUIntData(std::ostream &out, uint64_t n);
	static RpcValue read(std::istream &data);
	static int write(std::ostream &out, const RpcValue &pack);
private:
	static TypeInfo::Enum typeToTypeInfo(RpcValue::Type type);
	static RpcValue::Type typeInfoToType(TypeInfo::Enum type_info);
	static TypeInfo::Enum optimizedMetaTagType(RpcValue::Tag::Enum tag);

	static void writeMetaData(std::ostream &out, const RpcValue &pack);
	static bool writeTypeInfo(std::ostream &out, const RpcValue &pack);
	//static void writeData(Blob &out, const RpcValue &pack);
	static void writeData(std::ostream &out, const RpcValue &pack);
	static TypeInfo::Enum readTypeInfo(std::istream &data, RpcValue &meta, int &tiny_uint);
	static RpcValue readData(TypeInfo::Enum type, bool is_array, std::istream &data);
private:
	static void writeData_List(std::ostream &out, const RpcValue::List &list);
	static void writeData_Array(std::ostream &out, const RpcValue &pack);
	static void writeData_Map(std::ostream &out, const RpcValue::Map &map);
	static void writeData_IMap(std::ostream &out, const RpcValue::IMap &map);

	static RpcValue::MetaData readMetaData(std::istream &data);

	static RpcValue::List readData_List(std::istream &data);
	static RpcValue::Array readData_Array(TypeInfo::Enum type_info, std::istream &data);
	static RpcValue::Map readData_Map(std::istream &data);
	static RpcValue::IMap readData_IMap(std::istream &data);
};

}}}

