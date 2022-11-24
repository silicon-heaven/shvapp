#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "mockrpcconnection.h"
#include "pretty_printers.h"
#include <doctest/doctest.h>

#include "src/appclioptions.h"
#include "src/historyapp.h"
#include "tests/sites.h"
#include "tests/utils.h"

QCoro::Generator<int> MockRpcConnection::driver()
{
	co_yield {};

	// We want the test to download everything regardless of age. Ten years ought to be enough.
	HistoryApp::instance()->cliOptions()->setCacheInitMaxAge(60 /*seconds*/ * 60 /*minutes*/ * 24 /*hours*/ * 365 /*days*/ * 10 /*years*/);
	RpcValue::List expected_cache_contents;
	SEND_SITES_YIELD(mock_sites::two_devices);
	EXPECT_SUBSCRIPTION_YIELD("shv/one", "mntchng");
	EXPECT_SUBSCRIPTION_YIELD("shv/one", "chng");
	EXPECT_SUBSCRIPTION_YIELD("shv/two", "mntchng");
	EXPECT_SUBSCRIPTION("shv/two", "chng");

	DOCTEST_SUBCASE("standard operation")
	{
		create_dummy_cache_files("one", {
			{"2022-07-06T18-06-15-000.log2", dummy_logfile},
			{"dirtylog", dummy_logfile2}
		});
		create_dummy_cache_files("two", {
			{"2022-07-06T18-06-15-000.log2", dummy_logfile},
			{"dirtylog", dummy_logfile2}
		});

		expected_cache_contents = RpcValue::List({{
			RpcValue::List{ "2022-07-06T18-06-15-000.log2", 308UL },
			RpcValue::List{ "dirtylog", 249UL }
		}});

		REQUIRE(get_cache_contents("one") == expected_cache_contents);
		REQUIRE(get_cache_contents("two") == expected_cache_contents);

		REQUEST_YIELD("_shvjournal", "syncLog", RpcValue());
		EXPECT_REQUEST("shv/one/.app/shvjournal", "lsfiles", ls_size_true);
		RESPOND_YIELD((RpcValue::List{{
			{ "2022-07-07T18-06-15-557.log2", "f", dummy_logfile2.size() }
		}}));
		EXPECT_REQUEST("shv/one/.app/shvjournal/2022-07-07T18-06-15-557.log2", "read", read_offset_0);
		RESPOND_YIELD(RpcValue::stringToBlob(dummy_logfile2));

		EXPECT_REQUEST("shv/two/.app/shvjournal", "lsfiles", ls_size_true);
		RESPOND_YIELD(RpcValue::List());
		EXPECT_RESPONSE("All files have been synced");

		// two is left intact
		REQUIRE(get_cache_contents("two") == expected_cache_contents);

		expected_cache_contents = RpcValue::List({{
			RpcValue::List{ "2022-07-06T18-06-15-000.log2", 308UL },
			RpcValue::List{ "2022-07-07T18-06-15-557.log2", 148UL },
			RpcValue::List{ "dirtylog", 83UL }
		}});

		REQUIRE(get_cache_contents("one") == expected_cache_contents);
	}

	DOCTEST_SUBCASE("file to trim from is empty")
	{
		create_dummy_cache_files("one", {
			{"dirtylog", dummy_logfile2}
		});

		REQUEST_YIELD("_shvjournal", "syncLog", RpcValue());
		EXPECT_REQUEST("shv/one/.app/shvjournal", "lsfiles", ls_size_true);
		RESPOND_YIELD((RpcValue::List{{
			{ "2022-07-07T18-06-15-557.log2", "f", 0L }
		}}));
		EXPECT_REQUEST("shv/one/.app/shvjournal/2022-07-07T18-06-15-557.log2", "read", read_offset_0);
		RESPOND_YIELD(RpcValue::stringToBlob(""));
		EXPECT_REQUEST("shv/two/.app/shvjournal", "lsfiles", ls_size_true);
		RESPOND_YIELD(RpcValue::List());
		EXPECT_RESPONSE("All files have been synced");
	}

	DOCTEST_SUBCASE("file to trim from only has one entry")
	{
		// This means that the file will be trimmed due to the last-ms algorithm and therefore empty. We don't
		// really want empty files, so we don't even write it.
		create_dummy_cache_files("one", {
			{"dirtylog", dummy_logfile2}
		});

		REQUEST_YIELD("_shvjournal", "syncLog", RpcValue());
		EXPECT_REQUEST("shv/one/.app/shvjournal", "lsfiles", ls_size_true);
		RESPOND_YIELD((RpcValue::List{{
			{ "2022-07-07T18-06-15-557.log2", "f", logfile_one_entry.size() }
		}}));
		EXPECT_REQUEST("shv/one/.app/shvjournal/2022-07-07T18-06-15-557.log2", "read", read_offset_0);
		RESPOND_YIELD(RpcValue::stringToBlob(logfile_one_entry));
		EXPECT_REQUEST("shv/two/.app/shvjournal", "lsfiles", ls_size_true);
		RESPOND_YIELD(RpcValue::List());
		EXPECT_RESPONSE("All files have been synced");

		expected_cache_contents = RpcValue::List({{
			RpcValue::List{ "dirtylog", dummy_logfile2.size() }
		}});

		REQUIRE(get_cache_contents("one") == expected_cache_contents);
	}
}

TEST_HISTORYPROVIDER_MAIN("dirtylog trimming")
