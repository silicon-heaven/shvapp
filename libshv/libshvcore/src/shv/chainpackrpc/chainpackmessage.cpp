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

#include "chainpackmessage.h"
#include "protocol/jsonprotocol.h"

#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <sstream>
#include <iostream>

namespace shv {
namespace chainpackrpc {

/*
using std::string;
*/
class Message::AbstractValueData
{
public:
	virtual ~AbstractValueData() {}

	virtual Message::Type::Enum type() const {return Message::Type::Invalid;}

	virtual Message meta() const = 0;
	virtual void setMeta(const std::shared_ptr<AbstractValueData> &m) = 0;

	virtual bool equals(const AbstractValueData * other) const = 0;
	//virtual bool less(const Data * other) const = 0;

	virtual void dumpText(std::string &out) const = 0;
	virtual void dumpJson(std::string &out) const = 0;

	virtual bool isNull() const {return false;}
	virtual double toDouble() const {return 0;}
	virtual int toInt() const {return 0;}
	virtual unsigned int toUInt() const {return 0;}
	virtual bool toBool() const {return false;}
	virtual Message::DateTime toDateTime() const { return Message::DateTime{}; }
	virtual const std::string &toString() const;
	virtual const Message::Blob &toBlob() const;
	virtual const Message::List &toList() const;
	virtual const Message::Map &toMap() const;
	virtual size_t count() const {return 0;}
	virtual const Message &operator[](size_t i) const;
	virtual const Message &operator[](const Message::String &key) const;
};

/* * * * * * * * * * * * * * * * * * * *
 * Value wrappers
 */

template <Message::Type::Enum tag, typename T>
class ValueData : public Message::AbstractValueData
{
protected:
	explicit ValueData(const T &value) : m_value(value) {}
	explicit ValueData(T &&value) : m_value(std::move(value)) {}

	Message::Type::Enum type() const override { return tag; }
	Message meta() const override { return (!m_metaPtr)? Message{} : Message{m_metaPtr}; }
	void setMeta(const std::shared_ptr<AbstractValueData> &m) override { m_metaPtr = m; }

	void dumpJson(std::string &out) const override
	{
		protocol::JsonProtocol::dumpJson(m_value, out);
	}
	virtual bool dumpTextValue(std::string &out) const {dumpJson(out); return true;}
	void dumpText(std::string &out) const override
	{
		out += Message::Type::name(type());
		if(m_metaPtr) {
			out += '[';
			m_metaPtr->dumpText(out);
			out += ']';
		}
		std::string s;
		if(dumpTextValue(s)) {
			out += '(' + s + ')';
		}
	}
	// Comparisons
	/*
	bool equals(const Data * other) const override
	{
		return m_value == static_cast<const CPValue<tag, T> *>(other)->m_value;
	}

	bool less(const Data * other) const override
	{
		return m_value < static_cast<const CPValue<tag, T> *>(other)->m_value;
	}
	*/
protected:
	T m_value;
	std::shared_ptr<AbstractValueData> m_metaPtr;
};

class ChainPackDouble final : public ValueData<Message::Type::Double, double>
{
	double toDouble() const override { return m_value; }
	int toInt() const override { return static_cast<int>(m_value); }
	bool equals(const Message::AbstractValueData * other) const override { return m_value == other->toDouble(); }
	//bool less(const Data * other) const override { return m_value < other->toDouble(); }
public:
	explicit ChainPackDouble(double value) : ValueData(value) {}
};

class ChainPackInt final : public ValueData<Message::Type::Int, signed int>
{
	double toDouble() const override { return m_value; }
	int toInt() const override { return m_value; }
	unsigned int toUInt() const override { return (unsigned int)m_value; }
	bool equals(const Message::AbstractValueData * other) const override { return m_value == other->toInt(); }
	//bool less(const Data * other) const override { return m_value < other->toDouble(); }
public:
	explicit ChainPackInt(int value) : ValueData(value) {}
};

class ChainPackUInt : public ValueData<Message::Type::UInt, unsigned int>
{
protected:
	double toDouble() const override { return m_value; }
	int toInt() const override { return (int)m_value; }
	unsigned int toUInt() const override { return m_value; }
protected:
	bool equals(const Message::AbstractValueData * other) const override { return m_value == other->toUInt(); }
	//bool less(const Data * other) const override { return m_value < other->toDouble(); }
public:
	explicit ChainPackUInt(unsigned int value) : ValueData(value) {}
};

class ChainPackBoolean final : public ValueData<Message::Type::Bool, bool>
{
	bool toBool() const override { return m_value; }
	int toInt() const override { return m_value? true: false; }
	unsigned int toUInt() const override { return toInt(); }
	bool equals(const Message::AbstractValueData * other) const override { return m_value == other->toBool(); }
public:
	explicit ChainPackBoolean(bool value) : ValueData(value) {}
};

class ChainPackDateTime final : public ValueData<Message::Type::DateTime, Message::DateTime>
{
	bool toBool() const override { return m_value.msecs != 0; }
	int toInt() const override { return m_value.msecs; }
	unsigned int toUInt() const override { return m_value.msecs; }
	Message::DateTime toDateTime() const override { return m_value; }
	bool equals(const Message::AbstractValueData * other) const override { return m_value.msecs == other->toInt(); }
public:
	explicit ChainPackDateTime(Message::DateTime value) : ValueData(value) {}
};

class ChainPackString : public ValueData<Message::Type::String, Message::String>
{
	const std::string &toString() const override { return m_value; }
	bool equals(const Message::AbstractValueData * other) const override { return m_value == other->toString(); }
public:
	explicit ChainPackString(const Message::String &value) : ValueData(value) {}
	explicit ChainPackString(Message::String &&value) : ValueData(std::move(value)) {}
};

class ChainPackBlob final : public ValueData<Message::Type::Blob, Message::Blob>
{
	const Message::Blob &toBlob() const override { return m_value; }
	bool equals(const Message::AbstractValueData * other) const override { return m_value == other->toBlob(); }
public:
	explicit ChainPackBlob(const Message::Blob &value) : ValueData(value) {}
	explicit ChainPackBlob(Message::Blob &&value) : ValueData(std::move(value)) {}
	explicit ChainPackBlob(const uint8_t *bytes, size_t size) : ValueData(Message::Blob()) {
		m_value.reserve(size);
		for (size_t i = 0; i < size; ++i) {
			m_value[i] = bytes[i];
		}
	}
};

class ChainPackList final : public ValueData<Message::Type::List, Message::List>
{
	size_t count() const override {return m_value.size();}
	const Message & operator[](size_t i) const override;
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
	bool equals(const Message::AbstractValueData * other) const override { return m_value == other->toList(); }
public:
	explicit ChainPackList(const Message::List &value) : ValueData(value) {}
	explicit ChainPackList(Message::List &&value) : ValueData(move(value)) {}

