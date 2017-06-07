#include <shv/core/chainpack/rpcvalue.h>
#include <shv/core/chainpack/chainpackprotocol.h>

#include <QtTest/QtTest>
#include <QDebug>

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

using namespace shv::core::chainpack;
using std::string;

// Check that ChainPack has the properties we want.
#define CHECK_TRAIT(x) static_assert(std::x::value, #x)
CHECK_TRAIT(is_nothrow_constructible<RpcValue>);
CHECK_TRAIT(is_nothrow_default_constructible<RpcValue>);
CHECK_TRAIT(is_copy_constructible<RpcValue>);
CHECK_TRAIT(is_nothrow_move_constructible<RpcValue>);
CHECK_TRAIT(is_copy_assignable<RpcValue>);
CHECK_TRAIT(is_nothrow_move_assignable<RpcValue>);
CHECK_TRAIT(is_nothrow_destructible<RpcValue>);

namespace {
QDebug operator<<(QDebug debug, const std::string &s)
{
	//QDebugStateSaver saver(debug);
	debug << s.c_str();

	return debug;
}
}

class TestRpcValue: public QObject
{
	Q_OBJECT
private:
	void textTest()
	{
		qDebug() << "============= chainpack text test ============\n";
		const string simple_test =
				R"({"k1":"v1", "k2":42, "k3":["a",123,true,false,null]})";

		string err;
		const auto json = RpcValue::parseJson(simple_test, err);

		qDebug() << "k1: " << json["k1"].toString().c_str();
		qDebug() << "k3: " << json["k3"].dumpJson().c_str();

		RpcValue cp = json;
		//cp.setMeta(json["k1"]);
		qDebug() << "cp: " << cp.dumpText();

		//for (auto &k : json["k3"].toList()) {
		//	qDebug() << "    - " << k.dumpJson();
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
		auto json_comment = RpcValue::parseJson( comment_test, err_comment);
		QVERIFY(!json_comment.isNull());
		QVERIFY(err_comment.empty());

		comment_test = "{\"a\": 1}//trailing line comment";
		json_comment = RpcValue::parseJson( comment_test, err_comment);
		QVERIFY(!json_comment.isNull());
		QVERIFY(err_comment.empty());

		comment_test = "{\"a\": 1}/*trailing multi-line comment*/";
		json_comment = RpcValue::parseJson( comment_test, err_comment);
		QVERIFY(!json_comment.isNull());
		QVERIFY(err_comment.empty());

		string failing_comment_test = "{\n/* unterminated comment\n\"a\": 1,\n}";
		string err_failing_comment;
		RpcValue json_failing_comment = RpcValue::parseJson( failing_comment_test, err_failing_comment);
		QVERIFY(!json_failing_comment.isValid());
		QVERIFY(!err_failing_comment.empty());

		failing_comment_test = "{\n/* unterminated trailing comment }";
		json_failing_comment = RpcValue::parseJson( failing_comment_test, err_failing_comment);
		QVERIFY(!json_failing_comment.isValid());
		QVERIFY(!err_failing_comment.empty());

		failing_comment_test = "{\n/ / bad comment }";
		json_failing_comment = RpcValue::parseJson( failing_comment_test, err_failing_comment);
		QVERIFY(!json_failing_comment.isValid());
		QVERIFY(!err_failing_comment.empty());

		failing_comment_test = "{// bad comment }";
		json_failing_comment = RpcValue::parseJson( failing_comment_test, err_failing_comment);
		QVERIFY(!json_failing_comment.isValid());
		QVERIFY(!err_failing_comment.empty());

		failing_comment_test = "{\n\"a\": 1\n}/";
		json_failing_comment = RpcValue::parseJson( failing_comment_test, err_failing_comment);
		QVERIFY(!json_failing_comment.isValid());
		QVERIFY(!err_failing_comment.empty());

		failing_comment_test = "{/* bad\ncomment *}";
		json_failing_comment = RpcValue::parseJson( failing_comment_test, err_failing_comment);
		QVERIFY(!json_failing_comment.isValid());
		QVERIFY(!err_failing_comment.empty());

		{
			std::list<int> l1 { 1, 2, 3 };
			std::vector<int> l2 { 1, 2, 3 };
			std::set<int> l3 { 1, 2, 3 };
			QCOMPARE(RpcValue(l1), RpcValue(l2));
			QCOMPARE(RpcValue(l2), RpcValue(l3));
			std::map<string, string> m1 { { "k1", "v1" }, { "k2", "v2" } };
			std::unordered_map<string, string> m2 { { "k1", "v1" }, { "k2", "v2" } };
			QCOMPARE(RpcValue(m1), RpcValue(m2));
		}
		// ChainPack literals
		const RpcValue obj = RpcValue::Map({
											   { "k1", "v1" },
											   { "k2", 42.0 },
											   { "k3", RpcValue::List({ "a", 123.0, true, false, nullptr }) },
										   });

		qDebug() << "obj: " << obj.dumpJson();
		QCOMPARE(obj.dumpJson().c_str(), "{\"k1\": \"v1\", \"k2\": 42, \"k3\": [\"a\", 123, true, false, null]}");

		QCOMPARE(RpcValue("a").toDouble(), 0.);
		QCOMPARE(RpcValue("a").toString().c_str(), "a");
		QCOMPARE(RpcValue().toDouble(), 0.);

		const string unicode_escape_test =
				R"([ "blah\ud83d\udca9blah\ud83dblah\udca9blah\u0000blah\u1234" ])";

		const char utf8[] = "blah" "\xf0\x9f\x92\xa9" "blah" "\xed\xa0\xbd" "blah"
							"\xed\xb2\xa9" "blah" "\0" "blah" "\xe1\x88\xb4";

		RpcValue uni = RpcValue::parseJson(unicode_escape_test, err);
		QVERIFY(uni[0].toString().size() == (sizeof utf8) - 1);
		QVERIFY(std::memcmp(uni[0].toString().data(), utf8, sizeof utf8) == 0);

		RpcValue my_json = RpcValue::Map {
			{ "key1", "value1" },
			{ "key2", false },
			{ "key3", RpcValue::List { 1, 2, 3 } },
		};
		std::string json_obj_str = my_json.dumpJson();
		qDebug() << "json_obj_str: " << json_obj_str.c_str();
		QCOMPARE(json_obj_str.c_str(), "{\"key1\": \"value1\", \"key2\": false, \"key3\": [1, 2, 3]}");

		class Point {
		public:
			int x;
			int y;
			Point (int x, int y) : x(x), y(y) {}
			RpcValue to_json() const { return RpcValue::List { x, y }; }
		};

		std::vector<Point> points = { { 1, 2 }, { 10, 20 }, { 100, 200 } };
		std::string points_json = RpcValue(points).dumpJson();
		qDebug() << "points_json: " << points_json.c_str();
		QVERIFY(points_json == "[[1, 2], [10, 20], [100, 200]]");
	}

