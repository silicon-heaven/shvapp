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
#define MESSAGE_HANDLER enqueue(res, [] (MockRpcConnection* mock)

QQueue<std::function<CallNext(MockRpcConnection*)>> setup_test()
{
	QQueue<std::function<CallNext(MockRpcConnection*)>> res;
	DOCTEST_SUBCASE("sites request times out")
	{
		enqueue(res, [] (MockRpcConnection* mock) {
			EXPECT_REQUEST("sites", "getSites", RpcValue());
			mock->m_messageQueue.dequeue();
			DRIVER_WAIT(10000);
		});
		enqueue(res, [] (MockRpcConnection* mock) {
			EXPECT_REQUEST("sites", "getSites", RpcValue());
			RESPOND_YIELD(""); // Respond with empty sites to avoid leaks.
		});
		enqueue(res, [] (MockRpcConnection* mock) {
			EXPECT_SUBSCRIPTION("shv", "mntchng");
		});
	}

	DOCTEST_SUBCASE("manual request to reload sites")
	{
		enqueue(res, [] (MockRpcConnection* mock) {
			EXPECT_REQUEST("sites", "getSites", RpcValue());
			RESPOND_YIELD("");
		});
		enqueue(res, [] (MockRpcConnection* mock) {
			EXPECT_SUBSCRIPTION("shv", "mntchng");
			REQUEST_YIELD("", "reloadSites");
		});

		enqueue(res, [] (MockRpcConnection* mock) {
			EXPECT_REQUEST("sites", "getSites", RpcValue());
			return CallNext::Yes;
		});

		DOCTEST_SUBCASE("normal operation")
		{
			enqueue(res, [] (MockRpcConnection* mock) {
				RESPOND_YIELD("");
			});
		}

		DOCTEST_SUBCASE("reloadSites while already reloading produces error")
		{
			enqueue(res, [] (MockRpcConnection* mock) {
				REQUEST_YIELD("", "reloadSites");
			});
			enqueue(res, [] (MockRpcConnection* mock) {
				RESPOND_YIELD("");
			});
			enqueue(res, [] (MockRpcConnection* mock) {
				EXPECT_ERROR("MethodCallException: Sites are already being reloaded.");
				return CallNext::Yes;
			});
		}


		enqueue(res, [] (MockRpcConnection* mock) {
			EXPECT_SUBSCRIPTION_YIELD("shv", "mntchng");
		});
		enqueue(res, [] (MockRpcConnection* mock) {
			EXPECT_RESPONSE("Sites reloaded.");
		});
	}

	return res;
}

TEST_HISTORYPROVIDER_MAIN("sites_timeout")
