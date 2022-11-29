#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "mockrpcconnection.h"
#include "pretty_printers.h"
#include <doctest/doctest.h>

#include "src/appclioptions.h"
#include "src/historyapp.h"
#include "tests/sites.h"
#include "tests/utils.h"

#include <shv/core/utils/shvlogrpcvaluereader.h>

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

	std::vector<std::string> expected_timestamps;
	shv::core::utils::ShvGetLogParams get_log_params;

	DOCTEST_SUBCASE("default params")
	{
		expected_timestamps = {
			"2022-07-07T18:06:15.557Z",
			"2022-07-07T18:06:17.784Z",
			"2022-07-07T18:06:17.784Z",
			"2022-07-07T18:06:17.869Z",
			"2022-07-07T18:06:17.872Z",
			"2022-07-07T18:06:17.874Z",
			"2022-07-07T18:06:17.880Z"
		};
	}

	DOCTEST_SUBCASE("since")
	{
		expected_timestamps = {
			"2022-07-07T18:06:17.872Z",
			"2022-07-07T18:06:17.874Z",
			"2022-07-07T18:06:17.880Z",
		};
		get_log_params.since = RpcValue::DateTime::fromUtcString("2022-07-07T18:06:17.872Z");
	}

	DOCTEST_SUBCASE("until")
	{
		expected_timestamps = {
			"2022-07-07T18:06:15.557Z",
			"2022-07-07T18:06:17.784Z",
			"2022-07-07T18:06:17.784Z",
			"2022-07-07T18:06:17.869Z",
		};
		get_log_params.until = RpcValue::DateTime::fromUtcString("2022-07-07T18:06:17.872Z");
	}

	REQUEST_YIELD(cache_dir_path, "getLog", get_log_params.toRpcValue());

	// For now,I'll only make a simple test: I'll assume that if the timestamps are correct, everything else is also
	// correct. This is not the place to test the log uitilities anyway.
	REQUIRE(m_messageQueue.head().isResponse());
	std::vector<std::string> actual_timestamps;
	shv::core::utils::ShvLogRpcValueReader entries(shv::chainpack::RpcResponse(m_messageQueue.head()).result());
	while (entries.next()) {
		actual_timestamps.push_back(RpcValue::DateTime::fromMSecsSinceEpoch(entries.entry().epochMsec).toIsoString());
	}

	REQUIRE(actual_timestamps == expected_timestamps);
	m_messageQueue.dequeue();
}

TEST_HISTORYPROVIDER_MAIN("hp_getlog")
