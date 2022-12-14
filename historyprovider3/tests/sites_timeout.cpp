#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "mockrpcconnection.h"
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
	EXPECT_REQUEST("sites", "getSites", RpcValue());
	m_messageQueue.dequeue();
	DRIVER_WAIT(10000);
	EXPECT_REQUEST("sites", "getSites", RpcValue());
	RESPOND(""); // Respond with empty sites to avoid leaks.
	co_return;
}

TEST_HISTORYPROVIDER_MAIN("sites_timeout")
