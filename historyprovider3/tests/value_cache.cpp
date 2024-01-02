#include <iostream>
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "mockrpcconnection.h"
#include "pretty_printers.h"
#include <doctest/doctest.h>

#include "src/appclioptions.h"
#include "src/historyapp.h"
#include "tests/sites.h"
#include "tests/utils.h"

namespace cp = shv::chainpack;
using cp::RpcValue;

QQueue<std::function<CallNext(MockRpcConnection*)>> setup_test()
{
	QQueue<std::function<CallNext(MockRpcConnection*)>> res;
	std::string cache_dir_path = "eyas/opc";
	enqueue(res, [=] (MockRpcConnection* mock) {
		SEND_SITES_YIELD(mock_sites::fin_slave_broker);
	});
	enqueue(res, [=] (MockRpcConnection* mock) {
		EXPECT_SUBSCRIPTION_YIELD("shv", "mntchng");
	});
	enqueue(res, [=] (MockRpcConnection* mock) {
		EXPECT_SUBSCRIPTION_YIELD("shv/eyas/opc", "chng");
	});
	enqueue(res, [=] (MockRpcConnection* mock) {
		EXPECT_SUBSCRIPTION_YIELD("shv/eyas/opc", "cmdlog");
	});
	enqueue(res, [=] (MockRpcConnection* mock) {
		EXPECT_SUBSCRIPTION_YIELD("shv/eyas/with_app_history", "chng");
	});
	enqueue(res, [=] (MockRpcConnection* mock) {
		EXPECT_SUBSCRIPTION("shv/eyas/with_app_history", "cmdlog");
		NOTIFY("shv/eyas/opc/cached_status", "chng", std::string{"changed_value"});
		return CallNext::Yes;
	});

	DOCTEST_SUBCASE("value is in cache")
	{
		enqueue(res, [=] (MockRpcConnection* mock) {
			REQUEST_YIELD("_valuecache", "get", std::string{"shv/eyas/opc/cached_status"});
		});
		enqueue(res, [=] (MockRpcConnection* mock) {
			EXPECT_SIGNAL("_valuecache", "cmdlog");
		});
		enqueue(res, [=] (MockRpcConnection* mock) {
			EXPECT_RESPONSE("changed_value");

			// mntchng removes it from the cache.
			NOTIFY_YIELD("shv/eyas", "mntchng", true);
		});

		// Have to handle the log resets here.
		enqueue(res, [=] (MockRpcConnection* mock) {
			EXPECT_REQUEST("shv/eyas/opc/.app/shvjournal", "lsfiles");
			RESPOND_YIELD(RpcValue::List{});
		});
		enqueue(res, [=] (MockRpcConnection* mock) {
			EXPECT_REQUEST("shv/eyas/with_app_history/.app/history", "lsfiles");
			RESPOND_YIELD(RpcValue::List{});
		});
		enqueue(res, [=] (MockRpcConnection* mock) {
			EXPECT_SIGNAL("shv/eyas/opc", "logReset");
			EXPECT_SIGNAL("shv/eyas/with_app_history", "logReset");
		});

		// Now it shouldn't be cached anymore.
		enqueue(res, [=] (MockRpcConnection* mock) {
			REQUEST_YIELD("_valuecache", "get", std::string{"shv/eyas/opc/cached_status"});
		});
		enqueue(res, [=] (MockRpcConnection* mock) {
			EXPECT_SIGNAL("_valuecache", "cmdlog");
		});
		enqueue(res, [=] (MockRpcConnection* mock) {
			EXPECT_REQUEST("shv/eyas/opc/cached_status", "get");
			RESPOND_YIELD("new_value");
		});
		enqueue(res, [=] (MockRpcConnection* mock) {
			EXPECT_RESPONSE("new_value");

			// mntchng somewhere else doesn't remove the cache.
			NOTIFY_YIELD("shv/eyas/with_app_history", "mntchng", true);
		});

		// Have to handle the log resets here.
		enqueue(res, [=] (MockRpcConnection* mock) {
			EXPECT_REQUEST("shv/eyas/with_app_history/.app/history", "lsfiles");
			RESPOND_YIELD(RpcValue::List{});
		});
		enqueue(res, [=] (MockRpcConnection* mock) {
			EXPECT_SIGNAL("shv/eyas/with_app_history", "logReset");
			REQUEST_YIELD("_valuecache", "get", std::string{"shv/eyas/opc/cached_status"});
		});
		enqueue(res, [=] (MockRpcConnection* mock) {
			EXPECT_SIGNAL("_valuecache", "cmdlog");
		});
		enqueue(res, [=] (MockRpcConnection* mock) {
			EXPECT_RESPONSE("new_value");
		});
	}

	DOCTEST_SUBCASE("value is not in cache")
	{
		enqueue(res, [=] (MockRpcConnection* mock) {
			REQUEST_YIELD("_valuecache", "get", std::string{"shv/eyas/opc/not_cached"});
		});
		enqueue(res, [=] (MockRpcConnection* mock) {
			EXPECT_SIGNAL("_valuecache", "cmdlog");
		});
		enqueue(res, [=] (MockRpcConnection* mock) {
			EXPECT_REQUEST("shv/eyas/opc/not_cached", "get");
			RESPOND_YIELD("value_for_not_cached");
		});
		enqueue(res, [=] (MockRpcConnection* mock) {
			EXPECT_RESPONSE("value_for_not_cached");
			// Now it should be cached.
			REQUEST_YIELD("_valuecache", "get", std::string{"shv/eyas/opc/not_cached"});
		});

		enqueue(res, [=] (MockRpcConnection* mock) {
			EXPECT_SIGNAL("_valuecache", "cmdlog");
		});
		enqueue(res, [=] (MockRpcConnection* mock) {
			EXPECT_RESPONSE("value_for_not_cached");
		});
	}
	return res;
}

TEST_HISTORYPROVIDER_MAIN("value_cache")
