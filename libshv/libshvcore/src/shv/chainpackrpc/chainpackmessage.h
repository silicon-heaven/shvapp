#pragma once

#include "../../shvcoreglobal.h"

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <initializer_list>

namespace shv {
namespace chainpackrpc {


class SHVCORE_DECL_EXPORT Message final
{
public:
	class AbstractValueData;
	// Types
	struct Type {
		enum Enum { Invalid=-1,
					TERM = 0,
					FALSE,
					TRUE,
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
					MetaTypeId,
					MetaTypeNameSpaceId,
					MetaTypeName,
					MetaTypeNameSpaceName,
					COUNT
				  };
		static const char* name(Enum e);
	};

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
	using List = std::vector<Message>;
	using Map = std::map<Message::String, Message>;
	class Table : public List
	{
	public:
		Table() : List() {}
		Table(const Table &t) : List(t) {}
		Table(Table &&t) : List(std::move(t)) {}
		Table(std::initializer_list<value_type> l) : List(l) {}
	};
	struct MetaTypeId { unsigned int id = 0; MetaTypeId(unsigned int id) : id(id) {}};
	struct MetaTypeNameSpaceId { unsigned int id = 0; MetaTypeNameSpaceId(unsigned int id) : id(id) {}};
	struct MetaTypeName : public String { MetaTypeName(const String &id) : String(id) {} };
	struct MetaTypeNameSpaceName : public String { MetaTypeNameSpaceName(const String &id) : String(id) {} };

	// Constructors for the various types of JSON value.
	Message() noexcept;                // Null
	Message(std::nullptr_t) noexcept;  // Null
	Message(double value);             // Double
	Message(signed int value);                // Int
	Message(unsigned int value);                // Int
	Message(bool value);               // Bool
	Message(const DateTime &value);
	Message(const Blob &value); // Blob
	Message(Blob &&value);
	Message(const uint8_t *value, size_t size);
	Message(const std::string &value); // String
	Message(std::string &&value);      // String
	Message(const char * value);       // String
	Message(const List &values);      // List
	Message(List &&values);           // List
	Message(const Table &values);
	Message(Table &&values);
	Message(const Map &values);     // Map
	Message(Map &&values);          // Map

	Message(const MetaTypeId &value);
	Message(const MetaTypeNameSpaceId &value);
	Message(const MetaTypeName &value);
	Message(const MetaTypeNameSpaceName &value);

	//ChainPack fromType(Type::Enum t);
	Message(const std::shared_ptr<AbstractValueData> &r);

	// Implicit constructor: anything with a to_json() function.
	template <class T, class = decltype(&T::to_json)>
	Message(const T & t) : Message(t.to_json()) {}

	// Implicit constructor: map-like objects (std::map, std::unordered_map, etc)
	template <class M, typename std::enable_if<
				  std::is_constructible<std::string, typename M::key_type>::value
				  && std::is_constructible<Message, typename M::mapped_type>::value,
				  int>::type = 0>
	Message(const M & m) : Message(Map(m.begin(), m.end())) {}

	// Implicit constructor: vector-like objects (std::list, std::vector, std::set, etc)
	template <class V, typename std::enable_if<
				  std::is_constructible<Message, typename V::value_type>::value,
				  int>::type = 0>
	Message(const V & v) : Message(List(v.begin(), v.end())) {}

	// This prevents ChainPack(some_pointer) from accidentally producing a bool. Use
	// ChainPack(bool(some_pointer)) if that behavior is desired.
	Message(void *) = delete;

	// Accessors
	Type::Enum type() const;

	Message meta() const;
	void setMeta(const Message &m);

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
	int toInt() const;
	unsigned int toUInt() const;
	// Return the enclosed value if this is a boolean, false otherwise.
	bool toBool() const;
	DateTime toDateTime() const;
	// Return the enclosed string if this is a string, "" otherwise.
	const Message::String &toString() const;
	const Blob &toBlob() const;
	// Return the enclosed std::vector if this is an List, or an empty vector otherwise.
	const List &toList() const;
	// Return the enclosed std::map if this is an Map, or an empty map otherwise.
	const Map &toMap() const;

	int count() const;
	const Message & operator[](size_t i) const;
	const Message & operator[](const Message::String &key) const;

	// Serialize.
	void dumpText(std::string &out) const;
	void dumpJson(std::string &out) const;

	std::string dumpText() const { std::string out; dumpText(out); return out; }
	std::string dumpJson() const { std::string out; dumpJson(out); return out; }

	// Parse. If parse fails, return ChainPack() and assign an error message to err.
	static Message parseJson(const std::string & in, std::string & err);
	static Message parseJson(const char * in, std::string & err);
	// Parse multiple objects, concatenated or separated by whitespace
	/*
	static std::vector<ChainPack> parse_multi(
			const std::string & in,
			std::string::size_type & parser_stop_pos,
			std::string & err,
			ChainPackParse strategy = ChainPackParse::STANDARD);

	static inline std::vector<ChainPack> parse_multi(
			const std::string & in,
			std::string & err,
			ChainPackParse strategy = ChainPackParse::STANDARD) {
		std::string::size_type parser_stop_pos;
		return parse_multi(in, parser_stop_pos, err, strategy);
	}
	*/
	bool operator== (const Message &rhs) const;
	/*
	bool operator<  (const ChainPack &rhs) const;
	bool operator!= (const ChainPack &rhs) const { return !(*this == rhs); }
	bool operator<= (const ChainPack &rhs) const { return !(rhs < *this); }
	bool operator>  (const ChainPack &rhs) const { return  (rhs < *this); }
	bool operator>= (const ChainPack &rhs) const { return !(*this < rhs); }
	*/
	/* has_shape(types, err)
	 *
	 * Return true if this is a JSON object and, for each item in types, has a field of
	 * the given type. If not, return false and set err to a descriptive message.
	 */
	//typedef std::initializer_list<std::pair<std::string, Type::Enum>> shape;
	//bool has_shape(const shape & types, std::string & err) const;
private:
	std::shared_ptr<AbstractValueData> m_ptr;
};

}}
