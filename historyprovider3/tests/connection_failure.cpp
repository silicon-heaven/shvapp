#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "mockrpcconnection.h"
#include "pretty_printers.h"
#include <doctest/doctest.h>

#include "src/appclioptions.h"
#include "src/historyapp.h"
#include "tests/sites.h"

#include <shv/core/utils/shvjournalentry.h>
#include <shv/core/utils/shvmemoryjournal.h>

#include <QDir>


namespace cp = shv::chainpack;
using cp::RpcValue;

QCoro::Generator<int> MockRpcConnection::driver()
{
	co_yield {};
	std::string cache_dir_path = "eyas/opc";
	SEND_SITES_YIELD(mock_sites::fin_slave_broker);
	EXPECT_SUBSCRIPTION_YIELD("shv/eyas/opc", "mntchng");
	EXPECT_SUBSCRIPTION("shv/eyas/opc", "chng");

	REQUEST_YIELD(cache_dir_path, "ls", RpcValue());
	EXPECT_RESPONSE(RpcValue::List{});

	setBrokerConnected(false);
	QTimer::singleShot(0, [this] {setBrokerConnected(true);});
	co_yield {};

	std::string new_cache_dir_path = "fin/hel/tram/hel002";
	SEND_SITES_YIELD(mock_sites::fin_master_broker);
	EXPECT_SUBSCRIPTION_YIELD("shv/fin/hel/tram/hel002", "mntchng");
	EXPECT_SUBSCRIPTION("shv/fin/hel/tram/hel002", "chng");

	DOCTEST_SUBCASE("new site is available")
	{
		REQUEST_YIELD(new_cache_dir_path, "ls", RpcValue());
		EXPECT_RESPONSE(RpcValue::List{"eyas"});
	}

	DOCTEST_SUBCASE("old site is not available")
	{
		REQUEST_YIELD(cache_dir_path, "ls", RpcValue());
		EXPECT_ERROR("RPC ERROR MethodCallException: Method: 'ls' on path '/eyas/opc' doesn't exist.");
	}

	co_return;
}

DOCTEST_TEST_CASE("HistoryApp")
{
	NecroLog::registerTopic("MockRpcConnection", "");
	NecroLog::setTopicsLogTresholds(":D");
	NecroLog::setFileLogTresholds(":D");
	QCoreApplication::setApplicationName("historyprovider tester");

	int argc = 0;
	char *argv[] = { nullptr };
	AppCliOptions cli_opts;
	HistoryApp app(argc, argv, &cli_opts, new MockRpcConnection());
	app.exec();
}
