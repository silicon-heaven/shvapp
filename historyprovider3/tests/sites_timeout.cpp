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

QCoro::Generator<int> MockRpcConnection::driver()
{
	co_yield {};
	DOCTEST_SUBCASE("sites request times out")
	{
		EXPECT_REQUEST("sites", "getSites", RpcValue());
		m_messageQueue.dequeue();
		DRIVER_WAIT(10000);
		EXPECT_REQUEST("sites", "getSites", RpcValue());
		RESPOND_YIELD(""); // Respond with empty sites to avoid leaks.
		EXPECT_SUBSCRIPTION("shv", "mntchng");
	}

	DOCTEST_SUBCASE("manual request to reload sites")
	{
		EXPECT_REQUEST("sites", "getSites", RpcValue());
		RESPOND_YIELD("");
		EXPECT_SUBSCRIPTION("shv", "mntchng");

		REQUEST_YIELD("", "reloadSites");
		EXPECT_REQUEST("sites", "getSites", RpcValue());

		DOCTEST_SUBCASE("normal operation")
		{
			RESPOND_YIELD("");
		}

		DOCTEST_SUBCASE("reloadSites while already reloading produces error")
		{
			REQUEST_YIELD("", "reloadSites");
			RESPOND_YIELD("");
			EXPECT_ERROR("RPC ERROR MethodCallException: Sites are already being reloaded.");
		}

		EXPECT_SUBSCRIPTION_YIELD("shv", "mntchng");
		EXPECT_RESPONSE("Sites reloaded.");
	}
}

TEST_HISTORYPROVIDER_MAIN("sites_timeout")
