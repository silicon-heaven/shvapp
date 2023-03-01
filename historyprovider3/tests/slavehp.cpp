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
	std::string cache_dir_path = "fin/hel/tram/hel002";
	SEND_SITES_YIELD(mock_sites::fin_master_broker);
	EXPECT_SUBSCRIPTION_YIELD("shv", "mntchng");
	EXPECT_SUBSCRIPTION("shv/fin/hel/tram/hel002", "chng");
	std::string master_shv_journal_path = "_shvjournal";
	std::string slave_shv_journal_path = "shv/fin/hel/tram/hel002/.local/history/shvjournal";
	REQUEST_YIELD(master_shv_journal_path, "syncLog", RpcValue());
	EXPECT_REQUEST(slave_shv_journal_path, "lsfiles", ls_size_true);

	DOCTEST_SUBCASE("Remote - has files, local - empty")
	{
		create_dummy_cache_files(cache_dir_path, {});
		// NOTE: I'm only really testing whether historyprovider correctly enters the .local broker.
		expected_cache_contents = RpcValue::List({{
			RpcValue::List{ "eyas/opc/2022-07-07T18-06-15-557.log2", dummy_logfile.size() }
		}});
		RESPOND_YIELD((RpcValue::List{{
			{ "eyas", "d", 0 }
		}}));
		EXPECT_REQUEST(join(slave_shv_journal_path, "eyas"), "lsfiles", ls_size_true);
		RESPOND_YIELD((RpcValue::List{{
			{ "opc", "d", 0 }
		}}));
		EXPECT_REQUEST(join(slave_shv_journal_path, "eyas/opc"), "lsfiles", ls_size_true);
		RESPOND_YIELD((RpcValue::List{{
			{ "2022-07-07T18-06-15-557.log2", "f", dummy_logfile.size() }
		}}));

		EXPECT_REQUEST(join(slave_shv_journal_path, "eyas/opc/2022-07-07T18-06-15-557.log2"), "read", read_offset_0);
		RESPOND_YIELD(RpcValue::stringToBlob(dummy_logfile));
	}

	DOCTEST_SUBCASE("dirty log")
	{
		create_dummy_cache_files(cache_dir_path, {});
		// Respond to the initial `lsfiles` request.
		RESPOND_YIELD(RpcValue::List());

		DOCTEST_SUBCASE("HP accepts events and puts them into the log")
		{
			NOTIFY("shv/fin/hel/tram/hel002/eyas/opc/power-on", "chng", true);
			NOTIFY("shv/fin/hel/tram/hel002/eyas/opc/power-on", "chng", false);

			expected_cache_contents = RpcValue::List({{
				RpcValue::List{ "eyas/opc/dirtylog", 99UL }
			}});
		}
	}

	EXPECT_RESPONSE("All files have been synced");
	REQUIRE(get_cache_contents(cache_dir_path) == expected_cache_contents);
}

TEST_HISTORYPROVIDER_MAIN("slave_hp")
