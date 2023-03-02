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
	enqueue(res, [=] (MockRpcConnection* mock) {
		HistoryApp::instance()->cliOptions()->setCacheInitMaxAge(60 /*seconds*/ * 60 /*minutes*/ * 24 /*hours*/ * 365 /*days*/ * 10 /*years*/);
		SEND_SITES_YIELD(mock_sites::legacy_hp);
	});

	std::string shv_path = "shv/legacy";

	enqueue(res, [=] (MockRpcConnection* mock) {
		EXPECT_SUBSCRIPTION_YIELD("shv", "mntchng");
	});
	enqueue(res, [=] (MockRpcConnection* mock) {
		EXPECT_SUBSCRIPTION(shv_path, "chng");
		return CallNext::Yes;
	});

	std::string cache_dir_path = "legacy";
	auto expected_cache_contents = std::make_shared<RpcValue::List>();

	DOCTEST_SUBCASE("empty response")
	{
		enqueue(res, [=] (MockRpcConnection* mock) {
			create_dummy_cache_files(cache_dir_path, {});
			REQUEST_YIELD("_shvjournal", "syncLog", RpcValue());
		});
		enqueue(res, [=] (MockRpcConnection* mock) {
			EXPECT_REQUEST(shv_path, "getLog");
			RESPOND_YIELD(shv::chainpack::RpcValue::List());
		});
	}

	DOCTEST_SUBCASE("some response")
	{
		enqueue(res, [=] (MockRpcConnection* mock) {
			create_dummy_cache_files(cache_dir_path, {});
			REQUEST_YIELD("_shvjournal", "syncLog", RpcValue());
		});
		enqueue(res, [=] (MockRpcConnection* mock) {
			EXPECT_REQUEST(shv_path, "getLog");
			*expected_cache_contents = RpcValue::List({{
				RpcValue::List{ "2022-07-07T18-06-15-557.log2", 201UL }
			}});
			RESPOND_YIELD(dummy_getlog_response);
		});
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
		enqueue(res, [=] (MockRpcConnection* mock) {
			create_dummy_cache_files(cache_dir_path, {
				{ "2022-07-07T18-06-15-551.log2", QString::fromStdString(dummy_logfile).repeated(50000).toStdString() },
			});
			REQUEST_YIELD("_shvjournal", "syncLog", RpcValue());
		});
		enqueue(res, [=] (MockRpcConnection* mock) {
			EXPECT_REQUEST(shv_path, "getLog", create_get_log_options(RpcValue::fromCpon(R"(d"2022-07-07T18:06:17.870Z")"), WithSnapshot::True));
			*expected_cache_contents = RpcValue::List({{
				RpcValue::List{ "2022-07-07T18-06-15-551.log2", 15400000UL },
				RpcValue::List{ "2022-07-07T18-06-15-557.log2", 201L },
			}});
			RESPOND_YIELD(five_thousand_records_getlog_response);
		});
		enqueue(res, [=] (MockRpcConnection* mock) {
			EXPECT_REQUEST(shv_path, "getLog", create_get_log_options(RpcValue::fromCpon(R"(d"2022-07-07T18:06:15.557Z")"), WithSnapshot::False));
			RESPOND_YIELD(dummy_getlog_response);
		});
	}

	DOCTEST_SUBCASE("subdirectory in a log directory")
	{
		enqueue(res, [=] (MockRpcConnection* mock) {
			create_dummy_cache_files(cache_dir_path, {});
			QDir(QString::fromStdString(join(get_site_cache_dir(cache_dir_path).path().toStdString(), "blablabla"))).mkpath(".");
			REQUEST_YIELD("_shvjournal", "syncLog", RpcValue());
		});
		enqueue(res, [=] (MockRpcConnection* mock) {
			EXPECT_REQUEST(shv_path, "getLog");
			RESPOND_YIELD(shv::chainpack::RpcValue::List());
		});
	}

	DOCTEST_SUBCASE("appending to files")
	{
		enqueue(res, [=] (MockRpcConnection* mock) {
			create_dummy_cache_files(cache_dir_path, {
				{ "2022-07-07T18-06-15-557.log2", dummy_logfile },
			});
			REQUEST_YIELD("_shvjournal", "syncLog", RpcValue());
		});
		enqueue(res, [=] (MockRpcConnection* mock) {
			EXPECT_REQUEST(shv_path, "getLog", create_get_log_options(RpcValue::fromCpon(R"(d"2022-07-07T18:06:17.870Z")"), WithSnapshot::False));
			*expected_cache_contents = RpcValue::List({{
				RpcValue::List{ "2022-07-07T18-06-15-557.log2", 509UL }
			}});
			RESPOND_YIELD(dummy_getlog_response);
		});
	}

	DOCTEST_SUBCASE("since param")
	{
		enqueue(res, [=] (MockRpcConnection*) {
			HistoryApp::instance()->cliOptions()->setCacheInitMaxAge(60 /*seconds*/ * 60 /*minutes*/ * 24 /*hours*/ * 30 /*days*/);
			return CallNext::Yes;
		});
		DOCTEST_SUBCASE("empty cache")
		{
			DOCTEST_SUBCASE("no files")
			{
				enqueue(res, [=] (MockRpcConnection*) {
					create_dummy_cache_files(cache_dir_path, {});
					return CallNext::Yes;
				});
			}

			DOCTEST_SUBCASE("one empty files")
			{
				enqueue(res, [=] (MockRpcConnection*) {
					create_dummy_cache_files(cache_dir_path, {
						{ "2022-07-07T18-06-15-557.log2", "" },
					});
					*expected_cache_contents = RpcValue::List({{
						RpcValue::List{ "2022-07-07T18-06-15-557.log2", 0UL }
					}});
					return CallNext::Yes;
				});
			}
			enqueue(res, [=] (MockRpcConnection* mock) {
				REQUEST_YIELD("_shvjournal", "syncLog", RpcValue());
			});
			enqueue(res, [=] (MockRpcConnection* mock) {
				EXPECT_REQUEST(shv_path, "getLog");
				auto since_param_ms = shv::chainpack::RpcRequest(mock->m_messageQueue.head()).params().asMap().value("since").toDateTime().msecsSinceEpoch();
				auto now_ms = shv::chainpack::RpcValue::DateTime::now().msecsSinceEpoch();
				REQUIRE(now_ms - since_param_ms < int64_t{1000} /*ms*/ * 60 /*seconds*/ * 60 /*minutes*/ * 24 /*hours*/ * 31 /*days*/ );
				return CallNext::Yes;
			});
		}

		DOCTEST_SUBCASE("something in cache")
		{
			enqueue(res, [=] (MockRpcConnection* mock) {
				create_dummy_cache_files(cache_dir_path, {
					{ "2022-07-07T18-06-15-557.log2", dummy_logfile },
				});
				REQUEST_YIELD("_shvjournal", "syncLog", RpcValue());
			});
			enqueue(res, [=] (MockRpcConnection* mock) {
				EXPECT_REQUEST(shv_path, "getLog");
				auto since_param_ms = shv::chainpack::RpcRequest(mock->m_messageQueue.head()).params().asMap().value("since").toDateTime().msecsSinceEpoch();
				REQUIRE(since_param_ms == shv::chainpack::RpcValue::DateTime::fromUtcString("2022-07-07T18:06:17.870Z").msecsSinceEpoch());
				*expected_cache_contents = RpcValue::List({{
					RpcValue::List{ "2022-07-07T18-06-15-557.log2", 308UL }
				}});
				return CallNext::Yes;
			});
		}

		DOCTEST_SUBCASE("only dirty in cache")
		{
			enqueue(res, [=] (MockRpcConnection* mock) {
				create_dummy_cache_files(cache_dir_path, {
					{ "dirtylog", dummy_logfile },
				});
				REQUEST_YIELD("_shvjournal", "syncLog", RpcValue());
			});
			enqueue(res, [=] (MockRpcConnection* mock) {
				EXPECT_REQUEST(shv_path, "getLog");
				auto since_param_ms = shv::chainpack::RpcRequest(mock->m_messageQueue.head()).params().asMap().value("since").toDateTime().msecsSinceEpoch();
				auto now_ms = shv::chainpack::RpcValue::DateTime::now().msecsSinceEpoch();
				REQUIRE(now_ms - since_param_ms < int64_t{1000} /*ms*/ * 60 /*seconds*/ * 60 /*minutes*/ * 24 /*hours*/ * 31 /*days*/ );
				*expected_cache_contents = RpcValue::List({{
					RpcValue::List{ "dirtylog", 308UL }
				}});
				return CallNext::Yes;
			});
		}

		enqueue(res, [=] (MockRpcConnection* mock) {
			RESPOND_YIELD(shv::chainpack::RpcValue::List());
		});
	}

	enqueue(res, [=] (MockRpcConnection* mock) {
		EXPECT_RESPONSE("All files have been synced");
		REQUIRE(get_cache_contents(cache_dir_path) == *expected_cache_contents);
	});

	return res;
}

TEST_HISTORYPROVIDER_MAIN("legacy_getlog_retrieval")
