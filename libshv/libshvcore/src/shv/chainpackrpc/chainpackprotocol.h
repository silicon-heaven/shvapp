#pragma once

#include "chainpackvalue.h"

#include <string>

namespace shv {
namespace chainpackrpc {

class SHVCORE_DECL_EXPORT ChainPackProtocol
{
public:
	using Blob = Value::Blob;
public:
	static Value read(const Blob &data, size_t pos = 0, size_t *consumed = nullptr);
	static int write(Blob &out, const Value &pack);
private:
	static void writeMetaData(Blob &out, const Value &pack);
	static bool writeTypeInfo(Blob &out, const Value &pack);
	static void writeData(Blob &out, const Value &pack);
	static Value::Type::Enum readTypeInfo(const ChainPackProtocol::Blob &data, ChainPackProtocol::Blob::size_type &pos, Value &meta, int &tiny_uint);
	static Value readData(Value::Type::Enum type, const ChainPackProtocol::Blob &data, ChainPackProtocol::Blob::size_type &pos);
private:
	static void writeData_List(ChainPackProtocol::Blob &out, const Value &pack);
	static void writeData_Table(ChainPackProtocol::Blob &out, const Value &pack);
	static void writeData_Map(ChainPackProtocol::Blob &out, const Value::Map &map);
	static void writeData_IMap(ChainPackProtocol::Blob &out, const Value::IMap &map);

	static Value::MetaData readMetaData(const ChainPackProtocol::Blob &data, ChainPackProtocol::Blob::size_type &pos);

	static Value::List readData_List(const ChainPackProtocol::Blob &data, ChainPackProtocol::Blob::size_type &pos);
	static Value::Table readData_Table(const ChainPackProtocol::Blob &data, ChainPackProtocol::Blob::size_type &pos);
	static Value::Map readData_Map(const ChainPackProtocol::Blob &data, ChainPackProtocol::Blob::size_type &pos);
	static Value::IMap readData_IMap(const ChainPackProtocol::Blob &data, ChainPackProtocol::Blob::size_type &pos);
};

}}

