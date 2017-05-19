#include "chainpackprotocol.h"

#include <iostream>
#include <cassert>
#include <limits>

namespace shv {
namespace chainpack {

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
   0 ... 127              |0|x|x|x|x|x|x|x|<-- LSB
  28 ... 16383 (2^14-1)   |1|x|x|x|x|x|x|x| |0|x|x|x|x|x|x|x|<-- LSB
2^14 ... 2097151 (2^21-1) |1|x|x|x|x|x|x|x| |1|x|x|x|x|x|x|x| |0|x|x|x|x|x|x|x|<-- LSB
*/
template<typename T>
T read_UIntData(const ChainPackProtocol::Blob &out, ChainPackProtocol::Blob::size_type &pos)
{
	T n = 0;
	do {
		if(pos >= out.length())
			throw std::runtime_error("read_UInt: Index out of range!");
		uint8_t r = out[pos++];
		bool has_next = (r & 128);
		r = r & 127;
		n = (n << 7) | r;
		if(!has_next)
			break;
	} while(true);
	return n;
}

template<typename T>
void write_UIntData(ChainPackProtocol::Blob &out, T n)
{
	uint8_t bytes[2 * sizeof(T)];
	int pos = 0;
	do {
		uint8_t r = n & 127;
		n = n >> 7;
		bytes[pos++] = r;
	} while(n);
	pos--;
	while (pos >= 0) {
		uint8_t r = bytes[pos];
		if(pos > 0)
			r |= 128;
		out += r;
		pos--;
	}
}

/*
+/-    0 ... 63               |0|x|x|x|x|x|x|s|<-- LSB
+/-   64 ... 8191 (2^13-1)    |1|x|x|x|x|x|x|x| |0|x|x|x|x|x|x|s|<-- LSB
+/- 8192 ... 1048575 (2^20-1) |1|x|x|x|x|x|x|x| |1|x|x|x|x|x|x|x| |0|x|x|x|x|x|x|s|<-- LSB
*/

template<typename T>
T read_IntData(const ChainPackProtocol::Blob &out, ChainPackProtocol::Blob::size_type &pos)
{
	T n = 0;
	bool s = false;
	//uint8_t mask_len = 6;
	uint8_t mask = 127;
	int shift = 7;
	do {
		if(pos >= out.length())
			throw std::runtime_error("read_Int: Index out of range!");
		uint8_t r = out[pos++];
		bool has_next = (r & 128);
		if(!has_next) {
			mask = 63 << 1;
			shift = 6;
			s = r & 1;
		}
		r = r & mask;
		if(!has_next)
			r >>= 1;
		n = (n << shift) | r;
		if(!has_next)
			break;
	} while(true);
	if(s)
		n = -n;
	return n;
}

template<typename T>
void write_IntData(ChainPackProtocol::Blob &out, T n)
{
	if(n == std::numeric_limits<T>::min()) {
		std::cerr << "cannot pack MIN_INT, will be packed as MIN_INT+1\n";
		n++;
	}
	uint8_t bytes[2 * sizeof(T)];
	int pos = 0;
	bool s = (n < 0);
	if(s)
		n = -n;
	uint8_t shift = 6;
	uint8_t mask = 63;
	do {
		uint8_t r = n & mask;
		n = n >> shift;
		assert(n >= 0);
		bytes[pos++] = r;
		shift = 7;
		mask = 127;
	} while(n);
	pos--;
	while (pos >= 0) {
		uint8_t r = bytes[pos];
		if(pos > 0) {
			r |= 128;
		}
		else {
			r <<= 1;
			if(s)
				r = r | 1;
		}
		out += r;
		pos--;
	}
}

/*
void write_Int(ChainPackProtocol::Blob &out, Value::Int n)
{
	if(n == std::numeric_limits<Value::Int>::min()) {
		std::cerr << "cannot pack MIN_INT, will be packed as MIN_INT+1\n";
		n++;
	}
	bool s = (n < 0);
	if(s) {
		n = -n;
		out += Value::Type::NEG_INT;
	}
	else {
		out += Value::Type::POS_INT;
	}
	write_UIntData(out, (Value::UInt)n);
}
*/
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
	write_UIntData<S>(out, l);
	out.reserve(out.size() + l);
	for (S i = 0; i < l; ++i)
		out += (uint8_t)blob[i];
}

template<typename T>
T read_Blob(const ChainPackProtocol::Blob &data, ChainPackProtocol::Blob::size_type &pos)
{
	using S = typename T::size_type;
	unsigned int len = read_UIntData<unsigned int>(data, pos);
	T ret;
	ret.reserve(len);
	for (S i = 0; i < len; ++i)
		ret += data[pos++];
	return ret;
}

void write_DateTime(ChainPackProtocol::Blob &out, const RpcValue &pack)
{
	const RpcValue::DateTime dt = pack.toDateTime();
	uint64_t msecs = dt.msecs;
	write_IntData(out, msecs);
}

RpcValue::DateTime read_DateTime(const ChainPackProtocol::Blob &data, ChainPackProtocol::Blob::size_type &pos)
{
	RpcValue::DateTime dt;
	dt.msecs = read_IntData<decltype(dt.msecs)>(data, pos);
	return dt;
}

} // namespace

