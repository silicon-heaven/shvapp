#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "src/getlog.h"
#include "test_vars.h"
#include "pretty_printers.h"

#include <doctest/doctest.h>

#include <shv/core/utils/shvgetlogparams.h>
#include <shv/core/utils/shvlogrpcvaluereader.h>
#include <shv/core/utils/shvjournalfilereader.h>
#include <shv/core/utils/shvjournalfilewriter.h>
#include <shv/core/log.h>

#include <filesystem>

namespace cp = shv::chainpack;
using cp::RpcValue;
using namespace std::string_literals;

const auto journal_dir = std::filesystem::path{TESTS_DIR} / "getlog";

std::function<shv::core::utils::ShvJournalFileReader()> create_reader(std::vector<shv::core::utils::ShvJournalEntry> entries)
{
	if (entries.empty()) {
		return {};
	}
	shv::core::utils::ShvJournalFileWriter writer(journal_dir, entries.begin()->epochMsec, entries.back().epochMsec);

	for (const auto& entry : entries) {
		writer.append(entry);
	}

	return [file_name = writer.fileName()] {
		return shv::core::utils::ShvJournalFileReader{file_name};
	};
}

auto make_entry(const std::string& timestamp, const std::string& path, const RpcValue& value, bool snapshot)
{
	auto res = shv::core::utils::ShvJournalEntry(
		path,
		value,
		shv::core::utils::ShvJournalEntry::DOMAIN_VAL_CHANGE,
		shv::core::utils::ShvJournalEntry::NO_SHORT_TIME,
		shv::core::utils::ShvJournalEntry::NO_VALUE_FLAGS,
		RpcValue::DateTime::fromUtcString(timestamp).msecsSinceEpoch());

	res.setSnapshotValue(snapshot);
	return res;
}

auto as_vector(shv::core::utils::ShvLogRpcValueReader& reader)
{
	std::vector<shv::core::utils::ShvJournalEntry> res;
	while (reader.next()) {
		res.push_back(reader.entry());
	}
	return res;
}

