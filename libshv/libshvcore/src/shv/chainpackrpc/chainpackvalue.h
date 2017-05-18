#pragma once

#include "../../shvcoreglobal.h"

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <initializer_list>

#ifndef CHAINPACK_UINT
	#define CHAINPACK_UINT unsigned int
#endif
#ifndef CHAINPACK_INT
	#define CHAINPACK_INT signed int
#endif

namespace shv {
namespace chainpackrpc {


class SHVCORE_DECL_EXPORT Value final
{
public:
	class AbstractValueData;

	struct Type {
		enum Enum { Invalid=-1,
					TERM = 128, // first byte of packed Int or UInt cannot be like 0b1000000
					Null,
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
					//MetaMap,
					MetaIMap,
					META_TYPE_ID,
					META_TYPE_NAMESPACE_ID,
					FALSE,
					TRUE,
				  };
		static const char* name(Enum e);
	};
	struct Tag {
		enum Enum { Invalid = 0,
					MetaTypeId,
					MetaTypeNameSpaceId,
					MetaTypeName,
					MetaTypeNameSpaceName,
					USER = 8
				  };
		static const char* name(Enum e);
	};

	using Int = CHAINPACK_INT;
	using UInt = CHAINPACK_UINT;
	struct DateTime
	{
		int64_t msecs = 0;

		DateTime() {}

		static DateTime fromString(const std::string &local_date_time_str);
		static DateTime fromUtcString(const std::string &utc_date_time_str);
		std::string toString() const;
		std::string toUtcString() const;
	};
	using String = std::string;
	struct Blob : public std::basic_string<uint8_t>
	{
		Blob() : std::basic_string<uint8_t>() {}
		Blob(const std::string &str) {
			reserve(str.length());
			for (size_t i = 0; i < str.length(); ++i)
				this->operator +=((uint8_t)str[i]);
		}
		std::string toString() const {
			std::string ret;
			ret.reserve(length());
			for (size_t i = 0; i < length(); ++i)
				ret += (char)(this->operator [](i));
			return ret;
		}
	};
	using List = std::vector<Value>;
	using Map = std::map<Value::String, Value>;
	using IMap = std::map<Value::UInt, Value>;
	class Table : public List
	{
	public:
		Table() : List() {}
		Table(const Table &t) : List(t) {}
		Table(Table &&t) noexcept : List(std::move(t)) {}
		Table(std::initializer_list<value_type> l) : List(l) {}
	};
	struct SHVCORE_DECL_EXPORT MetaData
	{
		std::vector<Value::UInt> ikeys() const;
		Value value(Value::UInt key) const;
		void setValue(Value::UInt key, const Value &val);
		bool isEmpty() const {return m_imap.empty();}
		bool operator==(const MetaData &o) const;
	protected:
		//Value::Map smap;
		Value::IMap m_imap;
	};

	//struct MetaTypeId { uint32_t id = 0; MetaTypeId(uint32_t id) : id(id) {}};
	//struct MetaTypeNameSpaceId { uint32_t id = 0; MetaTypeNameSpaceId(uint32_t id) : id(id) {}};
	//struct MetaTypeName : public String { MetaTypeName(const String &id) : String(id) {} };
	//struct MetaTypeNameSpaceName : public String { MetaTypeNameSpaceName(const String &id) : String(id) {} };

	// Constructors for the various types of JSON value.
	Value() noexcept;                // Null
	Value(std::nullptr_t) noexcept;  // Null
	Value(double value);             // Double
	Value(Int value);                // Int
	Value(UInt value);                // Int
	Value(bool value);               // Bool
	Value(const DateTime &value);
	Value(const Blob &value); // Blob
	Value(Blob &&value);
	Value(const uint8_t *value, size_t size);
	Value(const std::string &value); // String
	Value(std::string &&value);      // String
	Value(const char *value);       // String
	Value(const List &values);      // List
	Value(List &&values);           // List
	Value(const Table &values);
	Value(Table &&values);
	Value(const Map &values);     // Map
	Value(Map &&values);          // Map
	Value(const IMap &values);     // IMap
	Value(IMap &&values);          // IMap

	//Value(const MetaTypeId &value);
	//Value(const MetaTypeNameSpaceId &value);
	//Value(const MetaTypeName &value);
	//Value(const MetaTypeNameSpaceName &value);

	//ChainPack fromType(Type::Enum t);
	//Value(const std::shared_ptr<AbstractValueData> &r);

	// Implicit constructor: anything with a to_json() function.
	template <class T, class = decltype(&T::to_json)>
	Value(const T & t) : Value(t.to_json()) {}

	// Implicit constructor: map-like objects (std::map, std::unordered_map, etc)
	template <class M, typename std::enable_if<
				  std::is_constructible<Value::String, typename M::key_type>::value
				  && std::is_constructible<Value, typename M::mapped_type>::value,
				  int>::type = 0>
	Value(const M & m) : Value(Map(m.begin(), m.end())) {}

	// Implicit constructor: vector-like objects (std::list, std::vector, std::set, etc)
	template <class V, typename std::enable_if<
				  std::is_constructible<Value, typename V::value_type>::value,
				  int>::type = 0>
	Value(const V & v) : Value(List(v.begin(), v.end())) {}

	// This prevents ChainPack(some_pointer) from accidentally producing a bool. Use
	// ChainPack(bool(some_pointer)) if that behavior is desired.
	Value(void *) = delete;

	Type::Enum type() const;

	const MetaData &metaData() const;
	void setMetaData(MetaData &&meta_data);
	void setMetaValue(UInt key, const Value &val);

	bool isValid() const;
	bool isNull() const { return type() == Type::Null; }
	bool isDouble() const { return type() == Type::Double; }
	bool isBool() const { return type() == Type::Bool; }
	bool isString() const { return type() == Type::String; }
	bool isList() const { return type() == Type::List; }
	bool isMap() const { return type() == Type::Map; }
	bool isIMap() const { return type() == Type::IMap; }

	double toDouble() const;
	Int toInt() const;
	UInt toUInt() const;
	bool toBool() const;
	DateTime toDateTime() const;
	const Value::String &toString() const;
	const Blob &toBlob() const;
	const List &toList() const;
	const Map &toMap() const;
	const IMap &toIMap() const;

	int count() const;
	const Value & at(UInt i) const;
	const Value & at(const Value::String &key) const;
	const Value & operator[](UInt i) const {return at(i);}
	const Value & operator[](const Value::String &key) const {return at(key);}
	void set(UInt ix, const Value &val);
	void set(const Value::String &key, const Value &val);

	void dumpText(std::string &out) const;
	void dumpJson(std::string &out) const;

	std::string dumpText() const { std::string out; dumpText(out); return out; }
	std::string dumpJson() const { std::string out; dumpJson(out); return out; }

	static Value parseJson(const std::string & in, std::string & err);
	static Value parseJson(const char * in, std::string & err);

	bool operator== (const Value &rhs) const;
	/*
	bool operator<  (const ChainPack &rhs) const;
	bool operator!= (const ChainPack &rhs) const { return !(*this == rhs); }
	bool operator<= (const ChainPack &rhs) const { return !(rhs < *this); }
	bool operator>  (const ChainPack &rhs) const { return  (rhs < *this); }
	bool operator>= (const ChainPack &rhs) const { return !(*this < rhs); }
	*/
private:
	std::shared_ptr<AbstractValueData> m_ptr;
};

}}
