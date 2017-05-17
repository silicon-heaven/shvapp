/* Copyright (c) 2013 Dropbox, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "chainpackvalue.h"
#include "jsonprotocol.h"

#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <sstream>
#include <iostream>
//#include <utility>

#ifdef _WIN32
namespace {
int is_leap(unsigned y)
{
	y += 1900;
	return (y % 4) == 0 && ((y % 100) != 0 || (y % 400) == 0);
}

time_t timegm(struct tm *tm)
{
	static const unsigned ndays[2][12] = {
		{31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
		{31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}
	};
	time_t res = 0;
	int i;

	for (i = 70; i < tm->tm_year; ++i) {
		res += is_leap(i) ? 366 : 365;
	}

	for (i = 0; i < tm->tm_mon; ++i) {
		res += ndays[is_leap(tm->tm_year)][i];
	}
	res += tm->tm_mday - 1;
	res *= 24;
	res += tm->tm_hour;
	res *= 60;
	res += tm->tm_min;
	res *= 60;
	res += tm->tm_sec;
	return res;
}
} // namespace
#endif

namespace shv {
namespace chainpackrpc {

/*
using std::string;
*/
class Value::AbstractValueData
{
public:
	virtual ~AbstractValueData() {}

	virtual Value::Type::Enum type() const {return Value::Type::Invalid;}
	/*
	virtual Value metaValue(Value::UInt key) const = 0;
	virtual Value metaValue(const Value::String &key) const = 0;
	virtual void setMetaValue(Value::UInt key, const Value &val) = 0;
	virtual void setMetaValue(const Value::String &key, const Value &val) = 0;
	*/
	virtual const Value::MetaData &metaData() const = 0;
	virtual void setMetaData(Value::MetaData &&meta_data) = 0;

	virtual bool equals(const AbstractValueData * other) const = 0;
	//virtual bool less(const Data * other) const = 0;

	virtual void dumpText(std::string &out) const = 0;
	virtual void dumpJson(std::string &out) const = 0;

	virtual bool isNull() const {return false;}
	virtual double toDouble() const {return 0;}
	virtual Value::Int toInt() const {return 0;}
	virtual Value::UInt toUInt() const {return 0;}
	virtual bool toBool() const {return false;}
	virtual Value::DateTime toDateTime() const { return Value::DateTime{}; }
	virtual const std::string &toString() const;
	virtual const Value::Blob &toBlob() const;
	virtual const Value::List &toList() const;
	virtual const Value::Map &toMap() const;
	virtual const Value::IMap &toIMap() const;
	virtual size_t count() const {return 0;}

	virtual const Value &at(Value::UInt i) const;
	virtual const Value &at(const Value::String &key) const;
	virtual void set(Value::UInt ix, const Value &val);
	virtual void set(const Value::String &key, const Value &val);
};

/* * * * * * * * * * * * * * * * * * * *
 * Value wrappers
 */

template <Value::Type::Enum tag, typename T>
class ValueData : public Value::AbstractValueData
{
protected:
	explicit ValueData(const T &value) : m_value(value) {}
	explicit ValueData(T &&value) : m_value(std::move(value)) {}
	virtual ~ValueData() override
	{
		if(m_metaData)
			delete m_metaData;
	}

	Value::Type::Enum type() const override { return tag; }

	const Value::MetaData &metaData() const
	{
		static Value::MetaData md;
		if(!m_metaData)
			return md;
		return *m_metaData;
	}

	void setMetaData(Value::MetaData &&d)
	{
		if(!m_metaData)
			m_metaData = new Value::MetaData(d);
	}


	void dumpJson(std::string &out) const override
	{
		JsonProtocol::dumpJson(m_value, out);
	}

	virtual bool dumpTextValue(std::string &out) const {dumpJson(out); return true;}
	void dumpText(std::string &out) const override
	{
		out += Value::Type::name(type());
		if(m_metaData) {
			out += '[';
			//m_metaPtr->dumpText(out);
			out += ']';
		}
		std::string s;
		if(dumpTextValue(s)) {
			out += '(' + s + ')';
		}
	}
protected:
	T m_value;
	Value::MetaData *m_metaData = nullptr;
};

