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
					MetaTypeId,
					MetaTypeNameSpaceId,
					//MetaMap,
					MetaIMap,
					FALSE,
					TRUE,
					TERM = 128, // first byte of packed Int or UInt cannot be like 0b1000000
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
		Table(Table &&t) : List(std::move(t)) {}
		Table(std::initializer_list<value_type> l) : List(l) {}
	};

	struct SHVCORE_DECL_EXPORT MetaData
	{
		const Value::IMap& imap() const {return m_imap;}
		Value metaValue(Value::UInt key) const;
		void setMetaValue(Value::UInt key, const Value &val);
		//void setMetaValues(Value::IMap &&vals);
		//Value metaValue(const Value::String &key) const;
		//void setMetaValue(const Value::String &key, const Value &val);
		bool isEmpty() const {return m_imap.empty();}
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
				  std::is_constructible<std::string, typename M::key_type>::value
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

	// Accessors
	Type::Enum type() const;

	//Value meta() const;
	//const Map& metaValues() const;
	const MetaData &metaData() const;
	void setMetaData(MetaData &&meta_data);

	bool isValid() const;
	bool isNull() const { return type() == Type::Null; }
	bool isDouble() const { return type() == Type::Double; }
	bool isBool() const { return type() == Type::Bool; }
	bool isString() const { return type() == Type::String; }
	bool isList() const { return type() == Type::List; }
	bool isMap() const { return type() == Type::Map; }

	// Return the enclosed value if this is a number, 0 otherwise. Note that json11 does not
	// distinguish between integer and non-integer numbers - number_value() and int_value()
	// can both be applied to a NUMBER-typed object.
	double toDouble() const;
	Int toInt() const;
	UInt toUInt() const;
	// Return the enclosed value if this is a boolean, false otherwise.
	bool toBool() const;
	DateTime toDateTime() const;
	// Return the enclosed string if this is a string, "" otherwise.
	const Value::String &toString() const;
	const Blob &toBlob() const;
	// Return the enclosed std::vector if this is an List, or an empty vector otherwise.
	const List &toList() const;
	// Return the enclosed std::map if this is an Map, or an empty map otherwise.
	const Map &toMap() const;
	const IMap &toIMap() const;

	int count() const;
	const Value & operator[](size_t i) const;
	const Value & operator[](const Value::String &key) const;

	// Serialize.
	void dumpText(std::string &out) const;
	void dumpJson(std::string &out) const;

	std::string dumpText() const { std::string out; dumpText(out); return out; }
	std::string dumpJson() const { std::string out; dumpJson(out); return out; }

	// Parse. If parse fails, return ChainPack() and assign an error message to err.
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