	const Message::List &toList() const override { return m_value; }
};

class ChainPackTable final : public ValueData<Message::Type::Table, Message::List>
{
	size_t count() const override {return m_value.size();}
	const Message & operator[](size_t i) const override;
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
	bool equals(const Message::AbstractValueData * other) const override { return m_value == other->toList(); }
public:
	explicit ChainPackTable(const Message::Table &value) : ValueData(value) {}
	explicit ChainPackTable(Message::Table &&value) : ValueData(move(value)) {}
	//explicit ChainPackTable(const ChainPack::List &value) : ValueData(value) {}
	//explicit ChainPackTable(ChainPack::List &&value) : ValueData(move(value)) {}

	const Message::List &toList() const override { return m_value; }
};

class ChainPackMap final : public ValueData<Message::Type::Map, Message::Map>
{
	//const ChainPack::Map &toMap() const override { return m_value; }
	size_t count() const override {return m_value.size();}
	const Message & operator[](const Message::String &key) const override;
	bool dumpTextValue(std::string &out) const override
	{
		bool first = true;
		for (const auto &kv : m_value) {
			if (!first)
				out += ",";
			protocol::JsonProtocol::dumpJson(kv.first, out);
			out += ":";
			kv.second.dumpText(out);
			first = false;
		}
		return true;
	}
	bool equals(const Message::AbstractValueData * other) const override { return m_value == other->toMap(); }
public:
	explicit ChainPackMap(const Message::Map &value) : ValueData(value) {}
	explicit ChainPackMap(Message::Map &&value) : ValueData(move(value)) {}

	const Message::Map &toMap() const override { return m_value; }
};

class ChainPackNull final : public ValueData<Message::Type::Null, std::nullptr_t>
{
	bool isNull() const override {return true;}
	bool equals(const Message::AbstractValueData * other) const override { return other->isNull(); }
public:
	ChainPackNull() : ValueData({}) {}
};

class ChainPackMetaTypeId final : public ChainPackUInt
{
public:
	explicit ChainPackMetaTypeId(const Message::MetaTypeId value) : ChainPackUInt(value.id) { }

	Message::Type::Enum type() const override { return Message::Type::MetaTypeId; }
};

class ChainPackMetaTypeNameSpaceId final : public ChainPackUInt
{
public:
	explicit ChainPackMetaTypeNameSpaceId(const Message::MetaTypeNameSpaceId value) : ChainPackUInt(value.id) {}

