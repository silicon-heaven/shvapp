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
	DOCTEST_SUBCASE("sanitizeLog correctly removes stuff")
	{
		std::string shv_path = "shv/eyas/opc";
		std::string cache_dir_path = "eyas/opc";
		SEND_SITES_YIELD(mock_sites::fin_slave_broker);
		EXPECT_SUBSCRIPTION_YIELD(shv_path, "mntchng");
		EXPECT_SUBSCRIPTION(shv_path, "chng");

		create_dummy_cache_files(cache_dir_path, {
			{ "2022-07-07T18-06-15-557.log2", dummy_logfile },
			{ "2022-07-07T18-06-15-560.log2", dummy_logfile },
		});

		DOCTEST_SUBCASE("when size is within quota")
		{
			HistoryApp::instance()->setSingleCacheSizeLimit(5000);
			expected_cache_contents = RpcValue::List({{
				RpcValue::List{ "2022-07-07T18-06-15-557.log2", 308UL },
				RpcValue::List{ "2022-07-07T18-06-15-560.log2", 308UL }
			}});
		}

		DOCTEST_SUBCASE("when size is over quota")
		{
			HistoryApp::instance()->setSingleCacheSizeLimit(500);
			expected_cache_contents = RpcValue::List({{
				RpcValue::List{ "2022-07-07T18-06-15-560.log2", 308UL }
			}});
		}

		REQUEST_YIELD(cache_dir_path, "sanitizeLog", RpcValue());
		EXPECT_RESPONSE("Cache sanitization done");
		REQUIRE(get_cache_contents(cache_dir_path) == expected_cache_contents);
	}

	DOCTEST_SUBCASE("sanitizer runs periodically")
	{
		HistoryApp::instance()->cliOptions()->setJournalSanitizerInterval(1);
		SEND_SITES_YIELD(mock_sites::two_devices);
		EXPECT_SUBSCRIPTION_YIELD("shv/one", "mntchng");
		EXPECT_SUBSCRIPTION_YIELD("shv/one", "chng");
		EXPECT_SUBSCRIPTION_YIELD("shv/two", "mntchng");
		EXPECT_SUBSCRIPTION("shv/two", "chng");

		HistoryApp::instance()->setSingleCacheSizeLimit(500);

		create_dummy_cache_files("one", {
			{ "2022-07-07T18-06-15-557.log2", dummy_logfile },
			{ "2022-07-07T18-06-15-560.log2", dummy_logfile },
		});
		create_dummy_cache_files("two", {
			{ "2022-07-07T18-06-15-557.log2", dummy_logfile },
			{ "2022-07-07T18-06-15-560.log2", dummy_logfile },
		});
		expected_cache_contents = RpcValue::List({{
			RpcValue::List{ "2022-07-07T18-06-15-560.log2", 308UL }
		}});

		DRIVER_WAIT(3000);

		REQUIRE(get_cache_contents("one") == expected_cache_contents);
		REQUIRE(get_cache_contents("two") == expected_cache_contents);
	}
}

TEST_HISTORYPROVIDER_MAIN("sanitizer")
