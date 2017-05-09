#pragma once

#include "../chainpackmessage.h"

#include <string>

namespace shv {
namespace chainpackrpc {
namespace protocol {

class ChainPackProtocol
{
public:
	using Blob = Message::Blob;
public:
	static Message read(const Blob &data, size_t pos = 0, size_t *consumed = nullptr);
	static int write(Blob &out, const Message &pack, bool use_tiny_uint = true);
private:
	static int writeTypeInfo(Blob &out, const Message &pack, bool use_tiny_uint = true);
	static int writeData(Blob &out, const Message &pack, bool use_tiny_uint = true);
	static Message::Type::Enum readTypeInfo(const ChainPackProtocol::Blob &data, ChainPackProtocol::Blob::size_type &pos, Message &meta, int &tiny_uint);
	static Message readData(Message::Type::Enum type, const ChainPackProtocol::Blob &data, ChainPackProtocol::Blob::size_type &pos);
private:
	static void writeData_List(ChainPackProtocol::Blob &out, const Message &pack);
	static void writeData_Table(ChainPackProtocol::Blob &out, const Message &pack);
	static void writeData_Map(ChainPackProtocol::Blob &out, const Message &pack);
	//void writeData_DateTime(Protocol::Blob &out, const ChainPack &pack);
	static Message::List readData_List(const ChainPackProtocol::Blob &data, ChainPackProtocol::Blob::size_type &pos);
	static Message::Table readData_Table(const ChainPackProtocol::Blob &data, ChainPackProtocol::Blob::size_type &pos);
	static Message::Map readData_Map(const ChainPackProtocol::Blob &data, ChainPackProtocol::Blob::size_type &pos);
};

}}}

