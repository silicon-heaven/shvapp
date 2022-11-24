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
	DOCTEST_SUBCASE("pushing logs")
	{
		std::string cache_dir_path = "pushlog";
		create_dummy_cache_files(cache_dir_path, {});
		SEND_SITES_YIELD(mock_sites::pushlog_hp);
		EXPECT_SUBSCRIPTION("shv/pushlog", "mntchng");

		DOCTEST_SUBCASE("empty log")
		{
			REQUEST_YIELD(cache_dir_path, "pushLog", RpcValue::List());
			EXPECT_RESPONSE(true);
		}

		DOCTEST_SUBCASE("some logging")
		{
			REQUEST_YIELD(cache_dir_path, "pushLog", dummy_pushlog);
			EXPECT_RESPONSE(true);
			REQUIRE(get_cache_contents(cache_dir_path) == RpcValue::List({{
				RpcValue::List{ "2022-07-07T18-06-15-557.log2", 53UL }
			}}));

			DOCTEST_SUBCASE("pushLog rejects older pushlogs")
			{
				REQUEST_YIELD(cache_dir_path, "pushLog", dummy_pushlog2);
				EXPECT_RESPONSE(true);
				REQUIRE(get_cache_contents(cache_dir_path) == RpcValue::List({{
					RpcValue::List{ "2022-07-07T18-06-15-557.log2", 53UL }
				}}));
			}

			DOCTEST_SUBCASE("pushLog rejects pushlogs with same timestamp and same path")
			{
				REQUEST_YIELD(cache_dir_path, "pushLog", dummy_pushlog);
				EXPECT_RESPONSE(true);
				REQUIRE(get_cache_contents(cache_dir_path) == RpcValue::List({{
					RpcValue::List{ "2022-07-07T18-06-15-557.log2", 53UL }
				}}));
			}

			DOCTEST_SUBCASE("pushLog accepts pushlogs with same timestamp and different path")
			{
				REQUEST_YIELD(cache_dir_path, "pushLog", dummy_pushlog3);
				EXPECT_RESPONSE(true);
				REQUIRE(get_cache_contents(cache_dir_path) == RpcValue::List({{
					RpcValue::List{ "2022-07-07T18-06-15-557.log2", 129UL }
				}}));
			}

		}
	}

	DOCTEST_SUBCASE("HP directly above a pushlog don't have a syncLog method" )
	{
		std::string shv_path = "pushlog";
		SEND_SITES_YIELD(mock_sites::pushlog_hp);
		EXPECT_SUBSCRIPTION("shv/pushlog", "mntchng");
		REQUEST_YIELD(shv_path, "syncLog", RpcValue());
		EXPECT_ERROR("RPC ERROR MethodCallException: Method: 'syncLog' on path 'pushlog/' doesn't exist.");
	}

	DOCTEST_SUBCASE("master HP can syncLog pushlogs from slave HPs")
	{
		std::string cache_dir_path = "shv/master";
		SEND_SITES_YIELD(mock_sites::master_hp_with_slave_pushlog);
		EXPECT_SUBSCRIPTION_YIELD(cache_dir_path, "mntchng");
		EXPECT_SUBSCRIPTION(cache_dir_path, "chng");
		REQUEST_YIELD("_shvjournal", "syncLog", RpcValue());
		EXPECT_REQUEST("shv/master/.local/history/shvjournal", "lsfiles", ls_size_true);
		RESPOND_YIELD(RpcValue::List());
		EXPECT_RESPONSE("All files have been synced");
	}

	DOCTEST_SUBCASE("periodic syncing")
	{
		HistoryApp::instance()->cliOptions()->setSyncIteratorInterval(1);

		DOCTEST_SUBCASE("master HP")
		{
			std::string cache_dir_path = "shv/master/pushlog";
			SEND_SITES_YIELD(mock_sites::master_hp_with_slave_pushlog);
			EXPECT_SUBSCRIPTION_YIELD("shv/master", "mntchng");

			DOCTEST_SUBCASE("logs aren't old enough")
			{
				EXPECT_SUBSCRIPTION("shv/master", "chng");
				create_dummy_cache_files(cache_dir_path, {
					// Make a 100 second old entry.
					{"dirtylog", QDateTime::currentDateTimeUtc().addSecs(-100).toString(Qt::DateFormat::ISODate).toStdString() + "	809781	zone1/zone/Zone1/plcDisconnected	false		chng	2	\n"}
				});
				// shouldn't sync here
				DRIVER_WAIT(10);
			}

			DOCTEST_SUBCASE("logs are old enough")
			{
				EXPECT_SUBSCRIPTION_YIELD("shv/master", "chng");
				create_dummy_cache_files(cache_dir_path, {
					// Make a two hour old entry.
					{"dirtylog", QDateTime::currentDateTimeUtc().addSecs(-60*60*2).toString(Qt::DateFormat::ISODate).toStdString() + "	809781	zone1/zone/Zone1/plcDisconnected	false		chng	2	\n"}
				});

				EXPECT_REQUEST("shv/master/.local/history/shvjournal", "lsfiles", ls_size_true);
				RESPOND_YIELD(RpcValue::List());
			}

			DOCTEST_SUBCASE("no logs - should always sync")
			{
				EXPECT_SUBSCRIPTION_YIELD("shv/master", "chng");
				create_dummy_cache_files(cache_dir_path, {});

				EXPECT_REQUEST("shv/master/.local/history/shvjournal", "lsfiles", ls_size_true);
				RESPOND_YIELD(RpcValue::List());
			}
		}

		DOCTEST_SUBCASE("more sites")
		{
			SEND_SITES_YIELD(mock_sites::two_devices);
			EXPECT_SUBSCRIPTION_YIELD("shv/one", "mntchng");
			EXPECT_SUBSCRIPTION_YIELD("shv/one", "chng");
			EXPECT_SUBSCRIPTION_YIELD("shv/two", "mntchng");
			EXPECT_SUBSCRIPTION_YIELD("shv/two", "chng");

			EXPECT_REQUEST("shv/one/.app/shvjournal", "lsfiles", ls_size_true);
			RESPOND_YIELD(RpcValue::List());
			EXPECT_REQUEST("shv/two/.app/shvjournal", "lsfiles", ls_size_true);
			RESPOND_YIELD(RpcValue::List());
			EXPECT_REQUEST("shv/one/.app/shvjournal", "lsfiles", ls_size_true);
			RESPOND_YIELD(RpcValue::List());
		}

		DOCTEST_SUBCASE("slave HP shouldn't sync")
		{
			SEND_SITES_YIELD(mock_sites::pushlog_hp);
			EXPECT_SUBSCRIPTION("shv/pushlog", "mntchng");
			DRIVER_WAIT(1500);
			// Slave pushlog shouldn't sync. We should not have any messages here.
			CAPTURE(m_messageQueue.head());
			REQUIRE(m_messageQueue.empty());
		}
	}

	DOCTEST_SUBCASE("master HP should sync on mntchng")
	{
		SEND_SITES_YIELD(mock_sites::master_hp_with_slave_pushlog);
		EXPECT_SUBSCRIPTION_YIELD("shv/master", "mntchng");
		EXPECT_SUBSCRIPTION("shv/master", "chng");
		NOTIFY_YIELD("shv/master", "mntchng", true);
		EXPECT_REQUEST("shv/master/.local/history/shvjournal", "lsfiles", ls_size_true);
		RESPOND(RpcValue::List()); // We only test if the syncLog triggers.
	}

	DOCTEST_SUBCASE("slave HP shouldn't sync on mntchng")
	{
		SEND_SITES_YIELD(mock_sites::pushlog_hp);
		EXPECT_SUBSCRIPTION("shv/pushlog", "mntchng");
		NOTIFY("shv/pushlog", "mntchng", true);
		DRIVER_WAIT(100);
		CAPTURE(m_messageQueue.head());
		REQUIRE(m_messageQueue.empty());
	}

}

TEST_HISTORYPROVIDER_MAIN("pushlog")
