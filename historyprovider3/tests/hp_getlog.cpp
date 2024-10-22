#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "mockrpcconnection.h"
#include "pretty_printers.h"
#include <doctest/doctest.h>

#include "src/appclioptions.h"
#include "src/historyapp.h"
#include "tests/sites.h"
#include "tests/utils.h"

#include <shv/core/utils/shvlogrpcvaluereader.h>

QQueue<std::function<CallNext(MockRpcConnection*)>> setup_test()
{
	QQueue<std::function<CallNext(MockRpcConnection*)>> res;

	enqueue(res, [=] (MockRpcConnection*) {
		// We want the test to download everything regardless of age. Ten years ought to be enough.
		HistoryApp::instance()->cliOptions()->setCacheInitMaxAge(60 /*seconds*/ * 60 /*minutes*/ * 24 /*hours*/ * 365 /*days*/ * 10 /*years*/);
		return CallNext::Yes;
	});
	std::string cache_dir_path;
	auto getlog_node_shvpath = std::make_shared<std::string>();
	std::string sub_path;

	std::vector<std::string> expected_timestamps;
	shv::core::utils::ShvGetLogParams get_log_params;

	DOCTEST_SUBCASE("slave broker")
	{
		cache_dir_path = "eyas/opc";
		*getlog_node_shvpath = cache_dir_path;
		enqueue(res, [=] (MockRpcConnection* mock) {
			SEND_SITES_YIELD(mock_sites::fin_slave_broker);
		});
		sub_path = "shv/eyas/opc";
		enqueue(res, [=] (MockRpcConnection* mock) {
			DISABLE_TYPEINFO(cache_dir_path);
		});
		enqueue(res, [=] (MockRpcConnection* mock) {
			DISABLE_TYPEINFO("eyas/with_app_history");
		});
		enqueue(res, [=] (MockRpcConnection* mock) {
			EXPECT_SUBSCRIPTION_YIELD("shv", "mntchng");
		});
		enqueue(res, [=] (MockRpcConnection* mock) {
			EXPECT_SUBSCRIPTION_YIELD(sub_path, "chng");
		});
		enqueue(res, [=] (MockRpcConnection* mock) {
			EXPECT_SUBSCRIPTION_YIELD(sub_path, "cmdlog");
		});
		enqueue(res, [=] (MockRpcConnection* mock) {
			EXPECT_SUBSCRIPTION_YIELD("shv/eyas/with_app_history", "chng");
		});
		enqueue(res, [=] (MockRpcConnection* mock) {
			EXPECT_SUBSCRIPTION("shv/eyas/with_app_history", "cmdlog");

			create_dummy_cache_files(cache_dir_path, {
				{ "2022-07-07T18-06-15-557.log2", dummy_logfile },
				{ "2022-07-07T18-06-17-872.log2", dummy_logfile2 },
				{ "dirtylog", dummy_logfile3 },
			});
			return CallNext::Yes;
		});

		DOCTEST_SUBCASE("default params")
		{
			expected_timestamps = {
				"2022-07-07T18:06:15.557Z",
				"2022-07-07T18:06:17.784Z",
				"2022-07-07T18:06:17.784Z",
				"2022-07-07T18:06:17.869Z",
				"2022-07-07T18:06:17.872Z",
				"2022-07-07T18:06:17.874Z",
				"2022-07-07T18:06:17.880Z",
				"2022-07-07T18:06:20.900Z",
			};
		}

		DOCTEST_SUBCASE("since")
		{
			DOCTEST_SUBCASE("empty dir")
			{
				enqueue(res, [=] (MockRpcConnection*) {
					remove_cache_contents(cache_dir_path);
					return CallNext::Yes;
				});
				expected_timestamps = {};
			}

			DOCTEST_SUBCASE("non-empty dir")
			{
				expected_timestamps = {
					"2022-07-07T18:06:17.874Z",
					"2022-07-07T18:06:17.880Z",
					"2022-07-07T18:06:20.900Z",
				};
			}

			get_log_params.since = RpcValue::DateTime::fromUtcString("2022-07-07T18:06:17.872Z");
		}

		DOCTEST_SUBCASE("until")
		{
			expected_timestamps = {
				"2022-07-07T18:06:15.557Z",
				"2022-07-07T18:06:17.784Z",
				"2022-07-07T18:06:17.784Z",
				"2022-07-07T18:06:17.869Z",
			};
			get_log_params.until = RpcValue::DateTime::fromUtcString("2022-07-07T18:06:17.872Z");
		}

		DOCTEST_SUBCASE("since last correctly selects files and doesn't crash")
		{
			get_log_params.since = "last";
			get_log_params.withSnapshot = true;

			DOCTEST_SUBCASE("empty dir")
			{
				enqueue(res, [=] (MockRpcConnection*) {
					remove_cache_contents(cache_dir_path);
					return CallNext::Yes;
				});
			}

			DOCTEST_SUBCASE("just dirtylog")
			{
				enqueue(res, [=] (MockRpcConnection*) {
					create_dummy_cache_files(cache_dir_path, {
						{ "dirtylog", dummy_logfile3 },
					});
					return CallNext::Yes;
				});
				expected_timestamps = {
					"2022-07-07T18:06:20.900Z",
				};
			}

			DOCTEST_SUBCASE("just a log file")
			{
				enqueue(res, [=] (MockRpcConnection*) {
					create_dummy_cache_files(cache_dir_path, {
						{ "2022-07-07T18-06-17-872.log2", dummy_logfile2 },
					});
					return CallNext::Yes;
				});

				// Note: the timestamps are correct, sinceLast will set all result timestamps to the latest one.
				expected_timestamps = {
					"2022-07-07T18:06:17.880Z",
					"2022-07-07T18:06:17.880Z",
					"2022-07-07T18:06:17.880Z",
				};
			}

			DOCTEST_SUBCASE("dirty log and log file")
			{
				enqueue(res, [=] (MockRpcConnection*) {
					create_dummy_cache_files(cache_dir_path, {
						{ "2022-07-07T18-06-17-872.log2", dummy_logfile2 },
							{ "dirtylog", dummy_logfile3 },
					});
					return CallNext::Yes;
				});
				expected_timestamps = {
					"2022-07-07T18:06:20.900Z",
					"2022-07-07T18:06:20.900Z",
					"2022-07-07T18:06:20.900Z",
				};
			}
		}
	}

	DOCTEST_SUBCASE("master broker")
	{
		cache_dir_path = "fin/hel/tram/hel002";
		*getlog_node_shvpath = "fin/hel/tram/hel002/eyas/opc";
		enqueue(res, [=] (MockRpcConnection* mock) {
			SEND_SITES_YIELD(mock_sites::fin_master_broker);
		});
		sub_path = "shv/fin/hel/tram/hel002";
		enqueue(res, [=] (MockRpcConnection* mock) {
			DISABLE_TYPEINFO(*getlog_node_shvpath);
		});
		enqueue(res, [=] (MockRpcConnection* mock) {
			EXPECT_SUBSCRIPTION_YIELD("shv", "mntchng");
		});
		enqueue(res, [=] (MockRpcConnection* mock) {
			EXPECT_SUBSCRIPTION_YIELD(sub_path, "chng");
		});
		enqueue(res, [=] (MockRpcConnection* mock) {
			EXPECT_SUBSCRIPTION(sub_path, "cmdlog");

			create_dummy_cache_files(cache_dir_path, {
				{ "eyas/opc/2022-07-07T18-06-15-557.log2", dummy_logfile },
				{ "eyas/opc/2022-07-07T18-06-17-872.log2", dummy_logfile2 },
				{ "eyas/opc/dirtylog", dummy_logfile3 },
			});
			return CallNext::Yes;
		});

		DOCTEST_SUBCASE("default params")
		{
			expected_timestamps = {
				"2022-07-07T18:06:15.557Z",
				"2022-07-07T18:06:17.784Z",
				"2022-07-07T18:06:17.784Z",
				"2022-07-07T18:06:17.869Z",
				"2022-07-07T18:06:17.872Z",
				"2022-07-07T18:06:17.874Z",
				"2022-07-07T18:06:17.880Z",
				"2022-07-07T18:06:20.900Z",
			};
		}
	}

	enqueue(res, [=] (MockRpcConnection* mock) {
		REQUEST_YIELD(*getlog_node_shvpath, "getLog", get_log_params.toRpcValue());
	});

	enqueue(res, [=] (MockRpcConnection* mock) {
		// For now,I'll only make a simple test: I'll assume that if the timestamps are correct, everything else is also
		// correct. This is not the place to test the log uitilities anyway.
		REQUIRE(mock->m_messageQueue.head().isResponse());
		std::vector<std::string> actual_timestamps;
		shv::core::utils::ShvLogRpcValueReader entries(shv::chainpack::RpcResponse(mock->m_messageQueue.head()).result());
		while (entries.next()) {
			actual_timestamps.push_back(RpcValue::DateTime::fromMSecsSinceEpoch(entries.entry().epochMsec).toIsoString());
		}

		REQUIRE(actual_timestamps == expected_timestamps);
		mock->m_messageQueue.dequeue();
	});

	return res;
}

TEST_HISTORYPROVIDER_MAIN("hp_getlog")