class ChainPackDouble final : public ValueData<Value::Type::Double, double>
{
	double toDouble() const override { return m_value; }
	Value::Int toInt() const override { return static_cast<int>(m_value); }
	bool equals(const Value::AbstractValueData * other) const override { return m_value == other->toDouble(); }
	//bool less(const Data * other) const override { return m_value < other->toDouble(); }
public:
	explicit ChainPackDouble(double value) : ValueData(value) {}
};

class ChainPackInt final : public ValueData<Value::Type::Int, signed int>
{
	double toDouble() const override { return m_value; }
	Value::Int toInt() const override { return m_value; }
	Value::UInt toUInt() const override { return (unsigned int)m_value; }
	bool equals(const Value::AbstractValueData * other) const override { return m_value == other->toInt(); }
	//bool less(const Data * other) const override { return m_value < other->toDouble(); }
public:
	explicit ChainPackInt(int value) : ValueData(value) {}
};

class ChainPackUInt : public ValueData<Value::Type::UInt, unsigned int>
{
protected:
	double toDouble() const override { return m_value; }
	Value::Int toInt() const override { return (int)m_value; }
	Value::UInt toUInt() const override { return m_value; }
protected:
	bool equals(const Value::AbstractValueData * other) const override { return m_value == other->toUInt(); }
	//bool less(const Data * other) const override { return m_value < other->toDouble(); }
public:
	explicit ChainPackUInt(unsigned int value) : ValueData(value) {}
};

class ChainPackBoolean final : public ValueData<Value::Type::Bool, bool>
{
	bool toBool() const override { return m_value; }
	Value::Int toInt() const override { return m_value? true: false; }
	Value::UInt toUInt() const override { return toInt(); }
	bool equals(const Value::AbstractValueData * other) const override { return m_value == other->toBool(); }
public:
	explicit ChainPackBoolean(bool value) : ValueData(value) {}
};

class ChainPackDateTime final : public ValueData<Value::Type::DateTime, Value::DateTime>
{
	bool toBool() const override { return m_value.msecs != 0; }
	Value::Int toInt() const override { return m_value.msecs; }
	Value::UInt toUInt() const override { return m_value.msecs; }
	Value::DateTime toDateTime() const override { return m_value; }
	bool equals(const Value::AbstractValueData * other) const override { return m_value.msecs == other->toInt(); }
public:
	explicit ChainPackDateTime(Value::DateTime value) : ValueData(value) {}
};

class ChainPackString : public ValueData<Value::Type::String, Value::String>
{
	const std::string &toString() const override { return m_value; }
	bool equals(const Value::AbstractValueData * other) const override { return m_value == other->toString(); }
public:
	explicit ChainPackString(const Value::String &value) : ValueData(value) {}
	explicit ChainPackString(Value::String &&value) : ValueData(std::move(value)) {}
};

class ChainPackBlob final : public ValueData<Value::Type::Blob, Value::Blob>
{
	const Value::Blob &toBlob() const override { return m_value; }
	bool equals(const Value::AbstractValueData * other) const override { return m_value == other->toBlob(); }
public:
	explicit ChainPackBlob(const Value::Blob &value) : ValueData(value) {}
	explicit ChainPackBlob(Value::Blob &&value) : ValueData(std::move(value)) {}
	explicit ChainPackBlob(const uint8_t *bytes, size_t size) : ValueData(Value::Blob()) {
		m_value.reserve(size);
		for (size_t i = 0; i < size; ++i) {
			m_value[i] = bytes[i];
		}
	}
};

class ChainPackList final : public ValueData<Value::Type::List, Value::List>
{
	size_t count() const override {return m_value.size();}
	const Value & at(Value::UInt i) const override;
	bool dumpTextValue(std::string &out) const override
	{
		bool first = true;
		for (const auto &value : m_value) {
			if (!first)
				out += ",";
			value.dumpText(out);
			first = false;
		}
		return true;
	}
	bool equals(const Value::AbstractValueData * other) const override { return m_value == other->toList(); }
public:
	explicit ChainPackList(const Value::List &value) : ValueData(value) {}
	explicit ChainPackList(Value::List &&value) : ValueData(move(value)) {}

