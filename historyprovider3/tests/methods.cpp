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
	DOCTEST_SUBCASE("fin slave HP")
	{
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
						RpcValue::List{ "dirtylog", 101UL }
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

	DOCTEST_SUBCASE("syncing from slave HP")
	{
		RpcValue::List expected_cache_contents;
		std::string cache_dir_path = "fin/hel/tram/hel002";
		SEND_SITES_YIELD(mock_sites::fin_master_broker);
		EXPECT_SUBSCRIPTION_YIELD("shv/fin/hel/tram/hel002", "mntchng");
		EXPECT_SUBSCRIPTION("shv/fin/hel/tram/hel002", "chng");
		std::string master_shv_journal_path = "_shvjournal";
		std::string slave_shv_journal_path = "shv/fin/hel/tram/hel002/.local/history/shvjournal";
		REQUEST_YIELD(master_shv_journal_path, "syncLog", RpcValue());
		EXPECT_REQUEST(slave_shv_journal_path, "lsfiles", ls_size_true);

		DOCTEST_SUBCASE("Remote - has files, local - empty")
		{
			// NOTE: I'm only really testing whether historyprovider correctly enters the .local broker.
			create_dummy_cache_files(cache_dir_path, {});
			expected_cache_contents = RpcValue::List({{
				RpcValue::List{ "2022-07-07T18-06-15-557.log2", dummy_logfile.size() }
			}});
			RESPOND_YIELD((RpcValue::List{{
				{ "2022-07-07T18-06-15-557.log2", "f", dummy_logfile.size() }
			}}));

			EXPECT_REQUEST(join(slave_shv_journal_path, "2022-07-07T18-06-15-557.log2"), "read", read_offset_0);
			RESPOND_YIELD(RpcValue::stringToBlob(dummy_logfile));
		}

		EXPECT_RESPONSE("All files have been synced");
		REQUIRE(get_cache_contents(cache_dir_path) == expected_cache_contents);
	}

	DOCTEST_SUBCASE("getLog")
	{
		std::string cache_dir_path = "eyas/opc";
		SEND_SITES_YIELD(mock_sites::fin_slave_broker);
		std::string sub_path = "shv/eyas/opc";
		EXPECT_SUBSCRIPTION_YIELD(sub_path, "mntchng");
		EXPECT_SUBSCRIPTION(sub_path, "chng");

		create_dummy_cache_files(cache_dir_path, {
			{ "2022-07-07T18-06-15-557.log2", dummy_logfile },
			{ "2022-07-07T18-06-15-558.log2", dummy_logfile2 },
			{ "dirtylog", dummy_logfile },
		});

		std::vector<int64_t> expected_timestamps;
		shv::core::utils::ShvGetLogParams get_log_params;

		DOCTEST_SUBCASE("default params")
		{
			expected_timestamps = {1657217175557, 1657217177784, 1657217177784, 1657217177869, 1657217177872, 1657217177874, 1657217177880};
		}

		DOCTEST_SUBCASE("since")
		{
			expected_timestamps = {1657217177872, 1657217177874, 1657217177880};
			get_log_params.since = RpcValue::DateTime::fromMSecsSinceEpoch(1657217177872);
		}

		DOCTEST_SUBCASE("until")
		{
			expected_timestamps = {1657217175557, 1657217177784, 1657217177784, 1657217177869};
			get_log_params.until = RpcValue::DateTime::fromMSecsSinceEpoch(1657217177870);
		}

		REQUEST_YIELD(cache_dir_path, "getLog", get_log_params.toRpcValue());

		// For now,I'll only make a simple test: I'll assume that if the timestamps are correct, everything else is also
		// correct. This is not the place to test the log uitilities anyway.
		REQUIRE(m_messageQueue.head().isResponse());
		std::vector<int64_t> actual_timestamps;
		shv::core::utils::ShvMemoryJournal entries;
		entries.loadLog(shv::chainpack::RpcResponse(m_messageQueue.head()).result());
		for (const auto& entry : entries.entries()) {
			actual_timestamps.push_back(entry.epochMsec);
		}

		REQUIRE(actual_timestamps == expected_timestamps);
		m_messageQueue.dequeue();
	}

	DOCTEST_SUBCASE("pushLog")
	{
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

	DOCTEST_SUBCASE("legacy getLog retrieval")
	{
		std::string shv_path = "shv/legacy";
		std::string cache_dir_path = "legacy";
		SEND_SITES_YIELD(mock_sites::legacy_hp);
		EXPECT_SUBSCRIPTION_YIELD(shv_path, "mntchng");
		EXPECT_SUBSCRIPTION(shv_path, "chng");

		RpcValue::List expected_cache_contents;

		DOCTEST_SUBCASE("empty response")
		{
			create_dummy_cache_files(cache_dir_path, {});
			REQUEST_YIELD("_shvjournal", "syncLog", RpcValue());
			EXPECT_REQUEST(shv_path, "getLog");
			RESPOND_YIELD(shv::chainpack::RpcValue::List());
		}

		DOCTEST_SUBCASE("some response")
		{
			create_dummy_cache_files(cache_dir_path, {});
			REQUEST_YIELD("_shvjournal", "syncLog", RpcValue());
			EXPECT_REQUEST(shv_path, "getLog");
			RESPOND_YIELD(dummy_getlog_response);
			expected_cache_contents = RpcValue::List({{
				RpcValue::List{ "2022-07-07T18-06-15-557.log2", 201UL }
			}});
		}

		enum class WithSnapshot {
			True,
			False
		};

		auto create_get_log_options = [] (const RpcValue& since, WithSnapshot with_snapshot) -> RpcValue {
			auto res = RpcValue::fromCpon(R"({"headerOptions":11u,"maxRecordCount":5000,"recordCountLimit":5000,"withPathsDict":true,"withTypeInfo":false} == null)").asMap();
			res.setValue("withSnapshot", with_snapshot == WithSnapshot::True ? true : false);
			res.setValue("since", since);
			return res;
		};

		DOCTEST_SUBCASE("hp retrieves snapshot correctly")
		{
			create_dummy_cache_files(cache_dir_path, {
				{ "2022-07-07T18-06-15-551.log2", QString::fromStdString(dummy_logfile).repeated(50000).toStdString() },
			});
			REQUEST_YIELD("_shvjournal", "syncLog", RpcValue());
			EXPECT_REQUEST(shv_path, "getLog", create_get_log_options(RpcValue::fromCpon(R"(d"2022-07-07T18:06:17.870Z")"), WithSnapshot::True));
			RESPOND_YIELD(five_thousand_records_getlog_response);
			expected_cache_contents = RpcValue::List({{
				RpcValue::List{ "2022-07-07T18-06-15-551.log2", 15400000UL },
				RpcValue::List{ "2022-07-07T18-06-15-557.log2", 201L },
			}});
			EXPECT_REQUEST(shv_path, "getLog", create_get_log_options(RpcValue::fromCpon(R"(d"2022-07-07T18:06:15.557Z")"), WithSnapshot::False));
			RESPOND_YIELD(dummy_getlog_response);
		}

		DOCTEST_SUBCASE("appending to files")
		{
			create_dummy_cache_files(cache_dir_path, {
				{ "2022-07-07T18-06-15-557.log2", dummy_logfile },
			});
			REQUEST_YIELD("_shvjournal", "syncLog", RpcValue());
			EXPECT_REQUEST(shv_path, "getLog", create_get_log_options(RpcValue::fromCpon(R"(d"2022-07-07T18:06:17.870Z")"), WithSnapshot::False));
			RESPOND_YIELD(dummy_getlog_response);
			expected_cache_contents = RpcValue::List({{
				RpcValue::List{ "2022-07-07T18-06-15-557.log2", 509UL }
			}});
		}

		DOCTEST_SUBCASE("since param")
		{
			HistoryApp::instance()->cliOptions()->setCacheInitMaxAge(60 /*seconds*/ * 60 /*minutes*/ * 24 /*hours*/ * 30 /*days*/);
			DOCTEST_SUBCASE("empty cache")
			{
				DOCTEST_SUBCASE("no files")
				{
					create_dummy_cache_files(cache_dir_path, {});
				}

				DOCTEST_SUBCASE("one empty files")
				{
					create_dummy_cache_files(cache_dir_path, {
						{ "2022-07-07T18-06-15-557.log2", "" },
					});
					expected_cache_contents = RpcValue::List({{
						RpcValue::List{ "2022-07-07T18-06-15-557.log2", 0UL }
					}});
				}
				REQUEST_YIELD("_shvjournal", "syncLog", RpcValue());
				EXPECT_REQUEST(shv_path, "getLog");
				auto since_param_ms = shv::chainpack::RpcRequest(m_messageQueue.head()).params().asMap().value("since").toDateTime().msecsSinceEpoch();
				auto now_ms = shv::chainpack::RpcValue::DateTime::now().msecsSinceEpoch();
				REQUIRE(now_ms - since_param_ms < int64_t{1000} /*ms*/ * 60 /*seconds*/ * 60 /*minutes*/ * 24 /*hours*/ * 31 /*days*/ );
			}

			DOCTEST_SUBCASE("something in cache")
			{
				create_dummy_cache_files(cache_dir_path, {
					{ "2022-07-07T18-06-15-557.log2", dummy_logfile },
				});
				REQUEST_YIELD("_shvjournal", "syncLog", RpcValue());
				EXPECT_REQUEST(shv_path, "getLog");
				auto since_param_ms = shv::chainpack::RpcRequest(m_messageQueue.head()).params().asMap().value("since").toDateTime().msecsSinceEpoch();
				REQUIRE(since_param_ms == shv::chainpack::RpcValue::DateTime::fromUtcString("2022-07-07T18:06:17.870Z").msecsSinceEpoch());
				expected_cache_contents = RpcValue::List({{
					RpcValue::List{ "2022-07-07T18-06-15-557.log2", 308UL }
				}});
			}

			DOCTEST_SUBCASE("only dirty in cache")
			{
				create_dummy_cache_files(cache_dir_path, {
					{ "dirtylog", dummy_logfile },
				});
				REQUEST_YIELD("_shvjournal", "syncLog", RpcValue());
				EXPECT_REQUEST(shv_path, "getLog");
				auto since_param_ms = shv::chainpack::RpcRequest(m_messageQueue.head()).params().asMap().value("since").toDateTime().msecsSinceEpoch();
				auto now_ms = shv::chainpack::RpcValue::DateTime::now().msecsSinceEpoch();
				REQUIRE(now_ms - since_param_ms < int64_t{1000} /*ms*/ * 60 /*seconds*/ * 60 /*minutes*/ * 24 /*hours*/ * 31 /*days*/ );
				expected_cache_contents = RpcValue::List({{
					RpcValue::List{ "dirtylog", 308UL }
				}});
			}

			RESPOND_YIELD(shv::chainpack::RpcValue::List());
		}

		EXPECT_RESPONSE("All files have been synced");
		REQUIRE(get_cache_contents(cache_dir_path) == expected_cache_contents);
	}

	DOCTEST_SUBCASE("sanitizer")
	{
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

	DOCTEST_SUBCASE("dirty log trim")
	{
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

	co_return;
}

TEST_HISTORYPROVIDER_MAIN("HistoryApp")