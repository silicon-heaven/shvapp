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
 * Beginning of standard source file, which makes use of the customizations above.
 */
#include <shv/chainpack/rpcmessage.h>
#include <shv/chainpack/chainpackprotocol.h>

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

using namespace shv::chainpack;
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

CHAINPACK_TEST_CASE(rpcmessage_test)
{
	std::cout << "============= chainpack rpcmessage test ============\n";
	std::cout << "------------- RpcRequest \n";
	{
		RpcRequest rq;
		rq.setId(123)
				.setMethod("foo")
				.setParams({{
							   {"a", 45},
							   {"b", "bar"},
							   {"c", RpcValue::List{1,2,3}},
						   }});
		ChainPackProtocol::Blob out;
		RpcValue cp1 = rq.value();
		int len = rq.write(out);
		RpcValue cp2 = ChainPackProtocol::read(out);
		std::cout << cp1.dumpText() << " " << cp2.dumpText() << " len: " << len << " dump: " << binary_dump(out) << "\n";
		CHAINPACK_TEST_ASSERT(cp1.type() == cp2.type());
		RpcRequest rq2(cp2);
		CHAINPACK_TEST_ASSERT(rq2.isRequest());
		CHAINPACK_TEST_ASSERT(rq2.id() == rq.id());
		CHAINPACK_TEST_ASSERT(rq2.method() == rq.method());
		CHAINPACK_TEST_ASSERT(rq2.params() == rq.params());
	}
	std::cout << "------------- RpcResponse \n";
	{
		RpcResponse rs;
		rs.setId(123).setResult(42u);
		ChainPackProtocol::Blob out;
		RpcValue cp1 = rs.value();
		int len = rs.write(out);
		RpcValue cp2 = ChainPackProtocol::read(out);
		std::cout << cp1.dumpText() << " " << cp2.dumpText() << " len: " << len << " dump: " << binary_dump(out) << "\n";
		CHAINPACK_TEST_ASSERT(cp1.type() == cp2.type());
		RpcResponse rs2(cp2);
		CHAINPACK_TEST_ASSERT(rs2.isResponse());
		CHAINPACK_TEST_ASSERT(rs2.id() == rs.id());
		CHAINPACK_TEST_ASSERT(rs2.result() == rs.result());
	}
	{
		RpcResponse rs;
		rs.setId(123)
				.setError(RpcResponse::Error::createError(RpcResponse::Error::InvalidParams, "Paramter length should be greater than zero!"));
		ChainPackProtocol::Blob out;
		RpcValue cp1 = rs.value();
		int len = rs.write(out);
		RpcValue cp2 = ChainPackProtocol::read(out);
		std::cout << cp1.dumpText() << " " << cp2.dumpText() << " len: " << len << " dump: " << binary_dump(out) << "\n";
		CHAINPACK_TEST_ASSERT(cp1.type() == cp2.type());
		RpcResponse rs2(cp2);
		CHAINPACK_TEST_ASSERT(rs2.isResponse());
		CHAINPACK_TEST_ASSERT(rs2.id() == rs.id());
		CHAINPACK_TEST_ASSERT(rs2.error() == rs.error());
	}
	std::cout << "------------- RpcNotify \n";
	{
		RpcRequest rq;
		rq.setMethod("foo")
				.setParams({{
							   {"a", 45},
							   {"b", "bar"},
							   {"c", RpcValue::List{1,2,3}},
						   }});
		ChainPackProtocol::Blob out;
		RpcValue cp1 = rq.value();
		int len = rq.write(out);
		RpcValue cp2 = ChainPackProtocol::read(out);
		std::cout << cp1.dumpText() << " " << cp2.dumpText() << " len: " << len << " dump: " << binary_dump(out) << "\n";
		CHAINPACK_TEST_ASSERT(cp1.type() == cp2.type());
		RpcRequest rq2(cp2);
		CHAINPACK_TEST_ASSERT(rq2.isNotify());
		CHAINPACK_TEST_ASSERT(rq2.method() == rq.method());
		CHAINPACK_TEST_ASSERT(rq2.params() == rq.params());
	}
}

#if CHAINPACK_TEST_STANDALONE_MAIN

static void parse_from_stdin() {
	string buf;
	string line;
	while (std::getline(std::cin, line)) {
		buf += line + "\n";
	}

	string err;
	auto json = RpcValue::parseJson(buf, err);
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

	try {
		rpcmessage_test();
	}
	catch (std::runtime_error &e) {
		std::cout << "ERROR: " << e.what() << "\n";
	}
}

#endif // CHAINPACK_TEST_STANDALONE_MAIN

// Insert user-defined suffix code (function definitions, etc)
// to set up a custom test suite
CHAINPACK_TEST_CPP_SUFFIX_CODE
