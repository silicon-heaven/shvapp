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
		HistoryApp::instance()->cliOptions()->setCacheInitMaxAge(60 /*seconds*/ * 60 /*minutes*/ * 24 /*hours*/ * 365 /*days*/ * 10 /*years*/);
		return CallNext::Yes;
	});

	auto expected_cache_contents = std::make_shared<RpcValue::List>();
	DOCTEST_SUBCASE("sanitizeLog correctly removes stuff")
	{
		std::string shv_path = "shv/eyas/opc";
		std::string cache_dir_path = "eyas/opc";
		enqueue(res, [=] (MockRpcConnection* mock) {
			SEND_SITES_YIELD(mock_sites::fin_slave_broker);
		});
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
			EXPECT_SUBSCRIPTION_YIELD(shv_path, "chng");
		});
		enqueue(res, [=] (MockRpcConnection* mock) {
			EXPECT_SUBSCRIPTION_YIELD(shv_path, "cmdlog");
		});
		enqueue(res, [=] (MockRpcConnection* mock) {
			EXPECT_SUBSCRIPTION_YIELD("shv/eyas/with_app_history", "chng");
		});
		enqueue(res, [=] (MockRpcConnection* mock) {
			EXPECT_SUBSCRIPTION("shv/eyas/with_app_history", "cmdlog");

			create_dummy_cache_files("", {
				{ "opc/2022-07-07T18-06-15-557.log2", dummy_logfile },
				{ "opc/2022-07-07T18-06-15-560.log2", dummy_logfile },
			});
			return CallNext::Yes;
		});

		DOCTEST_SUBCASE("when size is within quota")
		{
			enqueue(res, [=] (MockRpcConnection*) {
				HistoryApp::instance()->setTotalCacheSizeLimit(5000);
				*expected_cache_contents = RpcValue::List({{
					RpcValue::List{ "opc/2022-07-07T18-06-15-557.log2", 308UL },
					RpcValue::List{ "opc/2022-07-07T18-06-15-560.log2", 308UL }
				}});
				return CallNext::Yes;
			});
		}

		DOCTEST_SUBCASE("when size is over quota")
		{
			enqueue(res, [=] (MockRpcConnection*) {
				HistoryApp::instance()->setTotalCacheSizeLimit(500);
				*expected_cache_contents = RpcValue::List({{
					RpcValue::List{ "opc/2022-07-07T18-06-15-560.log2", 308UL }
				}});
				return CallNext::Yes;
			});
		}

		enqueue(res, [=] (MockRpcConnection* mock) {
			REQUEST_YIELD("_shvjournal", "sanitizeLog", RpcValue());
		});
		enqueue(res, [=] (MockRpcConnection* mock) {
			EXPECT_SIGNAL("_shvjournal", "cmdlog");
		});
		enqueue(res, [=] (MockRpcConnection* mock) {
			EXPECT_RESPONSE("Cache sanitization done");
			REQUIRE(get_cache_contents("") == *expected_cache_contents);
		});
	}

	DOCTEST_SUBCASE("sanitizer runs periodically")
	{
		enqueue(res, [=] (MockRpcConnection* mock) {
			HistoryApp::instance()->cliOptions()->setJournalSanitizerInterval(1);
			SEND_SITES_YIELD(mock_sites::two_devices);
		});
		enqueue(res, [=] (MockRpcConnection* mock) {
			DISABLE_TYPEINFO("one");
		});
		enqueue(res, [=] (MockRpcConnection* mock) {
			DISABLE_TYPEINFO("two");
		});
		enqueue(res, [=] (MockRpcConnection* mock) {
			EXPECT_SUBSCRIPTION_YIELD("shv", "mntchng");
		});
		enqueue(res, [=] (MockRpcConnection* mock) {
			EXPECT_SUBSCRIPTION_YIELD("shv/one", "chng");
		});
		enqueue(res, [=] (MockRpcConnection* mock) {
			EXPECT_SUBSCRIPTION_YIELD("shv/one", "cmdlog");
		});
		enqueue(res, [=] (MockRpcConnection* mock) {
			EXPECT_SUBSCRIPTION_YIELD("shv/two", "chng");
		});
		enqueue(res, [=] (MockRpcConnection* mock) {
			EXPECT_SUBSCRIPTION("shv/two", "cmdlog");
			HistoryApp::instance()->setTotalCacheSizeLimit(800);

			create_dummy_cache_files("", {
				{ "one/2022-07-07T18-06-15-557.log2", dummy_logfile },
				{ "one/2022-07-07T18-06-15-560.log2", dummy_logfile },
				{ "two/2022-07-07T18-06-15-557.log2", dummy_logfile },
				{ "two/2022-07-07T18-06-15-560.log2", dummy_logfile },
			});
			*expected_cache_contents = RpcValue::List({{
				RpcValue::List{ "one/2022-07-07T18-06-15-560.log2", 308UL },
				RpcValue::List{ "two/2022-07-07T18-06-15-560.log2", 308UL }
			}});
			DRIVER_WAIT(3000);
		});


		enqueue(res, [=] (MockRpcConnection*) {
			REQUIRE(get_cache_contents("") == *expected_cache_contents);
		});
	}
	return res;
}

TEST_HISTORYPROVIDER_MAIN("sanitizer")