void ChainPackProtocol::writeData_List(ChainPackProtocol::Blob &out, const RpcValue &pack)
{
	for (int i = 0; i < pack.count(); ++i) {
		const RpcValue &cp = pack.at(i);
		write(out, cp);
	}
	out += (uint8_t)RpcValue::Type::TERM;
}

RpcValue::List ChainPackProtocol::readData_List(const ChainPackProtocol::Blob &data, ChainPackProtocol::Blob::size_type &pos)
{
	RpcValue::List lst;
	while(true) {
		if(data[pos] == RpcValue::Type::TERM) {
			pos++;
			break;
		}
		size_t n;
		RpcValue cp = read(data, pos, &n);
		pos += n;
		lst.push_back(cp);
	}
	return lst;
}

void ChainPackProtocol::writeData_Table(ChainPackProtocol::Blob &out, const RpcValue &pack)
{
	for (int i = 0; i < pack.count(); ++i) {
		const RpcValue &cp = pack[i];
		if(i == 0)
			write(out, cp);
		else
			writeData(out, cp);
	}
	out += (uint8_t)RpcValue::Type::TERM;
}

RpcValue::Table ChainPackProtocol::readData_Table(const ChainPackProtocol::Blob &data, ChainPackProtocol::Blob::size_type &pos)
{
	RpcValue::Table ret;
	RpcValue::Type::Enum type;
	while(true) {
		if(data[pos] == RpcValue::Type::TERM) {
			pos++;
			break;
		}
		if(ret.empty()) {
			size_t consumed;
			RpcValue cp = read(data, pos, &consumed);
			type = cp.type();
			pos += consumed;
			ret.push_back(cp);
		}
		else {
			RpcValue cp = readData(type, data, pos);
			ret.push_back(cp);
		}
	}
	return ret;
}

void ChainPackProtocol::writeData_Map(ChainPackProtocol::Blob &out, const RpcValue::Map &map)
{
	for (const auto &kv : map) {
		write_Blob<RpcValue::String>(out, kv.first);
		write(out, kv.second);
	}
	out += (uint8_t)RpcValue::Type::TERM;
}

RpcValue::Map ChainPackProtocol::readData_Map(const ChainPackProtocol::Blob &data, ChainPackProtocol::Blob::size_type &pos)
{
	RpcValue::Map ret;
	while(true) {
		if(data[pos] == RpcValue::Type::TERM) {
			pos++;
			break;
		}
		RpcValue::String key = read_Blob<RpcValue::String>(data, pos);
		size_t consumed;
		RpcValue cp = read(data, pos, &consumed);
		pos += consumed;
		ret[key] = cp;
	}
	return ret;
}

RpcValue::IMap ChainPackProtocol::readData_IMap(const ChainPackProtocol::Blob &data, ChainPackProtocol::Blob::size_type &pos)
{
	RpcValue::IMap ret;
	while(true) {
		if(data[pos] == RpcValue::Type::TERM) {
			pos++;
			break;
		}
		RpcValue::UInt key = read_UIntData<RpcValue::UInt>(data, pos);
		size_t consumed;
		RpcValue cp = read(data, pos, &consumed);
		pos += consumed;
		ret[key] = cp;
	}
	return ret;
}

static RpcValue::Type::Enum optimized_meta_tag_type(RpcValue::UInt tag)
{
	switch(tag) {
	case RpcValue::Tag::MetaTypeId: return RpcValue::Type::META_TYPE_ID;
	case RpcValue::Tag::MetaTypeNameSpaceId: return RpcValue::Type::META_TYPE_NAMESPACE_ID;
	default:
		return RpcValue::Type::Invalid;
	}
}

void ChainPackProtocol::writeData_IMap(ChainPackProtocol::Blob &out, const RpcValue::IMap &map)
{
	for (const auto &kv : map) {
		RpcValue::UInt key = kv.first;
		write_UIntData(out, key);
		write(out, kv.second);
	}
	out += (uint8_t)RpcValue::Type::TERM;
}