	static std::string binary_dump(const RpcValue::Blob &out)
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

	void binaryTest()
	{
		qDebug() << "============= chainpack binary test ============";
		for (int i = ChainPackProtocol::TypeInfo::TERM; i <= ChainPackProtocol::TypeInfo::TRUE; ++i) {
			RpcValue::Blob out;
			out += i;
			ChainPackProtocol::TypeInfo::Enum e = (ChainPackProtocol::TypeInfo::Enum)i;
			std::ostringstream str;
			str << std::setw(3) << i << " " << binary_dump(out) << " "  << ChainPackProtocol::TypeInfo::name(e);
			qDebug() << str.str();
 		}
		for (int i = ChainPackProtocol::TypeInfo::Null; i <= ChainPackProtocol::TypeInfo::MetaIMap; ++i) {
			RpcValue::Blob out;
			out += i;
			ChainPackProtocol::TypeInfo::Enum e = (ChainPackProtocol::TypeInfo::Enum)i;
			std::ostringstream str;
			str << std::setw(3) << i << " " << binary_dump(out).c_str() << " "  << ChainPackProtocol::TypeInfo::name(e);
			qDebug() << str.str();
		}
		{
			qDebug() << "------------- NULL";
			RpcValue cp1{nullptr};
			std::stringstream out;
			int len = ChainPackProtocol::write(out, cp1);
			QVERIFY(len == 1);
			RpcValue cp2 = ChainPackProtocol::read(out);
			qDebug() << cp1.dumpText() << " " << cp2.dumpText() << " len: " << len << " dump: " << binary_dump(out.str()).c_str();
			QVERIFY(cp1.type() == cp2.type());
		}
		qDebug() << "------------- tiny uint";
	for (RpcValue::UInt n = 0; n < 64; ++n) {
			RpcValue cp1{n};
			std::stringstream out;
			int len = ChainPackProtocol::write(out, cp1);
			if(n < 10)
				qDebug() << n << " " << cp1.dumpText() << " len: " << len << " dump: " << binary_dump(out.str()).c_str();
			QVERIFY(len == 1);
			RpcValue cp2 = ChainPackProtocol::read(out);
			QVERIFY(cp1.type() == cp2.type());
			QVERIFY(cp1.toUInt() == cp2.toUInt());
		}
		{
			qDebug() << "------------- uint";
			RpcValue::UInt n_max = std::numeric_limits<unsigned int>::max();
			auto step = n_max / 1000;
			for (RpcValue::UInt n = 64; n < (n_max - step); n += step) {
				RpcValue cp1{n};
				std::stringstream out;
				int len = ChainPackProtocol::write(out, cp1);
				QVERIFY(len > 1);
				RpcValue cp2 = ChainPackProtocol::read(out);
				if(n < 100)
					qDebug() << n << " " << cp1.dumpText() << " " << cp2.dumpText() << " len: " << len << " dump: " << binary_dump(out.str()).c_str();
				QVERIFY(cp1.type() == cp2.type());
				QVERIFY(cp1.toUInt() == cp2.toUInt());
			}
		}
		qDebug() << "------------- tiny int";
		for (RpcValue::Int n = 0; n < 64; ++n) {
			RpcValue cp1{n};
			std::stringstream out;
			int len = ChainPackProtocol::write(out, cp1);
			if(n < 10)
				qDebug() << n << " " << cp1.dumpText() << " len: " << len << " dump: " << binary_dump(out.str()).c_str();
			QVERIFY(len == 1);
			RpcValue cp2 = ChainPackProtocol::read(out);
			QVERIFY(cp1.type() == cp2.type());
			QVERIFY(cp1.toInt() == cp2.toInt());
		}
		{
			qDebug() << "------------- int";
			{
				auto n_max = std::numeric_limits<signed int>::max();
				auto n_min = std::numeric_limits<signed int>::min()+1;
				auto step = n_max / 100;
			for (RpcValue::Int n = n_min; n < (n_max - step); n += step) {
					RpcValue cp1{n};
					std::stringstream out;
					int len = ChainPackProtocol::write(out, cp1);
					if(n >= 0 && n < 64)
						QVERIFY(len == 1);
					else
						QVERIFY(len > 1);
					RpcValue cp2 = ChainPackProtocol::read(out);
					if(n < 1000 && n > -1000)
						qDebug() << n << " " << cp1.dumpText() << " " << cp2.dumpText() << " len: " << len << " dump: " << binary_dump(out.str()).c_str();
					QVERIFY(cp1.type() == cp2.type());
					QVERIFY(cp1.toInt() == cp2.toInt());
				}
			}
		}
		{
			qDebug() << "------------- double";
			{
				auto n_max = 1000000.;
				auto n_min = -1000000.;
				auto step = (n_max - n_min) / 100;
				for (auto n = n_min; n < n_max; n += step) {
					RpcValue cp1{n};
					std::stringstream out;
					int len = ChainPackProtocol::write(out, cp1);
					QVERIFY(len > 1);
					RpcValue cp2 = ChainPackProtocol::read(out);
					if(n > -3*step && n < 3*step)
						qDebug() << n << " " << cp1.dumpText() << " " << cp2.dumpText() << " len: " << len << " dump: " << binary_dump(out.str()).c_str();
					QVERIFY(cp1.type() == cp2.type());
					QVERIFY(cp1.toDouble() == cp2.toDouble());
				}
			}
			{
				double n_max = std::numeric_limits<double>::max();
				double n_min = std::numeric_limits<double>::min();
				double step = -1.23456789e10;
				//qDebug() << n_min << " - " << n_max << ": " << step << " === " << (n_max / step / 10);
				for (double n = n_min; n < n_max / -step / 10; n *= step) {
					RpcValue cp1{n};
					std::stringstream out;
					int len = ChainPackProtocol::write(out, cp1);
					QVERIFY(len > 1);
					RpcValue cp2 = ChainPackProtocol::read(out);
					if(n > -100 && n < 100)
						qDebug() << n << " - " << cp1.dumpText() << " " << cp2.dumpText() << " len: " << len << " dump: " << binary_dump(out.str()).c_str();
					QVERIFY(cp1.type() == cp2.type());
					QVERIFY(cp1.toDouble() == cp2.toDouble());
				}
			}
		}
		{
			qDebug() << "------------- bool";
			for(bool b : {false, true}) {
				RpcValue cp1{b};
				std::stringstream out;
				int len = ChainPackProtocol::write(out, cp1);
				QVERIFY(len == 1);
				RpcValue cp2 = ChainPackProtocol::read(out);
				qDebug() << b << " " << cp1.dumpText() << " " << cp2.dumpText() << " len: " << len << " dump: " << binary_dump(out.str()).c_str();
				QVERIFY(cp1.type() == cp2.type());
				QVERIFY(cp1.toBool() == cp2.toBool());
			}
		}
		{
			qDebug() << "------------- blob";
			RpcValue::Blob blob{"fpowfksapofkpsaokfsa"};
			blob[5] = 0;
			RpcValue cp1{blob};
			std::stringstream out;
			int len = ChainPackProtocol::write(out, cp1);
			RpcValue cp2 = ChainPackProtocol::read(out);
			qDebug() << blob.toString() << " " << cp1.dumpText() << " " << cp2.dumpText() << " len: " << len << " dump: " << binary_dump(out.str()).c_str();
			QVERIFY(cp1.type() == cp2.type());
			QVERIFY(cp1.toBlob() == cp2.toBlob());
		}
		{
			qDebug() << "------------- string";
			RpcValue::String str{"lhklhklfkjdslfkposkfp79"};
			str[5] = 0;
			RpcValue cp1{str};
			std::stringstream out;
			ChainPackProtocol::write(out, cp1);
			RpcValue cp2 = ChainPackProtocol::read(out);
			//qDebug() << str << " " << cp1.dumpText() << " " << cp2.dumpText() << " len: " << len << " dump: " << binary_dump(out.str());
			QVERIFY(cp1.type() == cp2.type());
			QVERIFY(cp1.toString() == cp2.toString());
		}
		{
			qDebug() << "------------- DateTime";
			std::string str = "2017-05-03T15:52:31.123";
			RpcValue::DateTime dt = RpcValue::DateTime::fromString(str);
			RpcValue cp1{dt};
			std::stringstream out;
			int len = ChainPackProtocol::write(out, cp1);
			RpcValue cp2 = ChainPackProtocol::read(out);
			qDebug() << str << " " << dt.toUtcString().c_str() << " " << cp1.dumpText() << " " << cp2.dumpText() << " len: " << len << " dump: " << binary_dump(out.str()).c_str();
			QVERIFY(cp1.type() == cp2.type());
			QVERIFY(cp1.toInt() == cp2.toInt());
		}
		{
			qDebug() << "------------- List";
			{
				const std::string s = R"(["a",123,true,[1,2,3],null])";
				string err;
				RpcValue cp1 = RpcValue::parseJson(s, err);
				std::stringstream out;
				int len = ChainPackProtocol::write(out, cp1);
				RpcValue cp2 = ChainPackProtocol::read(out);
				qDebug() << s << " " << cp1.dumpText() << " " << cp2.dumpText() << " len: " << len << " dump: " << binary_dump(out.str()).c_str();
				QVERIFY(cp1.type() == cp2.type());
				QVERIFY(cp1.toList() == cp2.toList());
			}
			{
				RpcValue cp1{RpcValue::List{1,2,3}};
				std::stringstream out;
				int len = ChainPackProtocol::write(out, cp1);
				RpcValue cp2 = ChainPackProtocol::read(out);
				qDebug() << cp1.dumpText() << " " << cp2.dumpText() << " len: " << len << " dump: " << binary_dump(out.str()).c_str();
				QVERIFY(cp1.type() == cp2.type());
				QVERIFY(cp1.toList() == cp2.toList());
			}
		}
		{
			qDebug() << "------------- Array";
			{
				RpcValue cp1{RpcValue::Array{RpcValue::Type::Int, {1, 2, 3}}};
				std::stringstream out;
				int len = ChainPackProtocol::write(out, cp1);
				RpcValue cp2 = ChainPackProtocol::read(out);
				qDebug() << cp1.dumpText() << " " << cp2.dumpText() << " len: " << len << " dump: " << binary_dump(out.str()).c_str();
				QVERIFY(cp1.type() == cp2.type());
				QVERIFY(cp1.toList() == cp2.toList());
			}
			{
				RpcValue::Array t{RpcValue::Type::List};
				t.push_back(RpcValue::List{1, 2});
				t.push_back(RpcValue::List{RpcValue{3}, RpcValue{4}});
				t.push_back(RpcValue::List{RpcValue{5}, RpcValue{6}});
				t.push_back(RpcValue::List{RpcValue{7}, RpcValue{8}});
				RpcValue cp1{t};
				std::stringstream out;
				int len = ChainPackProtocol::write(out, cp1);
				RpcValue cp2 = ChainPackProtocol::read(out);
				qDebug() << cp1.dumpText() << " " << cp2.dumpText() << " len: " << len << " dump: " << binary_dump(out.str());
				QVERIFY(cp1.type() == cp2.type());
				QVERIFY(cp1.toList() == cp2.toList());
			}
		}
		{
			qDebug() << "------------- Map";
			{
				RpcValue cp1{{
						{"foo", 1},
						{"bar", 2},
						{"baz", 3},
							 }};
				std::stringstream out;
				int len = ChainPackProtocol::write(out, cp1);
				RpcValue cp2 = ChainPackProtocol::read(out);
				qDebug() << cp1.dumpText() << " " << cp2.dumpText() << " len: " << len << " dump: " << binary_dump(out.str());
				QVERIFY(cp1.type() == cp2.type());
				QVERIFY(cp1.toMap() == cp2.toMap());
			}
			{
				RpcValue cp1{{
						{"foo", RpcValue::List{11,12,13}},
						{"bar", 2},
						{"baz", 3},
							 }};
				std::stringstream out;
				int len = ChainPackProtocol::write(out, cp1);
				RpcValue cp2 = ChainPackProtocol::read(out);
				qDebug() << cp1.dumpText() << " " << cp2.dumpText() << " len: " << len << " dump: " << binary_dump(out.str());
				QVERIFY(cp1.type() == cp2.type());
				QVERIFY(cp1.toMap() == cp2.toMap());
			}
		}
		{
			qDebug() << "------------- IMap";
			{
				RpcValue::IMap map {
					{1, "foo"},
					{2, "bar"},
					{3, "baz"},
				};
				RpcValue cp1{map};
				std::stringstream out;
				int len = ChainPackProtocol::write(out, cp1);
				RpcValue cp2 = ChainPackProtocol::read(out);
				qDebug() << cp1.dumpText() << " " << cp2.dumpText() << " len: " << len << " dump: " << binary_dump(out.str());
				QVERIFY(cp1.type() == cp2.type());
				QVERIFY(cp1.toIMap() == cp2.toIMap());
			}
			{
				RpcValue cp1{{
						{127, RpcValue::List{11,12,13}},
						{128, 2},
						{129, 3},
							 }};
				std::stringstream out;
				int len = ChainPackProtocol::write(out, cp1);
				RpcValue cp2 = ChainPackProtocol::read(out);
				qDebug() << cp1.dumpText() << " " << cp2.dumpText() << " len: " << len << " dump: " << binary_dump(out.str());
				QVERIFY(cp1.type() == cp2.type());
				QVERIFY(cp1.toIMap() == cp2.toIMap());
			}
		}
		{
			qDebug() << "------------- Meta";
			RpcValue cp1{RpcValue::List{17, 18, 19}};
			cp1.setMetaValue(RpcValue::Tag::MetaTypeNameSpaceId, (unsigned)1);
			cp1.setMetaValue(RpcValue::Tag::MetaTypeId, (unsigned)2);
			cp1.setMetaValue(RpcValue::Tag::USER, "foo");
			cp1.setMetaValue(RpcValue::Tag::USER+1, RpcValue::List{1,2,3});
			std::stringstream out;
			int len = ChainPackProtocol::write(out, cp1);
			RpcValue cp2 = ChainPackProtocol::read(out);
			std::ostream::pos_type consumed = out.tellg();
			qDebug() << cp1.dumpText() << " " << cp2.dumpText() << " len: " << len << " consumed: " << consumed << " dump: " << binary_dump(out.str());
			QVERIFY(len == (int)consumed);
			QVERIFY(cp1.type() == cp2.type());
			QVERIFY(cp1.metaData() == cp2.metaData());
		}
	}