DOCTEST_TEST_CASE("getLog")
{
	NecroLog::setTopicsLogTresholds("GetLog:D,ShvJournal:D");
    shv::core::utils::ShvGetLogParams get_log_params;
	std::filesystem::remove_all(journal_dir);
	std::filesystem::create_directories(journal_dir);

	DOCTEST_SUBCASE("filtering")
	{
		std::vector<std::function<shv::core::utils::ShvJournalFileReader()>> readers {
			create_reader({
				make_entry("2022-07-07T18:06:15.557Z", "APP_START", true, false),
				make_entry("2022-07-07T18:06:17.784Z", "zone1/system/sig/plcDisconnected", false, false),
				make_entry("2022-07-07T18:06:17.784Z", "zone1/zone/Zone1/plcDisconnected", false, false),
				make_entry("2022-07-07T18:06:17.869Z", "zone1/pme/TSH1-1/switchRightCounterPermanent", 0U, false),
			}),
			create_reader({
				make_entry("2022-07-07T18:06:17.872Z", "zone1/system/sig/plcDisconnected", false, false),
				make_entry("2022-07-07T18:06:17.874Z", "zone1/zone/Zone1/plcDisconnected", false, false),
				make_entry("2022-07-07T18:06:17.880Z", "zone1/pme/TSH1-1/switchRightCounterPermanent", 0U, false),
			})
		};

		DOCTEST_SUBCASE("since/until")
		{
			std::vector<std::string> expected_timestamps;

			DOCTEST_SUBCASE("default params")
			{
				expected_timestamps = {
					"2022-07-07T18:06:15.557Z",
					"2022-07-07T18:06:17.784Z",
					"2022-07-07T18:06:17.784Z",
					"2022-07-07T18:06:17.869Z",
					"2022-07-07T18:06:17.872Z",
					"2022-07-07T18:06:17.874Z",
					"2022-07-07T18:06:17.880Z"};
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
			std::vector<std::string> actual_timestamps;
			shv::core::utils::ShvLogRpcValueReader entries(getLog(readers, get_log_params));
			for (const auto& entry : as_vector(entries)) {
				actual_timestamps.push_back(RpcValue::DateTime::fromMSecsSinceEpoch(entry.epochMsec).toIsoString());
			}

			REQUIRE(actual_timestamps == expected_timestamps);
		}

		DOCTEST_SUBCASE("pattern")
		{
			std::vector<std::string> expected_paths;

			DOCTEST_SUBCASE("default params")
			{
				expected_paths = {
					"APP_START",
					"zone1/system/sig/plcDisconnected",
					"zone1/zone/Zone1/plcDisconnected",
					"zone1/pme/TSH1-1/switchRightCounterPermanent",
					"zone1/system/sig/plcDisconnected",
					"zone1/zone/Zone1/plcDisconnected",
					"zone1/pme/TSH1-1/switchRightCounterPermanent",
				};
			}

			DOCTEST_SUBCASE("exact path")
			{
				get_log_params.pathPattern = "zone1/pme/TSH1-1/switchRightCounterPermanent";
				expected_paths = {
					// There are two entries with this path
					"zone1/pme/TSH1-1/switchRightCounterPermanent",
					"zone1/pme/TSH1-1/switchRightCounterPermanent",
				};
			}

			DOCTEST_SUBCASE("wildcard")
			{
				get_log_params.pathPattern = "zone1/**";
				expected_paths = {
					"zone1/system/sig/plcDisconnected",
					"zone1/zone/Zone1/plcDisconnected",
					"zone1/pme/TSH1-1/switchRightCounterPermanent",
					"zone1/system/sig/plcDisconnected",
					"zone1/zone/Zone1/plcDisconnected",
					"zone1/pme/TSH1-1/switchRightCounterPermanent",
				};
			}

			std::vector<std::string> actual_paths;
			shv::core::utils::ShvLogRpcValueReader entries(getLog(readers, get_log_params));
			for (const auto& entry : as_vector(entries)) {
				actual_paths.push_back(entry.path);
			}

			REQUIRE(actual_paths == expected_paths);
		}

		DOCTEST_SUBCASE("record count limit")
		{
			int expected_count;
			bool expected_record_count_limit_hit;

			DOCTEST_SUBCASE("default")
			{
				expected_count = 7;
				expected_record_count_limit_hit = false;
			}

			DOCTEST_SUBCASE("1000")
			{
				get_log_params.recordCountLimit = 1000;
				expected_count = 7;
				expected_record_count_limit_hit = false;
			}

			DOCTEST_SUBCASE("7")
			{
				get_log_params.recordCountLimit = 7;
				expected_count = 7;
				expected_record_count_limit_hit = false;
			}

			DOCTEST_SUBCASE("3")
			{
				get_log_params.recordCountLimit = 3;
				expected_count = 3;
				expected_record_count_limit_hit = true;
			}

			shv::core::utils::ShvLogRpcValueReader entries(getLog(readers, get_log_params));

			REQUIRE(entries.logHeader().recordCountLimitHit() == expected_record_count_limit_hit);
			REQUIRE(as_vector(entries).size() == expected_count);
		}
	}

	DOCTEST_SUBCASE("withPathsDict")
	{
		std::vector<std::function<shv::core::utils::ShvJournalFileReader()>> readers {
			create_reader({
				make_entry("2022-07-07T18:06:15.557Z", "APP_START", true, false),
				make_entry("2022-07-07T18:06:17.784Z", "zone1/system/sig/plcDisconnected", false, false),
				make_entry("2022-07-07T18:06:17.784Z", "zone1/zone/Zone1/plcDisconnected", false, false),
				make_entry("2022-07-07T18:06:17.869Z", "zone1/pme/TSH1-1/switchRightCounterPermanent", 0U, false),
			}),
			create_reader({
				make_entry("2022-07-07T18:06:17.872Z", "zone1/system/sig/plcDisconnected", false, false),
				make_entry("2022-07-07T18:06:17.874Z", "zone1/zone/Zone1/plcDisconnected", false, false),
				make_entry("2022-07-07T18:06:17.880Z", "zone1/pme/TSH1-1/switchRightCounterPermanent", 0U, false),
			})
		};

		DOCTEST_SUBCASE("without path dict")
		{
			get_log_params.withPathsDict = false;
		}

		DOCTEST_SUBCASE("with path dict")
		{
			get_log_params.withPathsDict = true;
		}

		shv::core::utils::ShvLogRpcValueReader entries(getLog(readers, get_log_params));
		REQUIRE(entries.logHeader().withPathsDict() == get_log_params.withPathsDict);
		REQUIRE(as_vector(entries).size() == 7); // Verify all entries were read correctly
	}

	DOCTEST_SUBCASE("withSnapshot")
	{
		std::vector<std::function<shv::core::utils::ShvJournalFileReader()>> readers {
			create_reader({
				make_entry("2022-07-07T18:06:17.784Z", "value1", false, true),
				make_entry("2022-07-07T18:06:17.784Z", "value2", false, true),
				make_entry("2022-07-07T18:06:17.784Z", "value3", true, true),
				make_entry("2022-07-07T18:06:17.822Z", "value2", true, false),
			}),
		};

		std::vector<shv::core::utils::ShvJournalEntry> expected_entries;

		DOCTEST_SUBCASE("without snapshot")
		{
			get_log_params.withSnapshot = false;
			expected_entries = {
				make_entry("2022-07-07T18:06:17.822Z", "value2", true, false),
			};
		}

		DOCTEST_SUBCASE("with snapshot")
		{
			get_log_params.withSnapshot = true;
			DOCTEST_SUBCASE("without since param")
			{
				expected_entries = {
					make_entry("2022-07-07T18:06:17.822Z", "value1", false, true),
					make_entry("2022-07-07T18:06:17.822Z", "value2", true, true),
					make_entry("2022-07-07T18:06:17.822Z", "value3", true, true),
					make_entry("2022-07-07T18:06:17.822Z", "value2", true, false),
				};
			}

			DOCTEST_SUBCASE("with since param")
			{
				get_log_params.since = RpcValue::DateTime::fromUtcString("2022-07-07T18:06:17.785Z");
				expected_entries = {
					make_entry("2022-07-07T18:06:17.785Z", "value1", false, true),
					make_entry("2022-07-07T18:06:17.785Z", "value2", true, true),
					make_entry("2022-07-07T18:06:17.785Z", "value3", true, true),
					make_entry("2022-07-07T18:06:17.822Z", "value2", true, false),
				};
			}
		}

		shv::core::utils::ShvLogRpcValueReader entries(getLog(readers, get_log_params));
		REQUIRE(entries.logHeader().withSnapShot() == get_log_params.withSnapshot);
		REQUIRE(as_vector(entries) == expected_entries);
	}
}