	const Value::List &toList() const override { return m_value; }
};
/*
class ChainPackTable final : public ValueData<Value::Type::Table, Value::List>
{
	size_t count() const override {return m_value.size();}
	const Value & operator[](Value::UInt i) const override;
	bool dumpTextValue(std::string &out) const override
	{
		bool first = true;
		for (const auto &value : m_value) {
			if (!first)
				out += ",";
			value.dumpText(out);
			first = false;
		}
		return true;
	}
	bool equals(const Value::AbstractValueData * other) const override { return m_value == other->toList(); }
public:
	explicit ChainPackTable(const Value::Table &value) : ValueData(value) {}
	explicit ChainPackTable(Value::Table &&value) : ValueData(move(value)) {}
	//explicit ChainPackTable(const ChainPack::List &value) : ValueData(value) {}
	//explicit ChainPackTable(ChainPack::List &&value) : ValueData(move(value)) {}

	const Value::List &toList() const override { return m_value; }
};
*/
class ChainPackMap final : public ValueData<Value::Type::Map, Value::Map>
{
	//const ChainPack::Map &toMap() const override { return m_value; }
	size_t count() const override {return m_value.size();}
	const Value & at(const Value::String &key) const override;
	bool dumpTextValue(std::string &out) const override
	{
		bool first = true;
		for (const auto &kv : m_value) {
			if (!first)
				out += ",";
			JsonProtocol::dumpJson(kv.first, out);
			out += ":";
			kv.second.dumpText(out);
			first = false;
		}
		return true;
	}
	bool equals(const Value::AbstractValueData * other) const override { return m_value == other->toMap(); }
public:
	explicit ChainPackMap(const Value::Map &value) : ValueData(value) {}
	explicit ChainPackMap(Value::Map &&value) : ValueData(move(value)) {}

	const Value::Map &toMap() const override { return m_value; }
};

class ChainPackIMap final : public ValueData<Value::Type::IMap, Value::IMap>
{
	//const ChainPack::Map &toMap() const override { return m_value; }
	size_t count() const override {return m_value.size();}
	const Value & at(Value::UInt key) const override;
	bool dumpTextValue(std::string &out) const override
	{
		bool first = true;
		for (const auto &kv : m_value) {
			if (!first)
				out += ",";
			JsonProtocol::dumpJson(kv.first, out);
			out += ":";
			kv.second.dumpText(out);
			first = false;
		}
		return true;
	}
	bool equals(const Value::AbstractValueData * other) const override { return m_value == other->toIMap(); }
public:
	explicit ChainPackIMap(const Value::IMap &value) : ValueData(value) {}
	explicit ChainPackIMap(Value::IMap &&value) : ValueData(move(value)) {}

	const Value::IMap &toIMap() const override { return m_value; }
};

class ChainPackNull final : public ValueData<Value::Type::Null, std::nullptr_t>
{
	bool isNull() const override {return true;}
	bool equals(const Value::AbstractValueData * other) const override { return other->isNull(); }
public:
	ChainPackNull() : ValueData({}) {}
};
/*
class ChainPackMetaTypeId final : public ChainPackUInt
{
public:
	explicit ChainPackMetaTypeId(const Value::MetaTypeId value) : ChainPackUInt(value.id) { }

	Value::Type::Enum type() const override { return Value::Type::MetaTypeId; }
};

class ChainPackMetaTypeNameSpaceId final : public ChainPackUInt
{
public:
	explicit ChainPackMetaTypeNameSpaceId(const Value::MetaTypeNameSpaceId value) : ChainPackUInt(value.id) {}

	Value::Type::Enum type() const override { return Value::Type::MetaTypeNameSpaceId; }
};

class ChainPackMetaTypeName final : public ChainPackString
{
public:
	explicit ChainPackMetaTypeName(const Value::String value) : ChainPackString(value) {}

	Value::Type::Enum type() const override { return Value::Type::MetaTypeName; }
};

class ChainPackMetaTypeNameSpaceName final : public ChainPackString
{
public:
	explicit ChainPackMetaTypeNameSpaceName(const Value::String value) : ChainPackString(value) {}

	Value::Type::Enum type() const override { return Value::Type::MetaTypeNameSpaceName; }
};
*/
/* * * * * * * * * * * * * * * * * * * *
 * Static globals - static-init-safe
 */
