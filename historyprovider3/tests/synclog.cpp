#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "mockrpcconnection.h"
#include "pretty_printers.h"
#include <doctest/doctest.h>

#include "src/appclioptions.h"
#include "src/historyapp.h"
#include "tests/sites.h"
#include "tests/utils.h"

// TODO: add proper testing for file contents (not just sizes)
QCoro::Generator<int> MockRpcConnection::driver()
{
	co_yield {};

	// We want the test to download everything regardless of age. Ten years ought to be enough.
	HistoryApp::instance()->cliOptions()->setCacheInitMaxAge(60 /*seconds*/ * 60 /*minutes*/ * 24 /*hours*/ * 365 /*days*/ * 10 /*years*/);
	std::string shv_path = "shv/eyas/opc";
	std::string cache_dir_path = "eyas/opc";
	SEND_SITES_YIELD(mock_sites::fin_slave_broker);
	EXPECT_SUBSCRIPTION_YIELD(shv_path, "mntchng");
	EXPECT_SUBSCRIPTION(shv_path, "chng");
	REQUEST_YIELD("_shvjournal", "syncLog", RpcValue());
	EXPECT_REQUEST(join(shv_path, "/.app/shvjournal"), "lsfiles", ls_size_true);

	DOCTEST_SUBCASE("syncLog")
	{
		RpcValue::List expected_cache_contents;
		DOCTEST_SUBCASE("Remote and local - empty")
		{
			create_dummy_cache_files(cache_dir_path, {});
			RESPOND_YIELD(RpcValue::List());
		}

		DOCTEST_SUBCASE("Remote - has files, local - empty")
		{
			create_dummy_cache_files(cache_dir_path, {});
			expected_cache_contents = RpcValue::List({{
				RpcValue::List{ "2022-07-07T18-06-15-557.log2", dummy_logfile.size() }
			}});
			RESPOND_YIELD((RpcValue::List({{
				{ "2022-07-07T18-06-15-557.log2", "f", dummy_logfile.size() }
			}})));

			EXPECT_REQUEST("shv/eyas/opc/.app/shvjournal/2022-07-07T18-06-15-557.log2", "read", read_offset_0);
			RESPOND_YIELD(RpcValue::stringToBlob(dummy_logfile));
		}

		DOCTEST_SUBCASE("Remote - has files and subdirectories, local - empty")
		{
			create_dummy_cache_files(cache_dir_path, {});
			expected_cache_contents = RpcValue::List({{
				RpcValue::List{ "subdir/2022-07-07T18-06-15-557.log2", dummy_logfile.size() }
			}});

			RESPOND_YIELD((RpcValue::List({{
				{ "subdir", "d", 0 }
			}})));

			EXPECT_REQUEST(join(shv_path, "/.app/shvjournal/subdir"), "lsfiles", ls_size_true);

			RESPOND_YIELD((RpcValue::List({{
				{ "2022-07-07T18-06-15-557.log2", "f", dummy_logfile.size() }
			}})));

			EXPECT_REQUEST("shv/eyas/opc/.app/shvjournal/subdir/2022-07-07T18-06-15-557.log2", "read", read_offset_0);
			RESPOND_YIELD(RpcValue::stringToBlob(dummy_logfile));
		}

		DOCTEST_SUBCASE("dirty log")
		{
			create_dummy_cache_files(cache_dir_path, {});
			// Respond to the initial `lsfiles` request.
			RESPOND_YIELD(RpcValue::List());

			DOCTEST_SUBCASE("HP accepts events and puts them into the log")
			{
				NOTIFY("shv/eyas/opc/power-on", "chng", true);
				NOTIFY("shv/eyas/opc/power-on", "chng", false);

				expected_cache_contents = RpcValue::List({{
					RpcValue::List{ "dirtylog", 99UL }
				}});
			}

			EXPECT_RESPONSE("All files have been synced");
		}

		DOCTEST_SUBCASE("Don't download files older than we already have")
		{
			expected_cache_contents = RpcValue::List({{
				RpcValue::List{ "2022-07-07T18-06-15-557.log2", 0UL },
				RpcValue::List{ "2022-07-07T18-06-15-558.log2", 0UL }
			}});
			create_dummy_cache_files(cache_dir_path, {
				{"2022-07-07T18-06-15-557.log2", ""},
				{"2022-07-07T18-06-15-558.log2", ""}
			});
			RESPOND_YIELD((RpcValue::List({{
				{ "2022-07-07T18-06-15-000.log2", "f", dummy_logfile.size() }
			}})));

			EXPECT_RESPONSE("All files have been synced");
		}

		DOCTEST_SUBCASE("Don't download files older than one month")
		{
			HistoryApp::instance()->cliOptions()->setCacheInitMaxAge(60 /*seconds*/ * 60 /*minutes*/ * 24 /*hours*/ * 30 /*days*/);
			create_dummy_cache_files(cache_dir_path, {});
			RESPOND_YIELD((RpcValue::List({{
				{ "2022-07-07T18-06-15-000.log2", "f", dummy_logfile.size() }
			}})));

			EXPECT_RESPONSE("All files have been synced");
		}

		REQUIRE(get_cache_contents(cache_dir_path) == expected_cache_contents);
	}

	DOCTEST_SUBCASE("periodic syncLog")
	{
		// Respond to the initial syncLog request.
		RESPOND_YIELD(RpcValue::List());
		EXPECT_RESPONSE("All files have been synced");

		DOCTEST_SUBCASE("dirty log too old")
		{
			create_dummy_cache_files(cache_dir_path, {
				// Make a 100 second old entry.
				{"dirtylog", QDateTime::currentDateTimeUtc().addSecs(-100).toString(Qt::DateFormat::ISODate).toStdString() + "	809781	zone1/zone/Zone1/plcDisconnected	false		chng	2	\n"}
			});

			DOCTEST_SUBCASE("synclog should not trigger")
			{
				HistoryApp::instance()->cliOptions()->setLogMaxAge(1000);
				NOTIFY("shv/eyas/opc/power-on", "chng", true); // Send an event so that HP checks for dirty log age.
				// Nothing happens afterwards, since max age is >10
			}
		}

		DOCTEST_SUBCASE("device mounted")
		{
			NOTIFY_YIELD(shv_path, "mntchng", true);
			EXPECT_REQUEST(join(shv_path, "/.app/shvjournal"), "lsfiles", ls_size_true);
			RESPOND(RpcValue::List()); // We only test if the syncLog triggers.
		}

		DOCTEST_SUBCASE("device unmounted")
		{
			NOTIFY(shv_path, "mntchng", false);
			// Sync shouldn't trigger.
			DRIVER_WAIT(100);
			CAPTURE(m_messageQueue.head());
			REQUIRE(m_messageQueue.empty());
		}
	}
}

TEST_HISTORYPROVIDER_MAIN("synclog")