	void charArrayTest()
	{
		{
			const char ca[] = "Ahoj Babi";
			CharDataStreamBuffer sb(ca, sizeof(ca));
			std::istream in(&sb);
			while(true) {
				char c = in.get();
				if(in.eof())
					break;
				qDebug() << c << "pos:" << in.tellg();
 			}
		}
		/*
		{
			std::istringstream in("ahoj babi");
			while(true) {
				char c = in.get();
				if(in.eof())
					break;
				qDebug() << c << "pos:" << in.tellg();
 			}
		}
		*/
		uint64_t u(585734057307);
		RpcValue cp1{RpcValue::List{17, 18, 19}};
		std::stringstream out;
		ChainPackProtocol::writeUIntData(out, u);
		//qDebug() << u << binary_dump(out.str());
		ChainPackProtocol::write(out, cp1);

		std::string s = out.str();
		qDebug() << u << binary_dump(s);
		size_t len;
		uint64_t u2 = ChainPackProtocol::readUIntData(s.c_str(), s.length(), &len);
		qDebug() << u << binary_dump(s) << "len:" << len;
		QVERIFY(len > 0);
		QCOMPARE(u, u2);
	}

private slots:
	void initTestCase()
	{
		//qDebug("called before everything else");
	}
	void testProtocol()
	{
		textTest();
		binaryTest();
		charArrayTest();
	}

	void cleanupTestCase()
	{
		//qDebug("called after firstTest and secondTest");
	}
};

QTEST_MAIN(TestRpcValue)
#include "tst_chainpack_rpcvalue.moc"