struct Statics
{
	const std::shared_ptr<Value::AbstractValueData> null = std::make_shared<ChainPackNull>();
	const std::shared_ptr<Value::AbstractValueData> t = std::make_shared<ChainPackBoolean>(true);
	const std::shared_ptr<Value::AbstractValueData> f = std::make_shared<ChainPackBoolean>(false);
	const Value::String empty_string;
	const Value::Blob empty_blob;
	//const std::vector<ChainPack> empty_vector;
	//const std::map<ChainPack::String, ChainPack> empty_map;
	Statics() {}
};

static const Statics & statics()
{
	static const Statics s {};
	return s;
}

static const Value::String & static_empty_string() { return statics().empty_string; }
static const Value::Blob & static_empty_blob() { return statics().empty_blob; }

static const Value & static_chain_pack_invalid() { static const Value s{}; return s; }
//static const ChainPack & static_chain_pack_null() { static const ChainPack s{statics().null}; return s; }
static const Value::List & static_empty_list() { static const Value::List s{}; return s; }
static const Value::Map & static_empty_map() { static const Value::Map s{}; return s; }
static const Value::IMap & static_empty_imap() { static const Value::IMap s{}; return s; }

/* * * * * * * * * * * * * * * * * * * *
 * Constructors
 */

Value::Value() noexcept {}
Value::Value(std::nullptr_t) noexcept : m_ptr(statics().null) {}
Value::Value(double value) : m_ptr(std::make_shared<ChainPackDouble>(value)) {}
Value::Value(Int value) : m_ptr(std::make_shared<ChainPackInt>(value)) {}
Value::Value(UInt value) : m_ptr(std::make_shared<ChainPackUInt>(value)) {}
Value::Value(bool value) : m_ptr(value ? statics().t : statics().f) {}
Value::Value(const Value::DateTime &value) : m_ptr(std::make_shared<ChainPackDateTime>(value)) {}

Value::Value(const Value::Blob &value) : m_ptr(std::make_shared<ChainPackBlob>(value)) {}
Value::Value(Value::Blob &&value) : m_ptr(std::make_shared<ChainPackBlob>(std::move(value))) {}
Value::Value(const uint8_t * value, size_t size) : m_ptr(std::make_shared<ChainPackBlob>(value, size)) {}
Value::Value(const std::string &value) : m_ptr(std::make_shared<ChainPackString>(value)) {}
Value::Value(std::string &&value) : m_ptr(std::make_shared<ChainPackString>(std::move(value))) {}
Value::Value(const char * value) : m_ptr(std::make_shared<ChainPackString>(value)) {}
Value::Value(const Value::List &values) : m_ptr(std::make_shared<ChainPackList>(values)) {}
Value::Value(Value::List &&values) : m_ptr(std::make_shared<ChainPackList>(move(values))) {}

//Value::Value(const Value::Table &values) : m_ptr(std::make_shared<ChainPackTable>(values)) {}
//Value::Value(Value::Table &&values) : m_ptr(std::make_shared<ChainPackTable>(move(values))) {}

Value::Value(const Value::Map &values) : m_ptr(std::make_shared<ChainPackMap>(values)) {}
Value::Value(Value::Map &&values) : m_ptr(std::make_shared<ChainPackMap>(move(values))) {}

Value::Value(const Value::IMap &values) : m_ptr(std::make_shared<ChainPackIMap>(values)) {}
Value::Value(Value::IMap &&values) : m_ptr(std::make_shared<ChainPackIMap>(move(values))) {}

