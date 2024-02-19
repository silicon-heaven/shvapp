#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "mockrpcconnection.h"
#include "pretty_printers.h"
#include <doctest/doctest.h>

#include "src/appclioptions.h"
#include "src/historyapp.h"
#include "tests/sites.h"
#include "tests/utils.h"

#include <shv/core/utils/shvjournalentry.h>
#include <shv/core/utils/shvmemoryjournal.h>

#include <QDir>


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
		EXPECT_SUBSCRIPTION("shv/eyas/opc", "chng");
	});
	enqueue(res, [=] (MockRpcConnection* mock) {
		EXPECT_SUBSCRIPTION("shv/eyas/opc", "cmdlog");
	});
	enqueue(res, [=] (MockRpcConnection* mock) {
		EXPECT_SUBSCRIPTION("shv/eyas/with_app_history", "chng");
	});
	enqueue(res, [=] (MockRpcConnection* mock) {
		EXPECT_SUBSCRIPTION("shv/eyas/with_app_history", "cmdlog");
		return CallNext::Yes;
	});

	DOCTEST_SUBCASE("node tree gets reinitialized")
	{
		enqueue(res, [=] (MockRpcConnection* mock) {
			REQUEST_YIELD(cache_dir_path, "ls", RpcValue());
		});
		enqueue(res, [=] (MockRpcConnection* mock) {
			EXPECT_RESPONSE(RpcValue::List{});
			mock->setBrokerConnected(false);
			QTimer::singleShot(0, [mock] {mock->setBrokerConnected(true);});
		});


		std::string new_cache_dir_path = "fin/hel/tram/hel002";
		enqueue(res, [=] (MockRpcConnection* mock) {
			SEND_SITES_YIELD(mock_sites::fin_master_broker);
		});
		enqueue(res, [=] (MockRpcConnection* mock) {
			EXPECT_SUBSCRIPTION_YIELD("shv", "mntchng");
		});
		enqueue(res, [=] (MockRpcConnection* mock) {
			EXPECT_SUBSCRIPTION_YIELD("shv/fin/hel/tram/hel002", "chng");
		});
		enqueue(res, [=] (MockRpcConnection* mock) {
			EXPECT_SUBSCRIPTION("shv/fin/hel/tram/hel002", "cmdlog");
			return CallNext::Yes;
		});

		DOCTEST_SUBCASE("new site is available")
		{
			enqueue(res, [=] (MockRpcConnection* mock) {
				REQUEST_YIELD(new_cache_dir_path, "ls", RpcValue());
			});
			enqueue(res, [=] (MockRpcConnection* mock) {
				EXPECT_RESPONSE(RpcValue::List{"eyas"});
			});
		}

		DOCTEST_SUBCASE("old site is not available")
		{
			enqueue(res, [=] (MockRpcConnection* mock) {
				REQUEST_YIELD(cache_dir_path, "ls", RpcValue());
			});
			enqueue(res, [=] (MockRpcConnection* mock) {
				EXPECT_ERROR("MethodNotFound: Method: 'ls' on path '/eyas/opc' doesn't exist");
			});
		}
	}

	DOCTEST_SUBCASE("disconnecting when pending request")
	{
		enqueue(res, [=] (MockRpcConnection* mock) {
			REQUEST_YIELD("_shvjournal", "syncLog", synclog_wait("shv/eyas/opc"));
		});
		enqueue(res, [=] (MockRpcConnection* mock) {
			EXPECT_REQUEST("shv/eyas/opc/.app/shvjournal", "lsfiles");
			mock->setBrokerConnected(false);
			DRIVER_WAIT(6000);
		});
		enqueue(res, [=] (MockRpcConnection*) {
			// Mustn't segfault.
		});
	}

	return res;
}

TEST_HISTORYPROVIDER_MAIN("connection_failure")
