/*
 * Define CHAINPACK_TEST_CUSTOM_CONFIG to 1 if you want to build this tester into
 * your own unit-test framework rather than a stand-alone program.  By setting
 * The values of the variables included below, you can insert your own custom
 * code into this file as it builds, in order to make it into a test case for
 * your favorite framework.
 */
#if !CHAINPACK_TEST_CUSTOM_CONFIG
#define CHAINPACK_TEST_CPP_PREFIX_CODE
#define CHAINPACK_TEST_CPP_SUFFIX_CODE
#define CHAINPACK_TEST_STANDALONE_MAIN 1
#define CHAINPACK_TEST_CASE(name) static void name()
#define CHAINPACK_TEST_ASSERT(b) assert(b)
#ifdef NDEBUG
#undef NDEBUG//at now assert will work even in Release build
#endif
#endif // CHAINPACK_TEST_CUSTOM_CONFIG

/*
 * Enable or disable code which demonstrates the behavior change in Xcode 7 / Clang 3.7,
 * introduced by DR1467 and described here: https://github.com/dropbox/json11/issues/86
 * Defaults to off since it doesn't appear the standards committee is likely to act
 * on this, so it needs to be considered normal behavior.
 */
#ifndef CHAINPACK_ENABLE_DR1467_CANARY
#define CHAINPACK_ENABLE_DR1467_CANARY 0
#endif

/*
 * Beginning of standard source file, which makes use of the customizations above.
 */
#include <shv/chainpackrpc/value.h>
#include <shv/chainpackrpc/chainpackprotocol.h>

#include <cassert>
#include <string>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <list>
#include <set>
#include <unordered_map>
#include <algorithm>
#include <type_traits>

// Insert user-defined prefix code (includes, function declarations, etc)
// to set up a custom test suite
CHAINPACK_TEST_CPP_PREFIX_CODE

using namespace shv::chainpackrpc;
using std::string;

// Check that ChainPack has the properties we want.
#define CHECK_TRAIT(x) static_assert(std::x::value, #x)
CHECK_TRAIT(is_nothrow_constructible<Value>);
CHECK_TRAIT(is_nothrow_default_constructible<Value>);
CHECK_TRAIT(is_copy_constructible<Value>);
CHECK_TRAIT(is_nothrow_move_constructible<Value>);
CHECK_TRAIT(is_copy_assignable<Value>);
CHECK_TRAIT(is_nothrow_move_assignable<Value>);
CHECK_TRAIT(is_nothrow_destructible<Value>);