//Value::Value(const Value::MetaTypeId &value) : m_ptr(std::make_shared<ChainPackMetaTypeId>(value)) {}
//Value::Value(const Value::MetaTypeNameSpaceId &value) : m_ptr(std::make_shared<ChainPackMetaTypeNameSpaceId>(value)) {}
//Value::Value(const Value::MetaTypeName &value) : m_ptr(std::make_shared<ChainPackMetaTypeName>(value)) {}
//Value::Value(const Value::MetaTypeNameSpaceName &value) : m_ptr(std::make_shared<ChainPackMetaTypeNameSpaceName>(value)) {}
/*
ChainPack ChainPack::fromType(ChainPack::Type::Enum t)
{
	switch (t) {
	case Type::Invalid: return ChainPack();
	case Type::Null: return ChainPack(nullptr);
	case Type::UInt: return ChainPack((unsigned int)0);
	case Type::Int: return ChainPack((signed int)0);
	case Type::Double: return ChainPack(0.);
	case Type::Bool: return ChainPack(false);
	case Type::Blob: return ChainPack(Blob());
	case Type::String: return ChainPack(ChainPack::String());
	case Type::List: ChainPack(List());
	case Type::Table: ChainPack(Table());
	case Type::Map: return ChainPack(Map());
	case Type::DateTime: return ChainPack(DateTime{});
	case Type::MetaTypeId: return ChainPack(DateTime{});
	case Type::MetaTypeNameSpaceId: return ChainPack(DateTime{});
	case Type::MetaTypeName: return ChainPack(DateTime{});
	case Type::MetaTypeNameSpaceName: return ChainPack(DateTime{});

	case Type::True: return "True";
	case Type::False: return "False";
	case Type::TERM: return "TERM";
	default:
		throw std::runtime_error("Internal error: attempt to create object of helper type. type: " + std::to_string(t));
	}
}
*/
//Value::Value(const std::shared_ptr<AbstractValueData> &r) : m_ptr(r) {}

/* * * * * * * * * * * * * * * * * * * *
 * Accessors
 */

Value::Type::Enum Value::type() const { return m_ptr? m_ptr->type(): Type::Invalid; }

const Value::MetaData &Value::metaData() const
{
	static MetaData md;
	if(m_ptr)
		return m_ptr->metaData();
	return md;
}

void Value::setMetaData(Value::MetaData &&meta_data)
{
	if(!m_ptr && !meta_data.isEmpty())
		throw std::runtime_error("Cannot set valid meta data to invalid ChainPack value!");
	if(m_ptr)
		m_ptr->setMetaData(std::move(meta_data));
}
/*
Value Value::metaValue(Value::UInt key) const
{
	if(m_ptr)
		return m_ptr->metaValue(key);
	return Value();
}

Value Value::metaValue(const Value::String &key) const
{
	if(m_ptr)
		return m_ptr->metaValue(key);
	return Value();
}

void Value::setMetaValue(Value::UInt key, const Value &val)
{
	if(m_ptr)
		m_ptr->setMetaValue(key, val);
	else if(val.isValid())
		throw std::runtime_error("Cannot set valid meta to invalid ChainPack value!");
}

void Value::setMetaValue(const Value::String &key, const Value &val)
{
	if(m_ptr)
		m_ptr->setMetaValue(key, val);
	else if(val.isValid())
		throw std::runtime_error("Cannot set valid meta to invalid ChainPack value!");
}
*/
/*
Value Value::meta() const { return m_ptr? m_ptr->meta(): Value{}; }

void Value::setMeta(const Value &m)
{
	if(m_ptr)
		m_ptr->setMeta(m.m_ptr);
	else if(m.isValid())
		throw std::runtime_error("Cannot set valid meta to invalid ChainPack value!");
}
*/
bool Value::isValid() const
{
	return !!m_ptr;
}

double Value::toDouble() const { return m_ptr? m_ptr->toDouble(): 0; }
Value::Int Value::toInt() const { return m_ptr? m_ptr->toInt(): 0; }
Value::UInt Value::toUInt() const { return m_ptr? m_ptr->toUInt(): 0; }
bool Value::toBool() const { return m_ptr? m_ptr->toBool(): false; }
Value::DateTime Value::toDateTime() const { return m_ptr? m_ptr->toDateTime(): Value::DateTime{}; }

const std::string & Value::toString() const { return m_ptr? m_ptr->toString(): static_empty_string(); }
const Value::Blob &Value::toBlob() const { return m_ptr? m_ptr->toBlob(): static_empty_blob(); }

int Value::count() const { return m_ptr? m_ptr->count(): 0; }
const Value::List & Value::toList() const { return m_ptr? m_ptr->toList(): static_empty_list(); }
const Value::Map & Value::toMap() const { return m_ptr? m_ptr->toMap(): static_empty_map(); }
const Value::IMap &Value::toIMap() const { return m_ptr? m_ptr->toIMap(): static_empty_imap(); }
const Value & Value::at (Value::UInt i) const { return m_ptr? m_ptr->at(i): static_chain_pack_invalid(); }
const Value & Value::at (const Value::String &key) const { return m_ptr? m_ptr->at(key): static_chain_pack_invalid(); }

