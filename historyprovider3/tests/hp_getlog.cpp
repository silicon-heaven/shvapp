#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "mockrpcconnection.h"
#include "pretty_printers.h"
#include <doctest/doctest.h>

#include "src/appclioptions.h"
#include "src/historyapp.h"
#include "tests/sites.h"
#include "tests/utils.h"

QCoro::Generator<int> MockRpcConnection::driver()
{
	co_yield {};

	// We want the test to download everything regardless of age. Ten years ought to be enough.
	HistoryApp::instance()->cliOptions()->setCacheInitMaxAge(60 /*seconds*/ * 60 /*minutes*/ * 24 /*hours*/ * 365 /*days*/ * 10 /*years*/);
	std::string cache_dir_path = "eyas/opc";
	SEND_SITES_YIELD(mock_sites::fin_slave_broker);
	std::string sub_path = "shv/eyas/opc";
	EXPECT_SUBSCRIPTION_YIELD(sub_path, "mntchng");
	EXPECT_SUBSCRIPTION(sub_path, "chng");

	create_dummy_cache_files(cache_dir_path, {
		{ "2022-07-07T18-06-15-557.log2", dummy_logfile },
		{ "2022-07-07T18-06-15-558.log2", dummy_logfile2 },
		{ "dirtylog", dummy_logfile },
	});

	std::vector<int64_t> expected_timestamps;
	shv::core::utils::ShvGetLogParams get_log_params;

	DOCTEST_SUBCASE("default params")
	{
		expected_timestamps = {1657217175557, 1657217177784, 1657217177784, 1657217177869, 1657217177872, 1657217177874, 1657217177880};
	}

	DOCTEST_SUBCASE("since")
	{
		expected_timestamps = {1657217177872, 1657217177874, 1657217177880};
		get_log_params.since = RpcValue::DateTime::fromMSecsSinceEpoch(1657217177872);
	}

	DOCTEST_SUBCASE("until")
	{
		expected_timestamps = {1657217175557, 1657217177784, 1657217177784, 1657217177869};
		get_log_params.until = RpcValue::DateTime::fromMSecsSinceEpoch(1657217177870);
	}

	REQUEST_YIELD(cache_dir_path, "getLog", get_log_params.toRpcValue());

	// For now,I'll only make a simple test: I'll assume that if the timestamps are correct, everything else is also
	// correct. This is not the place to test the log uitilities anyway.
	REQUIRE(m_messageQueue.head().isResponse());
	std::vector<int64_t> actual_timestamps;
	shv::core::utils::ShvMemoryJournal entries;
	entries.loadLog(shv::chainpack::RpcResponse(m_messageQueue.head()).result());
	for (const auto& entry : entries.entries()) {
		actual_timestamps.push_back(entry.epochMsec);
	}

	REQUIRE(actual_timestamps == expected_timestamps);
	m_messageQueue.dequeue();
}

TEST_HISTORYPROVIDER_MAIN("hp_getlog")
