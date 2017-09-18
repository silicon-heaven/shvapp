#pragma once

#include "../shvcoreglobal.h"
#include "metatypes.h"

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <initializer_list>

#ifndef CHAINPACK_UINT
	#define CHAINPACK_UINT uint64_t
#endif
#ifndef CHAINPACK_INT
	#define CHAINPACK_INT int64_t
#endif

namespace shv {
namespace core {
namespace chainpack {


class SHVCORE_DECL_EXPORT RpcValue final
{
public:
	class AbstractValueData;

	enum class Type {
		Invalid,
		Null,
		UInt,
		Int,
		Double,
		Bool,
		Blob,
		String,
		DateTime,
		List,
		Array,
		Map,
		IMap,
		MetaIMap,
	};
	static const char* typeToName(Type t);

	struct SHVCORE_DECL_EXPORT Tag
	{
		enum Enum : CHAINPACK_UINT {
			Invalid = 0,
			MetaTypeId,
			MetaTypeNameSpaceId,
			MetaIMap,
			MetaTypeName,
			MetaTypeNameSpaceName,
			USER = 8
		};
		//static const char* name(Enum e);
	};

	using Int = CHAINPACK_INT;
	using UInt = CHAINPACK_UINT;
	struct SHVCORE_DECL_EXPORT DateTime
	{
		int64_t msecs = 0;

		DateTime() {}

		static DateTime fromString(const std::string &local_date_time_str);
		static DateTime fromUtcString(const std::string &utc_date_time_str);
		static DateTime fromMSecsSinceEpoch(int64_t msecs);
		std::string toString() const;
		std::string toUtcString() const;
	};
	using String = std::string;
	struct SHVCORE_DECL_EXPORT Blob : public std::basic_string<uint8_t>
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
	using List = std::vector<RpcValue>;
	using Map = std::map<RpcValue::String, RpcValue>;
	using IMap = std::map<RpcValue::UInt, RpcValue>;
	class SHVCORE_DECL_EXPORT Array : public List
	{
	public:
		Array(Type type) : List(), m_type(type) {}
		Array(const Array &t) : List(t), m_type(t.type()) {}
		Array(Array &&t) noexcept : List(std::move(t)), m_type(t.type()) {}
		Array(Type type, const List &l) : List(l), m_type(type) {}
		Array(Type type, List &&l) noexcept : List(std::move(l)), m_type(type) {}
		Array(Type type, std::initializer_list<value_type> l) : List(l), m_type(type) {}

		Type type() const {return m_type;}
	private:
		Type m_type = Type::Invalid;
	};
	struct SHVCORE_DECL_EXPORT MetaData
	{
		RpcValue::UInt metaTypeId() const {return value(RpcValue::Tag::MetaTypeId).toUInt();}
		void setMetaTypeId(RpcValue::UInt id) {setValue(RpcValue::Tag::MetaTypeId, id);}
		RpcValue::UInt metaTypeNameSpaceId() const {return value(RpcValue::Tag::MetaTypeNameSpaceId).toUInt();}
		void setMetaTypeNameSpaceId(RpcValue::UInt id) {setValue(RpcValue::Tag::MetaTypeNameSpaceId, id);}
		std::vector<RpcValue::UInt> ikeys() const;
		RpcValue value(RpcValue::UInt key) const;
		void setValue(RpcValue::UInt key, const RpcValue &val);
		bool isEmpty() const {return m_imap.empty();}
		bool operator==(const MetaData &o) const;
		const RpcValue::IMap& toIMap() const {return m_imap;}
		std::string toStdString() const;
	protected:
		//Value::Map smap;
		RpcValue::IMap m_imap;
		//RpcValue::UInt m_metaTypeId = 0;
		//RpcValue::UInt m_metaTypeNameSpaceId = 0;
	};
	//struct MetaTypeId { uint32_t id = 0; MetaTypeId(uint32_t id) : id(id) {}};
	//struct MetaTypeNameSpaceId { uint32_t id = 0; MetaTypeNameSpaceId(uint32_t id) : id(id) {}};
	//struct MetaTypeName : public String { MetaTypeName(const String &id) : String(id) {} };
	//struct MetaTypeNameSpaceName : public String { MetaTypeNameSpaceName(const String &id) : String(id) {} };