CHAINPACK_TEST_CASE(text_test) {
	std::cout << "============= chainpack text test ============\n";
	const string simple_test =
			R"({"k1":"v1", "k2":42, "k3":["a",123,true,false,null]})";

	string err;
	const auto json = Value::parseJson(simple_test, err);

	std::cout << "k1: " << json["k1"].toString() << "\n";
	std::cout << "k3: " << json["k3"].dumpJson() << "\n";

	Value cp = json;
	//cp.setMeta(json["k1"]);
	std::cout << "cp: " << cp.dumpText() << "\n";

	//for (auto &k : json["k3"].toList()) {
	//	std::cout << "    - " << k.dumpJson() << "\n";
	//}

	string comment_test = R"({
						  // comment /* with nested comment */
						  "a": 1,
						  // comment
						  // continued
						  "b": "text",
						  /* multi
						  line
						  comment
						  // line-comment-inside-multiline-comment
						  */
						  // and single-line comment
						  // and single-line comment /* multiline inside single line */
						  "c": [1, 2, 3]
						  // and single-line comment at end of object
						  })";

	string err_comment;
	auto json_comment = Value::parseJson( comment_test, err_comment);
	CHAINPACK_TEST_ASSERT(!json_comment.isNull());
	CHAINPACK_TEST_ASSERT(err_comment.empty());

	comment_test = "{\"a\": 1}//trailing line comment";
	json_comment = Value::parseJson( comment_test, err_comment);
	CHAINPACK_TEST_ASSERT(!json_comment.isNull());
	CHAINPACK_TEST_ASSERT(err_comment.empty());

	comment_test = "{\"a\": 1}/*trailing multi-line comment*/";
	json_comment = Value::parseJson( comment_test, err_comment);
	CHAINPACK_TEST_ASSERT(!json_comment.isNull());
	CHAINPACK_TEST_ASSERT(err_comment.empty());

	string failing_comment_test = "{\n/* unterminated comment\n\"a\": 1,\n}";
	string err_failing_comment;
	Value json_failing_comment = Value::parseJson( failing_comment_test, err_failing_comment);
	CHAINPACK_TEST_ASSERT(!json_failing_comment.isValid());
	CHAINPACK_TEST_ASSERT(!err_failing_comment.empty());

	failing_comment_test = "{\n/* unterminated trailing comment }";
	json_failing_comment = Value::parseJson( failing_comment_test, err_failing_comment);
	CHAINPACK_TEST_ASSERT(!json_failing_comment.isValid());
	CHAINPACK_TEST_ASSERT(!err_failing_comment.empty());

	failing_comment_test = "{\n/ / bad comment }";
	json_failing_comment = Value::parseJson( failing_comment_test, err_failing_comment);
	CHAINPACK_TEST_ASSERT(!json_failing_comment.isValid());
	CHAINPACK_TEST_ASSERT(!err_failing_comment.empty());

	failing_comment_test = "{// bad comment }";
	json_failing_comment = Value::parseJson( failing_comment_test, err_failing_comment);
	CHAINPACK_TEST_ASSERT(!json_failing_comment.isValid());
	CHAINPACK_TEST_ASSERT(!err_failing_comment.empty());

	failing_comment_test = "{\n\"a\": 1\n}/";
	json_failing_comment = Value::parseJson( failing_comment_test, err_failing_comment);
	CHAINPACK_TEST_ASSERT(!json_failing_comment.isValid());
	CHAINPACK_TEST_ASSERT(!err_failing_comment.empty());

	failing_comment_test = "{/* bad\ncomment *}";
	json_failing_comment = Value::parseJson( failing_comment_test, err_failing_comment);
	CHAINPACK_TEST_ASSERT(!json_failing_comment.isValid());
	CHAINPACK_TEST_ASSERT(!err_failing_comment.empty());
	/*
	std::list<int> l1 { 1, 2, 3 };
	std::vector<int> l2 { 1, 2, 3 };
	std::set<int> l3 { 1, 2, 3 };
	CHAINPACK_TEST_ASSERT(ChainPack(l1) == ChainPack(l2));
	CHAINPACK_TEST_ASSERT(ChainPack(l2) == ChainPack(l3));

	std::map<string, string> m1 { { "k1", "v1" }, { "k2", "v2" } };
	std::unordered_map<string, string> m2 { { "k1", "v1" }, { "k2", "v2" } };
	CHAINPACK_TEST_ASSERT(ChainPack(m1) == ChainPack(m2));
	*/
	// ChainPack literals
	const Value obj = Value::Map({
											 { "k1", "v1" },
											 { "k2", 42.0 },
											 { "k3", Value::List({ "a", 123.0, true, false, nullptr }) },
										 });

	std::cout << "obj: " << obj.dumpJson() << "\n";
	CHAINPACK_TEST_ASSERT(obj.dumpJson() == "{\"k1\": \"v1\", \"k2\": 42, \"k3\": [\"a\", 123, true, false, null]}");

	CHAINPACK_TEST_ASSERT(Value("a").toDouble() == 0);
	CHAINPACK_TEST_ASSERT(Value("a").toString() == "a");
	CHAINPACK_TEST_ASSERT(Value().toDouble() == 0);
	/*
	CHAINPACK_TEST_ASSERT(obj == json);
	CHAINPACK_TEST_ASSERT(ChainPack(42) == ChainPack(42.0));
	CHAINPACK_TEST_ASSERT(ChainPack(42) != ChainPack(42.1));
	*/
	const string unicode_escape_test =
			R"([ "blah\ud83d\udca9blah\ud83dblah\udca9blah\u0000blah\u1234" ])";

	const char utf8[] = "blah" "\xf0\x9f\x92\xa9" "blah" "\xed\xa0\xbd" "blah"
						"\xed\xb2\xa9" "blah" "\0" "blah" "\xe1\x88\xb4";

	Value uni = Value::parseJson(unicode_escape_test, err);
	CHAINPACK_TEST_ASSERT(uni[0].toString().size() == (sizeof utf8) - 1);
	CHAINPACK_TEST_ASSERT(std::memcmp(uni[0].toString().data(), utf8, sizeof utf8) == 0);

	// Demonstrates the behavior change in Xcode 7 / Clang 3.7, introduced by DR1467
	// and described here: https://llvm.org/bugs/show_bug.cgi?id=23812
	/*
	if (CHAINPACK_ENABLE_DR1467_CANARY) {
		ChainPack nested_array = ChainPack::List { ChainPack::List { 1, 2, 3 } };
		CHAINPACK_TEST_ASSERT(nested_array.isList());
		CHAINPACK_TEST_ASSERT(nested_array.toList().size() == 1);
		CHAINPACK_TEST_ASSERT(nested_array.toList()[0].isList());
		CHAINPACK_TEST_ASSERT(nested_array.toList()[0].toList().size() == 3);
	}
	*/
	/*
	{
		const std::string good_json = R"( {"k1" : "v1"})";
		const std::string bad_json1 = good_json + " {";
		const std::string bad_json2 = good_json + R"({"k2":"v2", "k3":[)";
		struct TestMultiParse {
			std::string input;
			std::string::size_type expect_parser_stop_pos;
			size_t expect_not_empty_elms_count;
			ChainPack expect_parse_res;
		} tests[] = {
			{" {", 0, 0, {}},
			{good_json, good_json.size(), 1, ChainPack(std::map<string, string>{ { "k1", "v1" } })},
			{bad_json1, good_json.size() + 1, 1, ChainPack(std::map<string, string>{ { "k1", "v1" } })},
			{bad_json2, good_json.size(), 1, ChainPack(std::map<string, string>{ { "k1", "v1" } })},
			{"{}", 2, 1, ChainPack::Map{}},
		};
		for (const auto &tst : tests) {
			std::string::size_type parser_stop_pos;
			std::string err;
			auto res = ChainPack::parse_multi(tst.input, parser_stop_pos, err);
			CHAINPACK_TEST_ASSERT(parser_stop_pos == tst.expect_parser_stop_pos);
			CHAINPACK_TEST_ASSERT(
						(size_t)std::count_if(res.begin(), res.end(),
											  [](const ChainPack& j) { return !j.isNull(); })
						== tst.expect_not_empty_elms_count);
			if (!res.empty()) {
				CHAINPACK_TEST_ASSERT(tst.expect_parse_res == res[0]);
			}
		}
	}
	*/
	Value my_json = Value::Map {
		{ "key1", "value1" },
		{ "key2", false },
		{ "key3", Value::List { 1, 2, 3 } },
	};
	std::string json_obj_str = my_json.dumpJson();
	std::cout << "json_obj_str: " << json_obj_str << "\n";
	CHAINPACK_TEST_ASSERT(json_obj_str == "{\"key1\": \"value1\", \"key2\": false, \"key3\": [1, 2, 3]}");

	class Point {
	public:
		int x;
		int y;
		Point (int x, int y) : x(x), y(y) {}
		Value to_json() const { return Value::List { x, y }; }
	};

	std::vector<Point> points = { { 1, 2 }, { 10, 20 }, { 100, 200 } };
	std::string points_json = Value(points).dumpJson();
	std::cout << "points_json: " << points_json << "\n";
	CHAINPACK_TEST_ASSERT(points_json == "[[1, 2], [10, 20], [100, 200]]");
}