	Message::Type::Enum type() const override { return Message::Type::MetaTypeNameSpaceId; }
};

class ChainPackMetaTypeName final : public ChainPackString
{
public:
	explicit ChainPackMetaTypeName(const Message::String value) : ChainPackString(value) {}

	Message::Type::Enum type() const override { return Message::Type::MetaTypeName; }
};

class ChainPackMetaTypeNameSpaceName final : public ChainPackString
{
public:
	explicit ChainPackMetaTypeNameSpaceName(const Message::String value) : ChainPackString(value) {}

	Message::Type::Enum type() const override { return Message::Type::MetaTypeNameSpaceName; }
};

/* * * * * * * * * * * * * * * * * * * *
 * Static globals - static-init-safe
 */
struct Statics
{
	const std::shared_ptr<Message::AbstractValueData> null = std::make_shared<ChainPackNull>();
	const std::shared_ptr<Message::AbstractValueData> t = std::make_shared<ChainPackBoolean>(true);
	const std::shared_ptr<Message::AbstractValueData> f = std::make_shared<ChainPackBoolean>(false);
	const Message::String empty_string;
	const Message::Blob empty_blob;
	//const std::vector<ChainPack> empty_vector;
	//const std::map<ChainPack::String, ChainPack> empty_map;
	Statics() {}
};

static const Statics & statics()
{
	static const Statics s {};
	return s;
}

static const Message::String & static_empty_string() { return statics().empty_string; }
static const Message::Blob & static_empty_blob() { return statics().empty_blob; }

static const Message & static_chain_pack_invalid() { static const Message s{}; return s; }
//static const ChainPack & static_chain_pack_null() { static const ChainPack s{statics().null}; return s; }
static const Message::List & static_empty_list() { static const Message::List s{}; return s; }
static const Message::Map & static_empty_map() { static const Message::Map s{}; return s; }

/* * * * * * * * * * * * * * * * * * * *
 * Constructors
 */

Message::Message() noexcept {}
Message::Message(std::nullptr_t) noexcept : m_ptr(statics().null) {}
Message::Message(double value) : m_ptr(std::make_shared<ChainPackDouble>(value)) {}
Message::Message(signed int value) : m_ptr(std::make_shared<ChainPackInt>(value)) {}
Message::Message(unsigned int value) : m_ptr(std::make_shared<ChainPackUInt>(value)) {}
Message::Message(bool value) : m_ptr(value ? statics().t : statics().f) {}
Message::Message(const Message::DateTime &value) : m_ptr(std::make_shared<ChainPackDateTime>(value)) {}

Message::Message(const Message::Blob &value) : m_ptr(std::make_shared<ChainPackBlob>(value)) {}
Message::Message(Message::Blob &&value) : m_ptr(std::make_shared<ChainPackBlob>(std::move(value))) {}
Message::Message(const uint8_t * value, size_t size) : m_ptr(std::make_shared<ChainPackBlob>(value, size)) {}
Message::Message(const std::string &value) : m_ptr(std::make_shared<ChainPackString>(value)) {}
Message::Message(std::string &&value) : m_ptr(std::make_shared<ChainPackString>(std::move(value))) {}
Message::Message(const char * value) : m_ptr(std::make_shared<ChainPackString>(value)) {}
Message::Message(const Message::List &values) : m_ptr(std::make_shared<ChainPackList>(values)) {}
Message::Message(Message::List &&values) : m_ptr(std::make_shared<ChainPackList>(move(values))) {}

Message::Message(const Message::Table &values) : m_ptr(std::make_shared<ChainPackTable>(values)) {}
Message::Message(Message::Table &&values) : m_ptr(std::make_shared<ChainPackTable>(move(values))) {}

Message::Message(const Message::Map &values) : m_ptr(std::make_shared<ChainPackMap>(values)) {}
Message::Message(Message::Map &&values) : m_ptr(std::make_shared<ChainPackMap>(move(values))) {}

Message::Message(const Message::MetaTypeId &value) : m_ptr(std::make_shared<ChainPackMetaTypeId>(value)) {}
Message::Message(const Message::MetaTypeNameSpaceId &value) : m_ptr(std::make_shared<ChainPackMetaTypeNameSpaceId>(value)) {}
Message::Message(const Message::MetaTypeName &value) : m_ptr(std::make_shared<ChainPackMetaTypeName>(value)) {}
Message::Message(const Message::MetaTypeNameSpaceName &value) : m_ptr(std::make_shared<ChainPackMetaTypeNameSpaceName>(value)) {}
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
Message::Message(const std::shared_ptr<AbstractValueData> &r) : m_ptr(r) {}

/* * * * * * * * * * * * * * * * * * * *
 * Accessors
 */

Message::Type::Enum Message::type() const { return m_ptr? m_ptr->type(): Type::Invalid; }
Message Message::meta() const { return m_ptr? m_ptr->meta(): Message{}; }

void Message::setMeta(const Message &m)
{
	if(m_ptr)
		m_ptr->setMeta(m.m_ptr);
	else if(m.isValid())
		throw std::runtime_error("Cannot set valid meta to invalid ChainPack value!");
}

bool Message::isValid() const
{
	return !!m_ptr;
}

double Message::toDouble() const { return m_ptr? m_ptr->toDouble(): 0; }
int Message::toInt() const { return m_ptr? m_ptr->toInt(): 0; }
unsigned int Message::toUInt() const { return m_ptr? m_ptr->toUInt(): 0; }
bool Message::toBool() const { return m_ptr? m_ptr->toBool(): false; }
Message::DateTime Message::toDateTime() const { return m_ptr? m_ptr->toDateTime(): Message::DateTime{}; }

const std::string & Message::toString() const { return m_ptr? m_ptr->toString(): static_empty_string(); }
const Message::Blob &Message::toBlob() const { return m_ptr? m_ptr->toBlob(): static_empty_blob(); }

int Message::count() const { return m_ptr? m_ptr->count(): 0; }
const Message::List & Message::toList() const { return m_ptr? m_ptr->toList(): static_empty_list(); }
const Message::Map & Message::toMap() const { return m_ptr? m_ptr->toMap(): static_empty_map(); }
const Message & Message::operator[] (size_t i) const { return m_ptr? (*m_ptr)[i]: static_chain_pack_invalid(); }
const Message & Message::operator[] (const Message::String &key) const { return m_ptr? (*m_ptr)[key]: static_chain_pack_invalid(); }

const std::string & Message::AbstractValueData::toString() const { return static_empty_string(); }
const Message::Blob & Message::AbstractValueData::toBlob() const { return static_empty_blob(); }
const Message::List & Message::AbstractValueData::toList() const { return static_empty_list(); }
const Message::Map & Message::AbstractValueData::toMap() const { return static_empty_map(); }
const Message & Message::AbstractValueData::operator[] (size_t) const { return static_chain_pack_invalid(); }
const Message & Message::AbstractValueData::operator[] (const Message::String &) const { return static_chain_pack_invalid(); }

const Message & ChainPackList::operator[] (size_t i) const
{
	if (i >= m_value.size())
		return static_chain_pack_invalid();
	else
		return m_value[i];
}

const Message &ChainPackTable::operator[](size_t i) const
{
	if (i >= m_value.size())
		return static_chain_pack_invalid();
	else
		return m_value[i];
}

const Message & ChainPackMap::operator[] (const Message::String &key) const
{
	auto iter = m_value.find(key);
	return (iter == m_value.end()) ? static_chain_pack_invalid() : iter->second;
}

/* * * * * * * * * * * * * * * * * * * *
 * Comparison
 */
bool Message::operator== (const Message &other) const
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

Message Message::parseJson(const std::string &in, std::string &err)
{
	return protocol::JsonProtocol::parseJson(in, err);
}

Message Message::parseJson(const char *in, std::string &err)
{
	return protocol::JsonProtocol::parseJson(in, err);
}

void Message::dumpText(std::string &out) const
{
	if(m_ptr)
		m_ptr->dumpText(out);
}

void Message::dumpJson(std::string &out) const
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
const char *Message::Type::name(Message::Type::Enum e)
{
	switch (e) {
	case Invalid: return "INVALID";
	case TERM: return "TERM";
	case Null: return "Null";
	case UInt: return "UInt";
	case Int: return "Int";
	case Double: return "Double";
	case Bool: return "Bool";
	case Blob: return "Blob";
	case String: return "String";
	case List: return "List";
	case Table: return "Table";
	case Map: return "Map";
	case DateTime: return "DateTime";
	case TRUE: return "True";
	case FALSE: return "False";
	case MetaTypeId: return "MetaTypeId";
	case MetaTypeNameSpaceId: return "MetaTypeNameSpaceId";
	case MetaTypeName: return "MetaTypeName";
	case MetaTypeNameSpaceName: return "MetaTypeNameSpaceName";
	default:
		return "UNKNOWN";
	}
}

Message::DateTime Message::DateTime::fromString(const std::string &local_date_time_str)
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

Message::DateTime Message::DateTime::fromUtcString(const std::string &utc_date_time_str)
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

std::string Message::DateTime::toString() const
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

std::string Message::DateTime::toUtcString() const
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

}}
