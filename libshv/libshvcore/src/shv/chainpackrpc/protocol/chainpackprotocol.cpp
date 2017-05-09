#include "chainpackprotocol.h"

#include <iostream>
#include <cassert>
#include <limits>

namespace shv {
namespace chainpackrpc {
namespace protocol {

namespace {
/*
template<typename T>
bool readInt(T &i)
{
	const uchar *src = (const uchar *)(m_data.constData() + m_position);
	m_position += sizeof(T);
	return qFromLittleEndian<T>(src);
}

//QIODevice *outDevice();

template<typename T>
int64_t writeInt(T n)
{
	n = qToLittleEndian<T>(n);
	qint64 len = outDevice()->write((const char *)(&n), sizeof(n));
	if(len < (qint64)sizeof(n))
		QF_EXCEPTION("Write int ERROR!");
	return len;
}

qint64 writeBlob(const QByteArray &blob);
*/
/* UInt
   0 ... 127              |0|x|x|x|x|x|x|x|<-- MSB
  28 ... 16383 (2^14-1)   |1|x|x|x|x|x|x|x| |0|x|x|x|x|x|x|x|<-- MSB
2^14 ... 2097151 (2^21-1) |1|x|x|x|x|x|x|x| |1|x|x|x|x|x|x|x| |0|x|x|x|x|x|x|x|<-- MSB
*/
template<typename T>
T read_UInt(const ChainPackProtocol::Blob &out, ChainPackProtocol::Blob::size_type &pos)
{
	T n = 0;
	int shift = 0;
	do {
		if(pos >= out.length())
			throw std::runtime_error("read_UInt: Index out of range!");
		uint8_t r = out[pos++];
		bool has_next = (r & 128);
		r = r & 127;
		T n1 = r;
		n1 <<= shift;
		shift += 7;
		n += n1;
		if(!has_next)
			break;
	} while(true);
	return n;
}

template<typename T>
void write_UInt(ChainPackProtocol::Blob &out, T n)
{
	do {
		uint8_t r = n & 127;
		n = n >> 7;
		if(n > 0)
			r = r | 128;
		out += r;
	} while(n);
}

/*
+/-    0 ... 63               |0|s|x|x|x|x|x|x|<-- MSB
+/-   64 ... 8191 (2^13-1)    |1|s|x|x|x|x|x|x| |0|x|x|x|x|x|x|x|<-- MSB
+/- 8192 ... 1048575 (2^20-1) |1|s|x|x|x|x|x|x| |1|x|x|x|x|x|x|x| |0|x|x|x|x|x|x|x|<-- MSB
*/

template<typename T>
T read_Int(const ChainPackProtocol::Blob &out, ChainPackProtocol::Blob::size_type &pos)
{
	T n = 0;
	bool s = false;
	uint8_t mask_len = 6;
	uint8_t mask = 63;
	int shift = 0;
	do {
		if(pos >= out.length())
			throw std::runtime_error("read_Int: Index out of range!");
		uint8_t r = out[pos++];
		bool has_next = (r & 128);
		if(mask_len == 6 && (r & 64))
			s = true;
		r = r & mask;
		T n1 = r;
		n1 <<= shift;
		n += n1;
		if(!has_next)
			break;
		shift += mask_len;
		mask_len = 7;
		mask = 127;
	} while(true);
	if(s)
		n = -n;
	return n;
}

template<typename T>
void write_Int(ChainPackProtocol::Blob &out, T n)
{
	if(n == std::numeric_limits<T>::min()) {
		std::cerr << "cannot pack MIN_INT, will be packed as MIN_INT+1\n";
		n++;
	}
	bool s = (n < 0);
	if(s)
		n = -n;
	uint8_t shift = 6;
	uint8_t mask = 63;
	do {
		uint8_t r = n & mask;
		n = n >> shift;
		if(n > 0)
			r = r | 128;
		assert(n >= 0);
		if(shift == 6 && s)
			r = r | 64;
		out += r;
		shift = 7;
		mask = 127;
	} while(n);
}

double read_Double(const ChainPackProtocol::Blob &data, ChainPackProtocol::Blob::size_type &pos)
{
	union U {uint64_t n; double d;} u;
	u.n = 0;
	int shift = 0;
	for (size_t i = 0; i < sizeof(u.n); ++i) {
		if(pos >= data.length())
			throw std::runtime_error("read_Double: Index out of range!");
		uint8_t r = data[pos++];
		uint64_t n1 = r;
		n1 <<= shift;
		shift += 8;
		u.n += n1;
	}
	return u.d;
}

void write_Double(ChainPackProtocol::Blob &out, double d)
{
	union U {uint64_t n; double d;} u;
	assert(sizeof(u.n) == sizeof(u.d));
	u.d = d;
	for (size_t i = 0; i < sizeof(u.n); ++i) {
		uint8_t r = u.n & 255;
		u.n = u.n >> 8;
		out += r;
	}
}

template<typename T>
void write_Blob(ChainPackProtocol::Blob &out, const T &blob)
{
	using S = typename T::size_type;
	S l = blob.length();
	write_UInt<S>(out, l);
	out.reserve(out.size() + l);
	for (S i = 0; i < l; ++i)
		out += (uint8_t)blob[i];
}

template<typename T>
T read_Blob(const ChainPackProtocol::Blob &data, ChainPackProtocol::Blob::size_type &pos)
{
	using S = typename T::size_type;
	unsigned int len = read_UInt<unsigned int>(data, pos);
	T ret;
	ret.reserve(len);
	for (S i = 0; i < len; ++i)
		ret += data[pos++];
	return ret;
}

void write_DateTime(ChainPackProtocol::Blob &out, const Message &pack)
{
	const Message::DateTime dt = pack.toDateTime();
	write_Int(out, dt.msecs);
}

Message::DateTime read_DateTime(const ChainPackProtocol::Blob &out, ChainPackProtocol::Blob::size_type &pos)
{
	Message::DateTime dt;
	dt.msecs = read_Int<decltype(dt.msecs)>(out, pos);
	return dt;
}

} // namespace

void ChainPackProtocol::writeData_List(ChainPackProtocol::Blob &out, const Message &pack)
{
	for (int i = 0; i < pack.count(); ++i) {
		const Message &cp = pack[i];
		write(out, cp);
	}
	out += (uint8_t)Message::Type::TERM;
}

Message::List ChainPackProtocol::readData_List(const ChainPackProtocol::Blob &data, ChainPackProtocol::Blob::size_type &pos)
{
	Message::List lst;
	while(true) {
		if(data[pos] == Message::Type::TERM) {
			pos++;
			break;
		}
		size_t n;
		Message cp = read(data, pos, &n);
		pos += n;
		lst.push_back(cp);
	}
	return lst;
}

void ChainPackProtocol::writeData_Table(ChainPackProtocol::Blob &out, const Message &pack)
{
	for (int i = 0; i < pack.count(); ++i) {
		const Message &cp = pack[i];
		if(i == 0)
			write(out, cp, false);
		else
			writeData(out, cp, false);
	}
	out += (uint8_t)Message::Type::TERM;
}

Message::Table ChainPackProtocol::readData_Table(const ChainPackProtocol::Blob &data, ChainPackProtocol::Blob::size_type &pos)
{
	Message::Table ret;
	Message::Type::Enum type;
	Message meta;
	while(true) {
		if(data[pos] == Message::Type::TERM) {
			pos++;
			break;
		}
		if(ret.empty()) {
			size_t consumed;
			Message cp = read(data, pos, &consumed);
			meta = cp.meta();
			type = cp.type();
			pos += consumed;
			ret.push_back(cp);
		}
		else {
			Message cp = readData(type, data, pos);
			cp.setMeta(meta);
			ret.push_back(cp);
		}
	}
	return ret;
}

void ChainPackProtocol::writeData_Map(ChainPackProtocol::Blob &out, const Message &pack)
{
	const Message::Map &map = pack.toMap();
	for (const auto &kv : map) {
		write_Blob<Message::String>(out, kv.first);
		write(out, kv.second);
	}
	out += (uint8_t)Message::Type::TERM;
}

Message::Map ChainPackProtocol::readData_Map(const ChainPackProtocol::Blob &data, ChainPackProtocol::Blob::size_type &pos)
{
	Message::Map ret;
	while(true) {
		if(data[pos] == Message::Type::TERM) {
			pos++;
			break;
		}
		Message::String key = read_Blob<Message::String>(data, pos);
		size_t consumed;
		Message cp = read(data, pos, &consumed);
		pos += consumed;
		ret[key] = cp;
	}
	return ret;
}

int ChainPackProtocol::write(Blob &out, const Message &pack, bool use_tiny_uint)
{
	if(!pack.isValid())
		throw std::runtime_error("Cannot serialize invalid ChainPack.");
	int len = writeTypeInfo(out, pack, use_tiny_uint);
	len += writeData(out, pack, use_tiny_uint);
	return len;
}

int ChainPackProtocol::writeTypeInfo(ChainPackProtocol::Blob &out, const Message &pack, bool use_tiny_uint)
{
	if(!pack.isValid())
		throw std::runtime_error("Cannot serialize invalid ChainPack.");
	int len = out.length();
	Message::Type::Enum msg_type = pack.type();
	uint8_t t = (uint8_t)msg_type;
	if(msg_type == Message::Type::Bool) {
		t = pack.toBool()? Message::Type::TRUE: Message::Type::FALSE;
	}
	else if(use_tiny_uint) {
		if(msg_type == Message::Type::UInt) {
			unsigned int n = pack.toUInt();
			if(n < 64) {
				/// TinyUInt
				t = n + 64;
			}
		}
	}
	out += t;
	Message meta = pack.meta();
	if(meta.isValid()) {
		out[len] |= 128;
		write(out, meta);
	}
	return (out.length() - len);
}

int ChainPackProtocol::writeData(ChainPackProtocol::Blob &out, const Message &pack, bool use_tiny_uint)
{
	int len = out.length();
	switch (pack.type()) {
	case Message::Type::TERM:
	case Message::Type::Null:
	case Message::Type::Bool:
		break;
	case Message::Type::UInt: {
		unsigned int u = pack.toUInt();
		if(!use_tiny_uint || u >= 64) {
			write_UInt(out, u);
		}
		break;
	}
	case Message::Type::Int: write_Int(out, pack.toInt()); break;
	case Message::Type::Double: write_Double(out, pack.toDouble()); break;
	case Message::Type::String: write_Blob(out, pack.toString()); break;
	case Message::Type::Blob: write_Blob(out, pack.toBlob()); break;
	case Message::Type::List: writeData_List(out, pack); break;
	case Message::Type::Table: writeData_Table(out, pack); break;
	case Message::Type::Map: writeData_Map(out, pack); break;
	case Message::Type::DateTime: write_DateTime(out, pack); break;
	case Message::Type::MetaTypeNameSpaceId:
	case Message::Type::MetaTypeId: write_UInt(out, pack.toUInt()); break;
	case Message::Type::MetaTypeNameSpaceName:
	case Message::Type::MetaTypeName: write_Blob(out, pack.toString()); break;
	default:
		throw std::runtime_error("Internal error: attempt to write helper type directly. type: " + std::to_string(pack.type()));
	}
	return (out.length() - len);
}

Message ChainPackProtocol::read(const ChainPackProtocol::Blob &data, size_t pos, size_t *consumed)
{
	size_t pos1 = pos;
	Message meta;
	int tiny_uint = -1;
	Message::Type::Enum type = readTypeInfo(data, pos, meta, tiny_uint);
	Message ret;
	if(tiny_uint >= 0) {
		ret = Message((unsigned int)tiny_uint);
	}
	else {
		ret = readData(type, data, pos);
	}
	if(meta.isValid())
		ret.setMeta(meta);
	if(consumed)
		*consumed = pos - pos1;
	return ret;
}

Message::Type::Enum ChainPackProtocol::readTypeInfo(const ChainPackProtocol::Blob &data, ChainPackProtocol::Blob::size_type &pos, Message &meta, int &tiny_uint)
{
	if(pos >= data.length())
		throw std::runtime_error("Not enough data to read type info.");
	uint8_t t = data[pos++];
	if(t & 128) {
		t = t & ~128;
		size_t n;
		meta = read(data, pos, &n);
		pos += n;
	}
	if(t >= 64) {
		/// TinyUInt
		t -= 64;
		tiny_uint = t;
		return Message::Type::UInt;
	}
	else {
		Message::Type::Enum type = (Message::Type::Enum)t;
		return type;
	}
}

Message ChainPackProtocol::readData(Message::Type::Enum type, const ChainPackProtocol::Blob &data, ChainPackProtocol::Blob::size_type &pos)
{
	Message ret;
	switch (type) {
	case Message::Type::Null:
		ret = Message(nullptr);
		break;
	case Message::Type::UInt: { unsigned int u = read_UInt<unsigned int>(data, pos); ret = Message(u); break; }
	case Message::Type::Int: { signed int i = read_Int<signed int>(data, pos); ret = Message(i); break; }
	case Message::Type::Double: { double d = read_Double(data, pos); ret = Message(d); break; }
	case Message::Type::TRUE: { bool b = true; ret = Message(b); break; }
	case Message::Type::FALSE: { bool b = false; ret = Message(b); break; }
	case Message::Type::String: { Message::String val = read_Blob<Message::String>(data, pos); ret = Message(val); break; }
	case Message::Type::Blob: { Message::Blob val = read_Blob<Message::Blob>(data, pos); ret = Message(val); break; }
	case Message::Type::List: { Message::List val = readData_List(data, pos); ret = Message(val); break; }
	case Message::Type::Table: { Message::Table val = readData_Table(data, pos); ret = Message(val); break; }
	case Message::Type::Map: { Message::Map val = readData_Map(data, pos); ret = Message(val); break; }
	case Message::Type::DateTime: { Message::DateTime val = read_DateTime(data, pos); ret = Message(val); break; }
	case Message::Type::MetaTypeId:
	{
		using U = decltype(Message::MetaTypeId::id);
		U u = read_UInt<U>(data, pos);
		Message::MetaTypeId mid(u);
		ret = Message(mid);
		break;
	}
	case Message::Type::MetaTypeNameSpaceId:
	{
		using U = decltype(Message::MetaTypeNameSpaceId::id);
		U u = read_UInt<U>(data, pos);
		Message::MetaTypeNameSpaceId mid(u);
		ret = Message(mid);
		break;
	}
	case Message::Type::MetaTypeName: { Message::String val = read_Blob<Message::String>(data, pos); ret = Message(Message::MetaTypeName(val)); break; }
	case Message::Type::MetaTypeNameSpaceName: { Message::String val = read_Blob<Message::String>(data, pos); ret = Message(Message::MetaTypeNameSpaceName(val)); break; }
	default:
		throw std::runtime_error("Internal error: attempt to read helper type directly. type: " + std::to_string(type));
	}
	return ret;
}

}}}