int ChainPackProtocol::write(Blob &out, const RpcValue &pack)
{
	if(!pack.isValid())
		throw std::runtime_error("Cannot serialize invalid ChainPack.");
	int len = out.length();
	writeMetaData(out, pack);
	if(!writeTypeInfo(out, pack))
		writeData(out, pack);
	return (out.length() - len);
}

void ChainPackProtocol::writeMetaData(ChainPackProtocol::Blob &out, const RpcValue &pack)
{
	const RpcValue::MetaData &md = pack.metaData();
	for (const auto &key : md.ikeys()) {
		RpcValue::Type::Enum type = optimized_meta_tag_type(key);
		if(type != RpcValue::Type::Invalid) {
			out += (uint8_t)type;
			RpcValue::UInt val = md.value(key).toUInt();
			write_UIntData(out, val);
		}
	}
	RpcValue::IMap imap;
	for (const auto &key : md.ikeys()) {
		if(optimized_meta_tag_type(key) == RpcValue::Type::Invalid)
			imap[key] = md.value(key);
	}
	if(!imap.empty()) {
		out += (uint8_t)RpcValue::Type::MetaIMap;
		writeData_IMap(out, imap);
	}
}

bool ChainPackProtocol::writeTypeInfo(ChainPackProtocol::Blob &out, const RpcValue &pack)
{
	if(!pack.isValid())
		throw std::runtime_error("Cannot serialize invalid ChainPack.");
	bool ret = false;
	RpcValue::Type::Enum msg_type = pack.type();
	uint8_t t = (uint8_t)msg_type;
	{
		if(msg_type == RpcValue::Type::Bool) {
			t = pack.toBool()? RpcValue::Type::TRUE: RpcValue::Type::FALSE;
			ret = true;
		}
		else if(msg_type == RpcValue::Type::UInt) {
			auto n = pack.toUInt();
			if(n < 64) {
				/// TinyUInt
				t = n;
				ret = true;
			}
		}
		else if(msg_type == RpcValue::Type::Int) {
			auto n = pack.toInt();
			if(n >= 0 && n < 64) {
				/// TinyInt
				t = 64 + n;
				ret = true;
			}
		}
	}
	out += t;
	return ret;
}

void ChainPackProtocol::writeData(ChainPackProtocol::Blob &out, const RpcValue &pack)
{
	switch (pack.type()) {
	case RpcValue::Type::TERM:
	case RpcValue::Type::Null:
		break;
	case RpcValue::Type::Bool:
		out += pack.toBool()? 1: 0;
		break;
	case RpcValue::Type::UInt: {
		auto u = pack.toUInt();
		write_UIntData(out, u);
		break;
	}
	case RpcValue::Type::Int: {
		RpcValue::Int n = pack.toInt();
		write_IntData(out, n);
		break;
	}
	case RpcValue::Type::Double: write_Double(out, pack.toDouble()); break;
	case RpcValue::Type::String: write_Blob(out, pack.toString()); break;
	case RpcValue::Type::Blob: write_Blob(out, pack.toBlob()); break;
	case RpcValue::Type::List: writeData_List(out, pack); break;
	case RpcValue::Type::Table: writeData_Table(out, pack); break;
	case RpcValue::Type::Map: writeData_Map(out, pack.toMap()); break;
	case RpcValue::Type::IMap: writeData_IMap(out, pack.toIMap()); break;
	case RpcValue::Type::DateTime: write_DateTime(out, pack); break;
	default:
		throw std::runtime_error("Internal error: attempt to write helper type directly. type: " + std::to_string(pack.type()));
	}
}

RpcValue ChainPackProtocol::read(const ChainPackProtocol::Blob &data, size_t pos, size_t *consumed)
{
	size_t pos1 = pos;
	RpcValue ret;
	RpcValue::MetaData meta_data = readMetaData(data, pos);
	uint8_t type = data[pos++];
	if(type < 128) {
		if(type & 64) {
			// tiny Int
			RpcValue::Int n = type & 63;
			ret = RpcValue(n);
		}
		else {
			// tiny UInt
			RpcValue::UInt n = type & 63;
			ret = RpcValue(n);
		}
	}
	else if(type == RpcValue::Type::FALSE) {
		ret = RpcValue(false);
	}
	else if(type == RpcValue::Type::TRUE) {
		ret = RpcValue(true);
	}
	if(!ret.isValid()) {
		ret = readData((RpcValue::Type::Enum)type, data, pos);
	}
	if(!meta_data.isEmpty())
		ret.setMetaData(std::move(meta_data));
	if(consumed)
		*consumed = pos - pos1;
	return ret;
}

