#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "mockrpcconnection.h"
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
	EXPECT_REQUEST("sites", "getSites", RpcValue());
	m_messageQueue.dequeue();
	DRIVER_WAIT(6000);
	EXPECT_REQUEST("sites", "getSites", RpcValue());
	RESPOND(""); // Respond with empty sites to avoid leaks.
	co_return;
}

DOCTEST_TEST_CASE("HistoryApp")
{
	NecroLog::registerTopic("MockRpcConnection", "");
	NecroLog::setTopicsLogTresholds(":D");
	QCoreApplication::setApplicationName("historyprovider tester");

	int argc = 0;
	char *argv[] = { nullptr };
	AppCliOptions cli_opts;
	HistoryApp app(argc, argv, &cli_opts, new MockRpcConnection());
	app.exec();
}
