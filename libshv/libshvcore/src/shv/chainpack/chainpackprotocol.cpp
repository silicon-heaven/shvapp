#include "chainpackprotocol.h"

#include "../../shvexception.h"

#include <iostream>
#include <cassert>
#include <limits>

namespace shv {
namespace chainpack {

namespace {
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
			SHV_EXCEPTION("read_UInt: Index out of range!");
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
			SHV_EXCEPTION("read_Int: Index out of range!");
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
			SHV_EXCEPTION("read_Double: Index out of range!");
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

ChainPackProtocol::TypeInfo::Enum ChainPackProtocol::typeToTypeInfo(RpcValue::Type type)
{
	switch (type) {
	case RpcValue::Type::Invalid:
		SHV_EXCEPTION("There is no type info for type Invalid");
	case RpcValue::Type::Array:
		SHV_EXCEPTION("There is no type info for type Array");
	case RpcValue::Type::Null: return TypeInfo::Null;
	case RpcValue::Type::UInt: return TypeInfo::UInt;
	case RpcValue::Type::Int: return TypeInfo::Int;
	case RpcValue::Type::Double: return TypeInfo::Double;
	case RpcValue::Type::Bool: return TypeInfo::Bool;
	case RpcValue::Type::Blob: return TypeInfo::Blob;
	case RpcValue::Type::String: return TypeInfo::String;
	case RpcValue::Type::List: return TypeInfo::List;
	case RpcValue::Type::Map: return TypeInfo::Map;
	case RpcValue::Type::IMap: return TypeInfo::IMap;
	case RpcValue::Type::DateTime: return TypeInfo::DateTime;
	case RpcValue::Type::MetaIMap: return TypeInfo::MetaIMap;
	}
}

RpcValue::Type ChainPackProtocol::typeInfoToType(ChainPackProtocol::TypeInfo::Enum type_info)
{
	switch (type_info) {
	case ChainPackProtocol::TypeInfo::Null: return RpcValue::Type::Null;
	case ChainPackProtocol::TypeInfo::UInt: return RpcValue::Type::UInt;
	case ChainPackProtocol::TypeInfo::Int: return RpcValue::Type::Int;
	case ChainPackProtocol::TypeInfo::Double: return RpcValue::Type::Double;
	case ChainPackProtocol::TypeInfo::Bool: return RpcValue::Type::Bool;
	case ChainPackProtocol::TypeInfo::Blob: return RpcValue::Type::Blob;
	case ChainPackProtocol::TypeInfo::String: return RpcValue::Type::String;
	case ChainPackProtocol::TypeInfo::DateTime: return RpcValue::Type::DateTime;
	case ChainPackProtocol::TypeInfo::List: return RpcValue::Type::List;
	case ChainPackProtocol::TypeInfo::Map: return RpcValue::Type::Map;
	case ChainPackProtocol::TypeInfo::IMap: return RpcValue::Type::IMap;
	case ChainPackProtocol::TypeInfo::MetaIMap: return RpcValue::Type::MetaIMap;
	default:
		SHV_EXCEPTION(std::string("There is type for type info ") + ChainPackProtocol::TypeInfo::name(type_info));
	}
}

const char *ChainPackProtocol::TypeInfo::name(ChainPackProtocol::TypeInfo::Enum e)
{
	switch (e) {
	case Null: return "Null";
	case UInt: return "UInt";
	case Int: return "Int";
	case Double: return "Double";
	case Bool: return "Bool";
	case Blob: return "Blob";
	case String: return "String";
	case List: return "List";
	//case Array: return "Array";
	case Map: return "Map";
	case IMap: return "IMap";
	case DateTime: return "DateTime";
	case META_TYPE_ID: return "META_TYPE_ID";
	case META_TYPE_NAMESPACE_ID: return "META_TYPE_NAMESPACE_ID";
	case MetaIMap: return "MetaIMap";
	case TRUE: return "TRUE";
	case FALSE: return "FALSE";
	case TERM: return "TERM";
	default:
		return "UNKNOWN";
	}
}

void ChainPackProtocol::writeData_List(ChainPackProtocol::Blob &out, const RpcValue &pack)
{
	for (int i = 0; i < pack.count(); ++i) {
		const RpcValue &cp = pack.at(i);
		write(out, cp);
	}
	out += (uint8_t)ChainPackProtocol::TypeInfo::TERM;
}

RpcValue::List ChainPackProtocol::readData_List(const ChainPackProtocol::Blob &data, ChainPackProtocol::Blob::size_type &pos)
{
	RpcValue::List lst;
	while(true) {
		if(data[pos] == ChainPackProtocol::TypeInfo::TERM) {
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

void ChainPackProtocol::writeData_Array(ChainPackProtocol::Blob &out, const RpcValue &pack)
{
	unsigned size = pack.count();
	write_UIntData(out, size);
	for (unsigned i = 0; i < size; ++i) {
		const RpcValue &cp = pack[i];
		writeData(out, cp);
	}
}

RpcValue::Array ChainPackProtocol::readData_Array(ChainPackProtocol::TypeInfo::Enum array_type_info, const ChainPackProtocol::Blob &data, ChainPackProtocol::Blob::size_type &pos)
{
	RpcValue::Type type = typeInfoToType(array_type_info);
	RpcValue::Array ret(type);
	RpcValue::UInt size = read_UIntData<RpcValue::UInt>(data, pos);
	ret.reserve(size);
	for (unsigned i = 0; i < size; ++i) {
		RpcValue cp = readData(array_type_info, false, data, pos);
		ret.push_back(cp);
	}
	return ret;
}

void ChainPackProtocol::writeData_Map(ChainPackProtocol::Blob &out, const RpcValue::Map &map)
{
	for (const auto &kv : map) {
		write_Blob<RpcValue::String>(out, kv.first);
		write(out, kv.second);
	}
	out += (uint8_t)ChainPackProtocol::TypeInfo::TERM;
}

RpcValue::Map ChainPackProtocol::readData_Map(const ChainPackProtocol::Blob &data, ChainPackProtocol::Blob::size_type &pos)
{
	RpcValue::Map ret;
	while(true) {
		if(data[pos] == ChainPackProtocol::TypeInfo::TERM) {
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
		if(data[pos] == ChainPackProtocol::TypeInfo::TERM) {
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

ChainPackProtocol::TypeInfo::Enum ChainPackProtocol::optimizedMetaTagType(RpcValue::Tag::Enum tag)
{
	switch(tag) {
	case RpcValue::Tag::MetaTypeId: return ChainPackProtocol::TypeInfo::META_TYPE_ID;
	case RpcValue::Tag::MetaTypeNameSpaceId: return ChainPackProtocol::TypeInfo::META_TYPE_NAMESPACE_ID;
	default:
		return ChainPackProtocol::TypeInfo::INVALID;
	}
}

void ChainPackProtocol::writeData_IMap(ChainPackProtocol::Blob &out, const RpcValue::IMap &map)
{
	for (const auto &kv : map) {
		RpcValue::UInt key = kv.first;
		write_UIntData(out, key);
		write(out, kv.second);
	}
	out += (uint8_t)ChainPackProtocol::TypeInfo::TERM;
}

int ChainPackProtocol::write(Blob &out, const RpcValue &pack)
{
	if(!pack.isValid())
		SHV_EXCEPTION("Cannot serialize invalid ChainPack.");
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
		ChainPackProtocol::TypeInfo::Enum type = optimizedMetaTagType((RpcValue::Tag::Enum)key);
		if(type != ChainPackProtocol::TypeInfo::INVALID) {
			out += (uint8_t)type;
			RpcValue::UInt val = md.value(key).toUInt();
			write_UIntData(out, val);
		}
	}
	RpcValue::IMap imap;
	for (const auto &key : md.ikeys()) {
		if(optimizedMetaTagType((RpcValue::Tag::Enum)key) == ChainPackProtocol::TypeInfo::INVALID)
			imap[key] = md.value(key);
	}
	if(!imap.empty()) {
		out += (uint8_t)ChainPackProtocol::TypeInfo::MetaIMap;
		writeData_IMap(out, imap);
	}
}

bool ChainPackProtocol::writeTypeInfo(ChainPackProtocol::Blob &out, const RpcValue &pack)
{
	if(!pack.isValid())
		SHV_EXCEPTION("Cannot serialize invalid ChainPack.");
	bool ret = false;
	RpcValue::Type type = pack.type();
	int t = TypeInfo::INVALID;
	if(type == RpcValue::Type::Bool) {
		t = pack.toBool()? ChainPackProtocol::TypeInfo::TRUE: ChainPackProtocol::TypeInfo::FALSE;
		ret = true;
	}
	else if(type == RpcValue::Type::UInt) {
		auto n = pack.toUInt();
		if(n < 64) {
			/// TinyUInt
			t = n;
			ret = true;
		}
	}
	else if(type == RpcValue::Type::Int) {
		auto n = pack.toInt();
		if(n >= 0 && n < 64) {
			/// TinyInt
			t = 64 + n;
			ret = true;
		}
	}
	else if(type == RpcValue::Type::Array) {
		t = typeToTypeInfo(pack.arrayType());
		t |= ARRAY_FLAG_MASK;
	}
	if(t == TypeInfo::INVALID) {
		t = typeToTypeInfo(pack.type());
	}
	out += (uint8_t)t;
	return ret;
}

void ChainPackProtocol::writeData(ChainPackProtocol::Blob &out, const RpcValue &pack)
{
	RpcValue::Type type = pack.type();
	switch (type) {
	case RpcValue::Type::Null: break;
	case RpcValue::Type::Bool: out += pack.toBool()? 1: 0; break;
	case RpcValue::Type::UInt: { auto u = pack.toUInt(); write_UIntData(out, u); break; }
	case RpcValue::Type::Int: { RpcValue::Int n = pack.toInt(); write_IntData(out, n); break; }
	case RpcValue::Type::Double: write_Double(out, pack.toDouble()); break;
	case RpcValue::Type::String: write_Blob(out, pack.toString()); break;
	case RpcValue::Type::Blob: write_Blob(out, pack.toBlob()); break;
	case RpcValue::Type::List: writeData_List(out, pack); break;
	case RpcValue::Type::Array: writeData_Array(out, pack); break;
	case RpcValue::Type::Map: writeData_Map(out, pack.toMap()); break;
	case RpcValue::Type::IMap: writeData_IMap(out, pack.toIMap()); break;
	case RpcValue::Type::DateTime: write_DateTime(out, pack); break;
	case RpcValue::Type::Invalid:
	case RpcValue::Type::MetaIMap:
		SHV_EXCEPTION("Internal error: attempt to write helper type directly. type: " + std::string(RpcValue::typeToName(type)));
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
	else if(type == ChainPackProtocol::TypeInfo::FALSE) {
		ret = RpcValue(false);
	}
	else if(type == ChainPackProtocol::TypeInfo::TRUE) {
		ret = RpcValue(true);
	}
	if(!ret.isValid()) {
		bool is_array = type & ARRAY_FLAG_MASK;
		type = type & ~ARRAY_FLAG_MASK;
		//ChainPackProtocol::TypeInfo::Enum value_type = is_array? type: ChainPackProtocol::TypeInfo::INVALID;
		//ChainPackProtocol::TypeInfo::Enum array_type = is_array? type: ChainPackProtocol::TypeInfo::INVALID;
		ret = readData((ChainPackProtocol::TypeInfo::Enum)type, is_array, data, pos);
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
		uint8_t type_info = data[pos];
		switch(type_info) {
		case ChainPackProtocol::TypeInfo::META_TYPE_ID:  {
			pos++;
			RpcValue::UInt u = read_UIntData<RpcValue::UInt>(data, pos);
			ret.setValue(RpcValue::Tag::MetaTypeId, u);
			break;
		}
		case ChainPackProtocol::TypeInfo::META_TYPE_NAMESPACE_ID:  {
			pos++;
			RpcValue::UInt u = read_UIntData<RpcValue::UInt>(data, pos);
			ret.setValue(RpcValue::Tag::MetaTypeNameSpaceId, u);
			break;
		}
		case ChainPackProtocol::TypeInfo::MetaIMap:  {
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

ChainPackProtocol::TypeInfo::Enum ChainPackProtocol::readTypeInfo(const ChainPackProtocol::Blob &data, ChainPackProtocol::Blob::size_type &pos, RpcValue &meta, int &tiny_uint)
{
	if(pos >= data.length())
		SHV_EXCEPTION("Not enough data to read type info.");
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
		return ChainPackProtocol::TypeInfo::UInt;
	}
	else {
		ChainPackProtocol::TypeInfo::Enum type = (ChainPackProtocol::TypeInfo::Enum)t;
		return type;
	}
}

RpcValue ChainPackProtocol::readData(ChainPackProtocol::TypeInfo::Enum type, bool is_array, const ChainPackProtocol::Blob &data, ChainPackProtocol::Blob::size_type &pos)
{
	RpcValue ret;
	if(is_array) {
		RpcValue::Array val = readData_Array(type, data, pos);
		ret = RpcValue(val);
	}
	else {
		switch (type) {
		case ChainPackProtocol::TypeInfo::Null:
			ret = RpcValue(nullptr);
			break;
		case ChainPackProtocol::TypeInfo::UInt: { RpcValue::UInt u = read_UIntData<RpcValue::UInt>(data, pos); ret = RpcValue(u); break; }
		case ChainPackProtocol::TypeInfo::Int: { RpcValue::Int i = read_IntData<RpcValue::Int>(data, pos); ret = RpcValue(i); break; }
		case ChainPackProtocol::TypeInfo::Double: { double d = read_Double(data, pos); ret = RpcValue(d); break; }
		case ChainPackProtocol::TypeInfo::TRUE: { bool b = true; ret = RpcValue(b); break; }
		case ChainPackProtocol::TypeInfo::FALSE: { bool b = false; ret = RpcValue(b); break; }
		case ChainPackProtocol::TypeInfo::String: { RpcValue::String val = read_Blob<RpcValue::String>(data, pos); ret = RpcValue(val); break; }
		case ChainPackProtocol::TypeInfo::Blob: { RpcValue::Blob val = read_Blob<RpcValue::Blob>(data, pos); ret = RpcValue(val); break; }
		case ChainPackProtocol::TypeInfo::List: { RpcValue::List val = readData_List(data, pos); ret = RpcValue(val); break; }
		case ChainPackProtocol::TypeInfo::Map: { RpcValue::Map val = readData_Map(data, pos); ret = RpcValue(val); break; }
		case ChainPackProtocol::TypeInfo::IMap: { RpcValue::IMap val = readData_IMap(data, pos); ret = RpcValue(val); break; }
		case ChainPackProtocol::TypeInfo::DateTime: { RpcValue::DateTime val = read_DateTime(data, pos); ret = RpcValue(val); break; }
		default:
			SHV_EXCEPTION("Internal error: attempt to read helper type directly. type: " + std::to_string(type) + " " + TypeInfo::name(type));
		}
	}
	return ret;
}

}}