RpcValue::MetaData ChainPackProtocol::readMetaData(const ChainPackProtocol::Blob &data, RpcValue::Blob::size_type &pos)
{
	RpcValue::MetaData ret;
	while(true) {
		bool has_meta = true;
		uint8_t type = data[pos];
		switch(type) {
		case RpcValue::Type::META_TYPE_ID:  {
			pos++;
			RpcValue::UInt u = read_UIntData<RpcValue::UInt>(data, pos);
			ret.setValue(RpcValue::Tag::MetaTypeId, u);
			break;
		}
		case RpcValue::Type::META_TYPE_NAMESPACE_ID:  {
			pos++;
			RpcValue::UInt u = read_UIntData<RpcValue::UInt>(data, pos);
			ret.setValue(RpcValue::Tag::MetaTypeNameSpaceId, u);
			break;
		}
		case RpcValue::Type::MetaIMap:  {
			pos++;
			RpcValue::IMap imap = readData_IMap(data, pos);
			for( const auto &it : imap)
				ret.setValue(it.first, it.second);
			break;
		}
		default:
			has_meta = false;
			break;
		}
		if(!has_meta)
			break;
	}
	return ret;
}

RpcValue::Type::Enum ChainPackProtocol::readTypeInfo(const ChainPackProtocol::Blob &data, ChainPackProtocol::Blob::size_type &pos, RpcValue &meta, int &tiny_uint)
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
		return RpcValue::Type::UInt;
	}
	else {
		RpcValue::Type::Enum type = (RpcValue::Type::Enum)t;
		return type;
	}
}

RpcValue ChainPackProtocol::readData(RpcValue::Type::Enum type, const ChainPackProtocol::Blob &data, ChainPackProtocol::Blob::size_type &pos)
{
	RpcValue ret;
	switch (type) {
	case RpcValue::Type::Null:
		ret = RpcValue(nullptr);
		break;
	case RpcValue::Type::UInt: { RpcValue::UInt u = read_UIntData<RpcValue::UInt>(data, pos); ret = RpcValue(u); break; }
	case RpcValue::Type::Int: { RpcValue::Int i = read_IntData<RpcValue::Int>(data, pos); ret = RpcValue(i); break; }
	case RpcValue::Type::Double: { double d = read_Double(data, pos); ret = RpcValue(d); break; }
	case RpcValue::Type::TRUE: { bool b = true; ret = RpcValue(b); break; }
	case RpcValue::Type::FALSE: { bool b = false; ret = RpcValue(b); break; }
	case RpcValue::Type::String: { RpcValue::String val = read_Blob<RpcValue::String>(data, pos); ret = RpcValue(val); break; }
	case RpcValue::Type::Blob: { RpcValue::Blob val = read_Blob<RpcValue::Blob>(data, pos); ret = RpcValue(val); break; }
	case RpcValue::Type::List: { RpcValue::List val = readData_List(data, pos); ret = RpcValue(val); break; }
	case RpcValue::Type::Table: { RpcValue::Table val = readData_Table(data, pos); ret = RpcValue(val); break; }
	case RpcValue::Type::Map: { RpcValue::Map val = readData_Map(data, pos); ret = RpcValue(val); break; }
	case RpcValue::Type::IMap: { RpcValue::IMap val = readData_IMap(data, pos); ret = RpcValue(val); break; }
	case RpcValue::Type::DateTime: { RpcValue::DateTime val = read_DateTime(data, pos); ret = RpcValue(val); break; }
		/*
	case Value::Type::MetaTypeId:
	{
		using U = decltype(Value::MetaTypeId::id);
		U u = read_UInt<U>(data, pos);
		Value::MetaTypeId mid(u);
		ret = Value(mid);
		break;
	}
	case Value::Type::MetaTypeNameSpaceId:
	{
		using U = decltype(Value::MetaTypeNameSpaceId::id);
		U u = read_UInt<U>(data, pos);
		Value::MetaTypeNameSpaceId mid(u);
		ret = Value(mid);
		break;
	}
	case Value::Type::MetaTypeName: { Value::String val = read_Blob<Value::String>(data, pos); ret = Value(Value::MetaTypeName(val)); break; }
	case Value::Type::MetaTypeNameSpaceName: { Value::String val = read_Blob<Value::String>(data, pos); ret = Value(Value::MetaTypeNameSpaceName(val)); break; }
	*/
	default:
		throw std::runtime_error("Internal error: attempt to read helper type directly. type: " + std::to_string(type));
	}
	return ret;
}

}}
