#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "mockrpcconnection.h"
#include "pretty_printers.h"
#include <doctest/doctest.h>

#include "src/appclioptions.h"
#include "src/historyapp.h"
#include "tests/sites.h"
#include "tests/utils.h"

QQueue<std::function<CallNext(MockRpcConnection*)>> setup_test()
{
	QQueue<std::function<CallNext(MockRpcConnection*)>> res;
	// We want the test to download everything regardless of age. Ten years ought to be enough.
	enqueue(res, [=] (MockRpcConnection*) {
		// We want the test to download everything regardless of age. Ten years ought to be enough.
		HistoryApp::instance()->cliOptions()->setCacheInitMaxAge(60 /*seconds*/ * 60 /*minutes*/ * 24 /*hours*/ * 365 /*days*/ * 10 /*years*/);
		return CallNext::Yes;
	});
	DOCTEST_SUBCASE("pushing logs")
	{
		std::string cache_dir_path = "pushlog";
		enqueue(res, [=] (MockRpcConnection* mock) {
			create_dummy_cache_files(cache_dir_path, {});
			SEND_SITES_YIELD(mock_sites::pushlog_hp);
		});
		enqueue(res, [=] (MockRpcConnection* mock) {
			EXPECT_SUBSCRIPTION("shv", "mntchng");
			return CallNext::Yes;
		});

		DOCTEST_SUBCASE("empty log")
		{
			enqueue(res, [=] (MockRpcConnection* mock) {
				REQUEST_YIELD(cache_dir_path, "pushLog", RpcValue::List());
			});
			enqueue(res, [=] (MockRpcConnection* mock) {
				EXPECT_RESPONSE((RpcValue::Map {
					{"since", RpcValue(nullptr)},
					{"until", RpcValue(nullptr)},
					{"msg", "success"}
				}));
			});
		}

		DOCTEST_SUBCASE("some logging")
		{
			enqueue(res, [=] (MockRpcConnection* mock) {
				REQUEST_YIELD(cache_dir_path, "pushLog", dummy_pushlog);
			});
			enqueue(res, [=] (MockRpcConnection* mock) {
				EXPECT_RESPONSE((RpcValue::Map {
					{"since", shv::chainpack::RpcValue::DateTime::fromUtcString("2022-07-07T18:06:15.557Z")},
					{"until", shv::chainpack::RpcValue::DateTime::fromUtcString("2022-07-07T18:06:15.557Z")},
					{"msg", "success"}
				}));
				REQUIRE(get_cache_contents(cache_dir_path) == RpcValue::List({{
					RpcValue::List{ "2022-07-07T18-06-15-557.log2", 53UL }
				}}));
				return CallNext::Yes;
			});

			DOCTEST_SUBCASE("pushLog rejects older pushlogs")
			{
				enqueue(res, [=] (MockRpcConnection* mock) {
					REQUEST_YIELD(cache_dir_path, "pushLog", dummy_pushlog2);
				});

				enqueue(res, [=] (MockRpcConnection* mock) {
					EXPECT_RESPONSE((RpcValue::Map {
						{"since", shv::chainpack::RpcValue::DateTime::fromUtcString("2022-07-07T18:06:15.554Z")},
						{"until", shv::chainpack::RpcValue::DateTime::fromUtcString("2022-07-07T18:06:15.554Z")},
						{"msg", "success"}
					}));
					REQUIRE(get_cache_contents(cache_dir_path) == RpcValue::List({{
						RpcValue::List{ "2022-07-07T18-06-15-557.log2", 53UL }
					}}));
				});
			}

			DOCTEST_SUBCASE("pushLog rejects pushlogs with same timestamp and same path")
			{
				enqueue(res, [=] (MockRpcConnection* mock) {
					REQUEST_YIELD(cache_dir_path, "pushLog", dummy_pushlog);
				});
				enqueue(res, [=] (MockRpcConnection* mock) {
					EXPECT_RESPONSE((RpcValue::Map {
						{"since", shv::chainpack::RpcValue::DateTime::fromUtcString("2022-07-07T18:06:15.557Z")},
						{"until", shv::chainpack::RpcValue::DateTime::fromUtcString("2022-07-07T18:06:15.557Z")},
						{"msg", "success"}
					}));
					REQUIRE(get_cache_contents(cache_dir_path) == RpcValue::List({{
						RpcValue::List{ "2022-07-07T18-06-15-557.log2", 53UL }
					}}));
				});
			}

			DOCTEST_SUBCASE("pushLog accepts pushlogs with same timestamp and different path")
			{
				enqueue(res, [=] (MockRpcConnection* mock) {
					REQUEST_YIELD(cache_dir_path, "pushLog", dummy_pushlog3);
				});
				enqueue(res, [=] (MockRpcConnection* mock) {
					EXPECT_RESPONSE((RpcValue::Map {
						{"since", shv::chainpack::RpcValue::DateTime::fromUtcString("2022-07-07T18:06:15.557Z")},
						{"until", shv::chainpack::RpcValue::DateTime::fromUtcString("2022-07-07T18:06:15.557Z")},
						{"msg", "success"}
					}));
					REQUIRE(get_cache_contents(cache_dir_path) == RpcValue::List({{
						RpcValue::List{ "2022-07-07T18-06-15-557.log2", 129UL }
					}}));
				});
			}
		}
	}

	DOCTEST_SUBCASE("HP directly above a pushlog don't have a syncLog method" )
	{
		std::string shv_path = "pushlog";
		enqueue(res, [=] (MockRpcConnection* mock) {
			SEND_SITES_YIELD(mock_sites::pushlog_hp);
		});
		enqueue(res, [=] (MockRpcConnection* mock) {
			EXPECT_SUBSCRIPTION("shv", "mntchng");
			REQUEST_YIELD(shv_path, "syncLog", synclog_wait);
		});
		enqueue(res, [=] (MockRpcConnection* mock) {
			EXPECT_ERROR("RPC ERROR MethodCallException: Method: 'syncLog' on path 'pushlog/' doesn't exist.");
		});
	}

	DOCTEST_SUBCASE("master HP can syncLog pushlogs from slave HPs")
	{
		std::string cache_dir_path = "shv/master";
		enqueue(res, [=] (MockRpcConnection* mock) {
			SEND_SITES_YIELD(mock_sites::master_hp_with_slave_pushlog);
		});
		enqueue(res, [=] (MockRpcConnection* mock) {
			EXPECT_SUBSCRIPTION_YIELD("shv", "mntchng");
		});
		enqueue(res, [=] (MockRpcConnection* mock) {
			EXPECT_SUBSCRIPTION(cache_dir_path, "chng");
			REQUEST_YIELD("_shvjournal", "syncLog", synclog_wait);
		});
		enqueue(res, [=] (MockRpcConnection* mock) {
			EXPECT_REQUEST("shv/master/.local/history/shvjournal", "lsfiles", ls_size_true);
			RESPOND_YIELD(RpcValue::List());
		});
		enqueue(res, [=] (MockRpcConnection* mock) {
			EXPECT_RESPONSE(R"(["shv/master"])"_cpon);
		});
	}

	DOCTEST_SUBCASE("periodic syncing")
	{
		enqueue(res, [=] (MockRpcConnection*) {
			HistoryApp::instance()->cliOptions()->setSyncIteratorInterval(1);
			return CallNext::Yes;
		});

		DOCTEST_SUBCASE("master HP")
		{
			std::string cache_dir_path = "shv/master/pushlog";
			enqueue(res, [=] (MockRpcConnection* mock) {
				SEND_SITES_YIELD(mock_sites::master_hp_with_slave_pushlog);
			});
			enqueue(res, [=] (MockRpcConnection* mock) {
				EXPECT_SUBSCRIPTION_YIELD("shv", "mntchng");
			});

			DOCTEST_SUBCASE("logs aren't old enough")
			{
				enqueue(res, [=] (MockRpcConnection* mock) {
					EXPECT_SUBSCRIPTION("shv/master", "chng");
					create_dummy_cache_files(cache_dir_path, {
						// Make a 100 second old entry.
						{"dirtylog", QDateTime::currentDateTimeUtc().addSecs(-100).toString(Qt::DateFormat::ISODate).toStdString() + "	809781	zone1/zone/Zone1/plcDisconnected	false		chng	2	\n"}
					});
					// shouldn't sync here
					DRIVER_WAIT(10);
				});
				enqueue(res, [=] (MockRpcConnection*) {
				});
			}

			DOCTEST_SUBCASE("logs are old enough")
			{
				enqueue(res, [=] (MockRpcConnection* mock) {
					create_dummy_cache_files(cache_dir_path, {
						// Make a two hour old entry.
						{"dirtylog", QDateTime::currentDateTimeUtc().addSecs(-60*60*2).toString(Qt::DateFormat::ISODate).toStdString() + "	809781	zone1/zone/Zone1/plcDisconnected	false		chng	2	\n"}
					});
					EXPECT_SUBSCRIPTION_YIELD("shv/master", "chng");
				});
				enqueue(res, [=] (MockRpcConnection* mock) {
					EXPECT_REQUEST("shv/master/.local/history/shvjournal", "lsfiles", ls_size_true);
					RESPOND(RpcValue::List());
					DRIVER_WAIT(10);
				});
				enqueue(res, [=] (MockRpcConnection*) {
			});
			}

			DOCTEST_SUBCASE("no logs - should always sync")
			{
				enqueue(res, [=] (MockRpcConnection* mock) {
					create_dummy_cache_files(cache_dir_path, {});
					EXPECT_SUBSCRIPTION_YIELD("shv/master", "chng");
				});

				enqueue(res, [=] (MockRpcConnection* mock) {
					EXPECT_REQUEST("shv/master/.local/history/shvjournal", "lsfiles", ls_size_true);
					RESPOND(RpcValue::List());
					DRIVER_WAIT(10);
				});
				enqueue(res, [=] (MockRpcConnection*) {
				});
			}
		}

		DOCTEST_SUBCASE("more sites")
		{
			enqueue(res, [=] (MockRpcConnection* mock) {
				SEND_SITES_YIELD(mock_sites::two_devices);
			});
			enqueue(res, [=] (MockRpcConnection* mock) {
				EXPECT_SUBSCRIPTION_YIELD("shv", "mntchng");
			});
			enqueue(res, [=] (MockRpcConnection* mock) {
				EXPECT_SUBSCRIPTION_YIELD("shv/one", "chng");
			});
			enqueue(res, [=] (MockRpcConnection* mock) {
				EXPECT_SUBSCRIPTION_YIELD("shv/two", "chng");
			});

			enqueue(res, [=] (MockRpcConnection* mock) {
				EXPECT_REQUEST("shv/one/.app/shvjournal", "lsfiles", ls_size_true);
				RESPOND_YIELD(RpcValue::List());
			});
			enqueue(res, [=] (MockRpcConnection* mock) {
				EXPECT_REQUEST("shv/two/.app/shvjournal", "lsfiles", ls_size_true);
				RESPOND_YIELD(RpcValue::List());
			});
			enqueue(res, [=] (MockRpcConnection* mock) {
				EXPECT_REQUEST("shv/one/.app/shvjournal", "lsfiles", ls_size_true);
				RESPOND(RpcValue::List());
				DRIVER_WAIT(10);
			});
			enqueue(res, [=] (MockRpcConnection*) {
			});

		}

		DOCTEST_SUBCASE("slave HP shouldn't sync")
		{
			enqueue(res, [=] (MockRpcConnection* mock) {
				SEND_SITES_YIELD(mock_sites::pushlog_hp);
			});
			enqueue(res, [=] (MockRpcConnection* mock) {
				EXPECT_SUBSCRIPTION("shv", "mntchng");
				DRIVER_WAIT(10);
			});
			enqueue(res, [=] (MockRpcConnection* mock) {
				// Slave pushlog shouldn't sync. We should not have any messages here.
				CAPTURE(mock->m_messageQueue.head());
				REQUIRE(mock->m_messageQueue.empty());
			});
		}
	}

	DOCTEST_SUBCASE("master HP should sync on mntchng")
	{
		enqueue(res, [=] (MockRpcConnection* mock) {
			SEND_SITES_YIELD(mock_sites::master_hp_with_slave_pushlog);
		});
		enqueue(res, [=] (MockRpcConnection* mock) {
			EXPECT_SUBSCRIPTION_YIELD("shv", "mntchng");
		});
		enqueue(res, [=] (MockRpcConnection* mock) {
			EXPECT_SUBSCRIPTION("shv/master", "chng");
			NOTIFY_YIELD("shv/master", "mntchng", true);
		});
		enqueue(res, [=] (MockRpcConnection* mock) {
			EXPECT_REQUEST("shv/master/.local/history/shvjournal", "lsfiles", ls_size_true);
			RESPOND_YIELD(RpcValue::List()); // We only test if the syncLog triggers.
		});
		enqueue(res, [=] (MockRpcConnection* mock) {
			EXPECT_SIGNAL("shv/master", "logReset", RpcValue());
		});
	}

	DOCTEST_SUBCASE("slave HP shouldn't sync on mntchng")
	{
		enqueue(res, [=] (MockRpcConnection* mock) {
			SEND_SITES_YIELD(mock_sites::pushlog_hp);
		});
		enqueue(res, [=] (MockRpcConnection* mock) {
			EXPECT_SUBSCRIPTION("shv", "mntchng");
			NOTIFY("shv/pushlog", "mntchng", true);
			DRIVER_WAIT(10);
		});

		enqueue(res, [=] (MockRpcConnection* mock) {
			CAPTURE(mock->m_messageQueue.head());
			REQUIRE(mock->m_messageQueue.empty());
		});
	}

	return res;
}

TEST_HISTORYPROVIDER_MAIN("pushlog")