static std::string binary_dump(const Value::Blob &out)
{
	std::string ret;
	for (size_t i = 0; i < out.size(); ++i) {
		uint8_t u = out[i];
		//ret += std::to_string(u);
		if(i > 0)
			ret += '|';
		for (size_t j = 0; j < 8*sizeof(u); ++j) {
			ret += (u & (((uint8_t)128) >> j))? '1': '0';
		}
	}
	return ret;
}

CHAINPACK_TEST_CASE(binary_test)
{
	std::cout << "============= chainpack binary test ============\n";
	for (int i = 0; i <= shv::chainpackrpc::Value::Type::TRUE; ++i) {
		Value::Blob out;
		out += i;
		Value::Type::Enum e = (Value::Type::Enum)i;
		//std::string s = std::to_string(i);
		//if(i < 10)
		//	s = ' ' + s;
		std::cout << std::setw(2) << i << " " << binary_dump(out) << " "  << Value::Type::name(e) << "\n";
	}
	{
		std::cout << "------------- NULL \n";
		Value cp1{nullptr};
		ChainPackProtocol::Blob out;
		int len = ChainPackProtocol::write(out, cp1);
		CHAINPACK_TEST_ASSERT(len == 1);
		Value cp2 = ChainPackProtocol::read(out);
		std::cout << cp1.dumpText() << " " << cp2.dumpText() << " len: " << len << " dump: " << binary_dump(out) << "\n";
		CHAINPACK_TEST_ASSERT(cp1.type() == cp2.type());
	}
	std::cout << "------------- tiny uint \n";
	for (unsigned int n = 0; n < 64; ++n) {
		Value cp1{n};
		ChainPackProtocol::Blob out;
		int len = ChainPackProtocol::write(out, cp1);
		if(n < 10)
			std::cout << n << " " << cp1.dumpText() << " len: " << len << " dump: " << binary_dump(out) << "\n";
		CHAINPACK_TEST_ASSERT(len == 1);
		Value cp2 = ChainPackProtocol::read(out);
		CHAINPACK_TEST_ASSERT(cp1.type() == cp2.type());
		CHAINPACK_TEST_ASSERT(cp1.toUInt() == cp2.toUInt());
	}
	{
		std::cout << "------------- uint \n";
		auto n_max = std::numeric_limits<unsigned int>::max();
		auto step = n_max / 1000;
		for (unsigned int n = 64; n < (n_max - step); n += step) {
			Value cp1{n};
			ChainPackProtocol::Blob out;
			int len = ChainPackProtocol::write(out, cp1);
			CHAINPACK_TEST_ASSERT(len > 1);
			Value cp2 = ChainPackProtocol::read(out);
			if(n < 66)
				std::cout << n << " " << cp1.dumpText() << " " << cp2.dumpText() << " len: " << len << " dump: " << binary_dump(out) << "\n";
			CHAINPACK_TEST_ASSERT(cp1.type() == cp2.type());
			CHAINPACK_TEST_ASSERT(cp1.toUInt() == cp2.toUInt());
		}
	}
	{
		std::cout << "------------- int \n";
		{
			for (int n = 0; n < 10; n++) {
				Value cp1{n};
				ChainPackProtocol::Blob out;
				int len = ChainPackProtocol::write(out, cp1);
				CHAINPACK_TEST_ASSERT(len > 1);
				Value cp2 = ChainPackProtocol::read(out);
				std::cout << n << " " << cp1.dumpText() << " " << cp2.dumpText() << " len: " << len << " dump: " << binary_dump(out) << "\n";
				CHAINPACK_TEST_ASSERT(cp1.type() == cp2.type());
				CHAINPACK_TEST_ASSERT(cp1.toInt() == cp2.toInt());
			}
		}
		{
			auto n_max = std::numeric_limits<signed int>::max();
			auto n_min = std::numeric_limits<signed int>::min()+1;
			auto step = n_max / 1000;
			for (int n = n_min; n < (n_max - step); n += step) {
				Value cp1{n};
				ChainPackProtocol::Blob out;
				int len = ChainPackProtocol::write(out, cp1);
				CHAINPACK_TEST_ASSERT(len > 1);
				Value cp2 = ChainPackProtocol::read(out);
				CHAINPACK_TEST_ASSERT(cp1.type() == cp2.type());
				CHAINPACK_TEST_ASSERT(cp1.toInt() == cp2.toInt());
			}
		}
	}
	{
		std::cout << "------------- double \n";
		{
			auto n_max = 1000000.;
			auto n_min = -1000000.;
			auto step = (n_max - n_min) / 100;
			for (auto n = n_min; n < n_max; n += step) {
				Value cp1{n};
				ChainPackProtocol::Blob out;
				int len = ChainPackProtocol::write(out, cp1);
				CHAINPACK_TEST_ASSERT(len > 1);
				Value cp2 = ChainPackProtocol::read(out);
				//std::cout << n << " " << cp1.dumpText() << " " << cp2.dumpText() << " len: " << len << " dump: " << binary_dump(out) << "\n";
				CHAINPACK_TEST_ASSERT(cp1.type() == cp2.type());
				CHAINPACK_TEST_ASSERT(cp1.toDouble() == cp2.toDouble());
			}
		}
		{
			auto n_max = std::numeric_limits<double>::max();
			auto n_min = std::numeric_limits<double>::min();
			auto step = (n_max - n_min) / 100;
			for (auto n = n_min; n < n_max; n += step) {
				Value cp1{n};
				ChainPackProtocol::Blob out;
				int len = ChainPackProtocol::write(out, cp1);
				CHAINPACK_TEST_ASSERT(len > 1);
				Value cp2 = ChainPackProtocol::read(out);
				//std::cout << n << " " << cp1.dumpText() << " " << cp2.dumpText() << " len: " << len << " dump: " << binary_dump(out) << "\n";
				CHAINPACK_TEST_ASSERT(cp1.type() == cp2.type());
				CHAINPACK_TEST_ASSERT(cp1.toDouble() == cp2.toDouble());
			}
		}
	}
	{
		std::cout << "------------- bool \n";
		for(bool b : {false, true}) {
			Value cp1{b};
			ChainPackProtocol::Blob out;
			int len = ChainPackProtocol::write(out, cp1);
			CHAINPACK_TEST_ASSERT(len == 1);
			Value cp2 = ChainPackProtocol::read(out);
			std::cout << b << " " << cp1.dumpText() << " " << cp2.dumpText() << " len: " << len << " dump: " << binary_dump(out) << "\n";
			CHAINPACK_TEST_ASSERT(cp1.type() == cp2.type());
			CHAINPACK_TEST_ASSERT(cp1.toBool() == cp2.toBool());
		}
	}
	{
		std::cout << "------------- blob \n";
		Value::Blob blob{"fpowfksapofkpsaokfsa"};
		blob[5] = 0;
		Value cp1{blob};
		ChainPackProtocol::Blob out;
		ChainPackProtocol::write(out, cp1);
		Value cp2 = ChainPackProtocol::read(out);
		//std::cout << blob.toString() << " " << cp1.dumpText() << " " << cp2.dumpText() << " len: " << len << " dump: " << binary_dump(out) << "\n";
		CHAINPACK_TEST_ASSERT(cp1.type() == cp2.type());
		CHAINPACK_TEST_ASSERT(cp1.toBlob() == cp2.toBlob());
	}
	{
		std::cout << "------------- string \n";
		Value::String str{"lhklhklfkjdslfkposkfp79"};
		str[5] = 0;
		Value cp1{str};
		ChainPackProtocol::Blob out;
		ChainPackProtocol::write(out, cp1);
		Value cp2 = ChainPackProtocol::read(out);
		//std::cout << str << " " << cp1.dumpText() << " " << cp2.dumpText() << " len: " << len << " dump: " << binary_dump(out) << "\n";
		CHAINPACK_TEST_ASSERT(cp1.type() == cp2.type());
		CHAINPACK_TEST_ASSERT(cp1.toString() == cp2.toString());
	}
	{
		std::cout << "------------- DateTime \n";
		std::string str = "2017-05-03T15:52:31.123";
		Value::DateTime dt = Value::DateTime::fromString(str);
		Value cp1{dt};
		ChainPackProtocol::Blob out;
		int len = ChainPackProtocol::write(out, cp1);
		Value cp2 = ChainPackProtocol::read(out);
		std::cout << str << " " << dt.toUtcString() << " " << cp1.dumpText() << " " << cp2.dumpText() << " len: " << len << " dump: " << binary_dump(out) << "\n";
		CHAINPACK_TEST_ASSERT(cp1.type() == cp2.type());
		CHAINPACK_TEST_ASSERT(cp1.toInt() == cp2.toInt());
	}
	{
		std::cout << "------------- List \n";
		{
			const std::string s = R"(["a",123,true,[1,2,3],null])";
			string err;
			Value cp1 = Value::parseJson(s, err);
			ChainPackProtocol::Blob out;
			int len = ChainPackProtocol::write(out, cp1);
			Value cp2 = ChainPackProtocol::read(out);
			std::cout << s << " " << cp1.dumpText() << " " << cp2.dumpText() << " len: " << len << " dump: " << binary_dump(out) << "\n";
			CHAINPACK_TEST_ASSERT(cp1.type() == cp2.type());
			CHAINPACK_TEST_ASSERT(cp1.toList() == cp2.toList());
		}
		{
			Value cp1{Value::List{1,2,3}};
			ChainPackProtocol::Blob out;
			int len = ChainPackProtocol::write(out, cp1);
			Value cp2 = ChainPackProtocol::read(out);
			std::cout << cp1.dumpText() << " " << cp2.dumpText() << " len: " << len << " dump: " << binary_dump(out) << "\n";
			CHAINPACK_TEST_ASSERT(cp1.type() == cp2.type());
			CHAINPACK_TEST_ASSERT(cp1.toList() == cp2.toList());
		}
	}
	{
		std::cout << "------------- Table \n";
		{
			Value cp1{Value::Table{(unsigned)1, (unsigned)2, (unsigned)3}};
			ChainPackProtocol::Blob out;
			int len = ChainPackProtocol::write(out, cp1);
			Value cp2 = ChainPackProtocol::read(out);
			std::cout << cp1.dumpText() << " " << cp2.dumpText() << " len: " << len << " dump: " << binary_dump(out) << "\n";
			CHAINPACK_TEST_ASSERT(cp1.type() == cp2.type());
			CHAINPACK_TEST_ASSERT(cp1.toList() == cp2.toList());
		}
		{
			Value::Table t;
			t.push_back(Value::List{Value{1}, Value{2}});
			t.push_back(Value::List{Value{3}, Value{4}});
			t.push_back(Value::List{Value{5}, Value{6}});
			t.push_back(Value::List{Value{7}, Value{8}});
			Value cp1{t};
			ChainPackProtocol::Blob out;
			int len = ChainPackProtocol::write(out, cp1);
			Value cp2 = ChainPackProtocol::read(out);
			std::cout << cp1.dumpText() << " " << cp2.dumpText() << " len: " << len << " dump: " << binary_dump(out) << "\n";
			CHAINPACK_TEST_ASSERT(cp1.type() == cp2.type());
			CHAINPACK_TEST_ASSERT(cp1.toList() == cp2.toList());
		}
	}
	{
		std::cout << "------------- Map \n";
		{
			Value cp1{{
				{"foo", 1},
				{"bar", 2},
				{"baz", 3},
			}};
			ChainPackProtocol::Blob out;
			int len = ChainPackProtocol::write(out, cp1);
			Value cp2 = ChainPackProtocol::read(out);
			std::cout << cp1.dumpText() << " " << cp2.dumpText() << " len: " << len << " dump: " << binary_dump(out) << "\n";
			CHAINPACK_TEST_ASSERT(cp1.type() == cp2.type());
			CHAINPACK_TEST_ASSERT(cp1.toMap() == cp2.toMap());
		}
		{
			Value cp1{{
				{"foo", Value::List{11,12,13}},
				{"bar", 2},
				{"baz", 3},
			}};
			ChainPackProtocol::Blob out;
			int len = ChainPackProtocol::write(out, cp1);
			Value cp2 = ChainPackProtocol::read(out);
			std::cout << cp1.dumpText() << " " << cp2.dumpText() << " len: " << len << " dump: " << binary_dump(out) << "\n";
			CHAINPACK_TEST_ASSERT(cp1.type() == cp2.type());
			CHAINPACK_TEST_ASSERT(cp1.toMap() == cp2.toMap());
		}
	}
	/*
	{
		std::cout << "------------- Meta \n";
		Value cp1{1};
		Value meta{Value::MetaTypeId(2)};
		cp1.setMeta(meta);
		ChainPackProtocol::Blob out;
		int len = ChainPackProtocol::write(out, cp1);
		Value cp2 = ChainPackProtocol::read(out);
		std::cout << cp1.dumpText() << " " << cp2.dumpText() << " len: " << len << " dump: " << binary_dump(out) << "\n";
		CHAINPACK_TEST_ASSERT(cp1.type() == cp2.type());
		CHAINPACK_TEST_ASSERT(cp1.toInt() == cp2.toInt());
		CHAINPACK_TEST_ASSERT(cp1.meta().type() == cp2.meta().type());
		CHAINPACK_TEST_ASSERT(cp1.meta().toUInt() == cp2.meta().toUInt());
	}
	*/
}

#if CHAINPACK_TEST_STANDALONE_MAIN

static void parse_from_stdin() {
	string buf;
	string line;
	while (std::getline(std::cin, line)) {
		buf += line + "\n";
	}

	string err;
	auto json = Value::parseJson(buf, err);
	if (!err.empty()) {
		printf("Failed: %s\n", err.c_str());
	} else {
		printf("Result: %s\n", json.dumpJson().c_str());
	}
}

int main(int argc, char **argv) {
	if (argc == 2 && argv[1] == string("--stdin")) {
		parse_from_stdin();
		return 0;
	}

	binary_test();
	try {
		text_test();
		binary_test();
	}
	catch (std::runtime_error &e) {
		std::cout << "ERROR: " << e.what() << "\n";
	}
}

#endif // CHAINPACK_TEST_STANDALONE_MAIN

// Insert user-defined suffix code (function definitions, etc)
// to set up a custom test suite
CHAINPACK_TEST_CPP_SUFFIX_CODE