	// Constructors for the various types of JSON value.
	RpcValue() noexcept;                // Null
	RpcValue(std::nullptr_t) noexcept;  // Null
	RpcValue(double value);             // Double
	RpcValue(Int value);                // Int
	RpcValue(UInt value);                // UInt
	RpcValue(int value) : RpcValue((Int)value) {}
	RpcValue(unsigned int value) : RpcValue((UInt)value) {}
	RpcValue(bool value);               // Bool
	RpcValue(const DateTime &value);
	RpcValue(const Blob &value); // Blob
	RpcValue(Blob &&value);
	RpcValue(const uint8_t *value, size_t size);
	RpcValue(const std::string &value); // String
	RpcValue(std::string &&value);      // String
	RpcValue(const char *value);       // String
	RpcValue(const List &values);      // List
	RpcValue(List &&values);           // List
	RpcValue(const Array &values);
	RpcValue(Array &&values);
	RpcValue(const Map &values);     // Map
	RpcValue(Map &&values);          // Map
	RpcValue(const IMap &values);     // IMap
	RpcValue(IMap &&values);          // IMap

	//Value(const MetaTypeId &value);
	//Value(const MetaTypeNameSpaceId &value);
	//Value(const MetaTypeName &value);
	//Value(const MetaTypeNameSpaceName &value);

	//ChainPack fromType(Type::Enum t);
	//Value(const std::shared_ptr<AbstractValueData> &r);

	// Implicit constructor: anything with a to_json() function.
	template <class T, class = decltype(&T::to_json)>
	RpcValue(const T & t) : RpcValue(t.to_json()) {}

	// Implicit constructor: map-like objects (std::map, std::unordered_map, etc)
	template <class M, typename std::enable_if<
				  std::is_constructible<RpcValue::String, typename M::key_type>::value
				  && std::is_constructible<RpcValue, typename M::mapped_type>::value,
				  int>::type = 0>
	RpcValue(const M & m) : RpcValue(Map(m.begin(), m.end())) {}

	// Implicit constructor: vector-like objects (std::list, std::vector, std::set, etc)
	template <class V, typename std::enable_if<
				  std::is_constructible<RpcValue, typename V::value_type>::value,
				  int>::type = 0>
	RpcValue(const V & v) : RpcValue(List(v.begin(), v.end())) {}

	// This prevents ChainPack(some_pointer) from accidentally producing a bool. Use
	// ChainPack(bool(some_pointer)) if that behavior is desired.
	RpcValue(void *) = delete;

	Type type() const;
	Type arrayType() const;

	const MetaData &metaData() const;
	void setMetaData(MetaData &&meta_data);
	void setMetaValue(UInt key, const RpcValue &val);

	bool isValid() const;
	bool isNull() const { return type() == Type::Null; }
	bool isInt() const { return type() == Type::Int; }
	bool isDateTime() const { return type() == Type::DateTime; }
	bool isUInt() const { return type() == Type::UInt; }
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
	const RpcValue::String &toString() const;
	const Blob &toBlob() const;
	const List &toList() const;
	const Map &toMap() const;
	const IMap &toIMap() const;

	size_t count() const;
	const RpcValue & at(UInt i) const;
	const RpcValue & at(const RpcValue::String &key) const;
	const RpcValue & operator[](UInt i) const {return at(i);}
	const RpcValue & operator[](const RpcValue::String &key) const {return at(key);}
	void set(UInt ix, const RpcValue &val);
	void set(const RpcValue::String &key, const RpcValue &val);

	void dumpText(std::string &out) const;
	void dumpJson(std::string &out) const;

	std::string toStdString() const { std::string out; dumpText(out); return out; }
	std::string toJson() const { std::string out; dumpJson(out); return out; }

	static RpcValue parseJson(const std::string & in, std::string & err);
	static RpcValue parseJson(const char * in, std::string & err);

	bool operator== (const RpcValue &rhs) const;
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

}}}