void Value::set(Value::UInt ix, const Value &val)
{
	if(m_ptr)
		m_ptr->set(ix, val);
	else
		std::cerr << __FILE__ << ':' << __LINE__ << " Cannot set value to invalid ChainPack value! Index: " << ix << std::endl;
}

void Value::set(const Value::String &key, const Value &val)
{
	if(m_ptr)
		m_ptr->set(key, val);
	else
		std::cerr << __FILE__ << ':' << __LINE__ << " Cannot set value to invalid ChainPack value! Key: " << key << std::endl;
}

const std::string & Value::AbstractValueData::toString() const { return static_empty_string(); }
const Value::Blob & Value::AbstractValueData::toBlob() const { return static_empty_blob(); }
const Value::List & Value::AbstractValueData::toList() const { return static_empty_list(); }
const Value::Map & Value::AbstractValueData::toMap() const { return static_empty_map(); }
const Value::IMap & Value::AbstractValueData::toIMap() const { return static_empty_imap(); }

const Value & Value::AbstractValueData::at(Value::UInt) const { return static_chain_pack_invalid(); }
const Value & Value::AbstractValueData::at(const Value::String &) const { return static_chain_pack_invalid(); }

void Value::AbstractValueData::set(Value::UInt ix, const Value &)
{
	std::cerr << __FILE__ << ':' << __LINE__ << "Value::AbstractValueData::set: trivial implementation called! Key: " << ix << std::endl;
}

void Value::AbstractValueData::set(const Value::String &key, const Value &)
{
	std::cerr << __FILE__ << ':' << __LINE__ << "Value::AbstractValueData::set: trivial implementation called! Key: " << key << std::endl;
}


const Value & ChainPackList::at(Value::UInt i) const
{
	if (i >= m_value.size())
		return static_chain_pack_invalid();
	else
		return m_value[i];
}
/*
const Value &ChainPackTable::operator[](Value::UInt i) const
{
	if (i >= m_value.size())
		return static_chain_pack_invalid();
	else
		return m_value[i];
}
*/
/* * * * * * * * * * * * * * * * * * * *
 * Comparison
 */
bool Value::operator== (const Value &other) const
{
	if(isValid() && other.isValid()) {
		if (m_ptr->type() != other.m_ptr->type())
			return false;
		return m_ptr->equals(other.m_ptr.get());
	}
	return (!isValid() && !other.isValid());
}
/*
bool ChainPack::operator< (const ChainPack &other) const
{
	if(isValid() && other.isValid()) {
		if (m_ptr->type() != other.m_ptr->type())
			return m_ptr->type() < other.m_ptr->type();
		return m_ptr->less(other.m_ptr.get());
	}
	return (!isValid() && other.isValid());
}
*/

Value Value::parseJson(const std::string &in, std::string &err)
{
	return JsonProtocol::parseJson(in, err);
}

Value Value::parseJson(const char *in, std::string &err)
{
	return JsonProtocol::parseJson(in, err);
}

void Value::dumpText(std::string &out) const
{
	if(m_ptr)
		m_ptr->dumpText(out);
}

void Value::dumpJson(std::string &out) const
{
	if(m_ptr)
		m_ptr->dumpJson(out);
	else
		out += "INVALID";
}

/* * * * * * * * * * * * * * * * * * * *
 * Shape-checking
 */
/*
bool ChainPack::has_shape(const shape & types, std::string & err) const
{
	if (!isMap()) {
		err = "expected JSON object, got " + dumpJson();
		return false;
	}

	for (auto & item : types) {
		if ((*this)[item.first].type() != item.second) {
			err = "bad type for " + item.first + " in " + dumpJson();
			return false;
		}
	}

	return true;
}
*/
const char *Value::Type::name(Value::Type::Enum e)
{
	switch (e) {
	case Invalid: return "INVALID";
	case Null: return "Null";
	case UInt: return "UInt";
	case Int: return "Int";
	case Double: return "Double";
	case Bool: return "Bool";
	case Blob: return "Blob";
	case String: return "String";
	case List: return "List";
	//case Table: return "Table";
	case Map: return "Map";
	case IMap: return "IMap";
	case DateTime: return "DateTime";
	case MetaTypeId: return "MetaTypeId";
	case MetaTypeNameSpaceId: return "MetaTypeNameSpaceId";
	case MetaIMap: return "MetaIMap";
	case TRUE: return "TRUE";
	case FALSE: return "FALSE";
	case TERM: return "TERM";
	default:
		return "UNKNOWN";
	}
}

const Value & ChainPackMap::at(const Value::String &key) const
{
	auto iter = m_value.find(key);
	return (iter == m_value.end()) ? static_chain_pack_invalid() : iter->second;
}

const Value & ChainPackIMap::at(Value::UInt key) const
{
	auto iter = m_value.find(key);
	return (iter == m_value.end()) ? static_chain_pack_invalid() : iter->second;
}

Value::DateTime Value::DateTime::fromString(const std::string &local_date_time_str)
{
	std::istringstream iss(local_date_time_str);
	unsigned int day = 0, month = 0, year = 0, hour = 0, min = 0, sec = 0, msec = 0;
	char dsep, tsep, msep;

	DateTime ret;
	ret.msecs = 0;

	if (iss >> year >> dsep >> month >> dsep >> day >> tsep >> hour >> tsep >> min >> tsep >> sec >> msep >> msec) {
		std::tm tm;
		tm.tm_year = year - 1900;
		tm.tm_mon = month - 1;
		tm.tm_mday = day;
		tm.tm_hour = hour;
		tm.tm_min = min;
		tm.tm_sec = sec;
		tm.tm_isdst = -1;
		std::time_t tim = std::mktime(&tm);
		if(tim == -1) {
			std::cerr << "Invalid date time string: " << local_date_time_str;
		}
		else {
			ret.msecs = tim * 1000 + msec;
		}
	}
	return ret;
}

Value::DateTime Value::DateTime::fromUtcString(const std::string &utc_date_time_str)
{
	std::istringstream iss(utc_date_time_str);
	unsigned int day = 0, month = 0, year = 0, hour = 0, min = 0, sec = 0, msec = 0;
	char dsep, tsep, msep;

	DateTime ret;
	ret.msecs = 0;

	if (iss >> year >> dsep >> month >> dsep >> day >> tsep >> hour >> tsep >> min >> tsep >> sec >> msep >> msec) {
		std::tm tm;
		tm.tm_year = year - 1900;
		tm.tm_mon = month - 1;
		tm.tm_mday = day;
		tm.tm_hour = hour;
		tm.tm_min = min;
		tm.tm_sec = sec;
		tm.tm_isdst = 0;
		std::time_t tim = timegm(&tm);
		if(tim == -1) {
			std::cerr << "Invalid date time string: " << utc_date_time_str;
		}
		else {
			ret.msecs = tim * 1000 + msec;
		}
	}
	return ret;
}

std::string Value::DateTime::toString() const
{
	std::time_t tim = msecs / 1000;
	std::tm *tm = std::localtime(&tim);
	if(tm == nullptr) {
		std::cerr << "Invalid date time: " << msecs;
	}
	else {
		char buffer[80];
		std::strftime(buffer, sizeof(buffer),"%Y-%m-%dT%H:%M:%S",tm);
		std::string ret(buffer);
		ret += '.' + std::to_string(msecs % 1000);
		return ret;
	}
	return std::string();
}

std::string Value::DateTime::toUtcString() const
{
	std::time_t tim = msecs / 1000;
	std::tm *tm = std::gmtime(&tim);
	if(tm == nullptr) {
		std::cerr << "Invalid date time: " << msecs;
	}
	else {
		char buffer[80];
		std::strftime(buffer, sizeof(buffer),"%Y-%m-%dT%H:%M:%S",tm);
		std::string ret(buffer);
		ret += '.' + std::to_string(msecs % 1000);
		return ret;
	}
	return std::string();
}

std::vector<Value::UInt> Value::MetaData::ikeys() const
{
	std::vector<Value::UInt> ret;
	for(const auto &it : m_imap)
		ret.push_back(it.first);
	return ret;
}

Value Value::MetaData::value(Value::UInt key) const
{
	auto it = m_imap.find(key);
	if(it != m_imap.end())
		return it->second;
	return Value();
}

void Value::MetaData::setValue(Value::UInt key, const Value &val)
{
	m_imap[key] = val;
}

}}
