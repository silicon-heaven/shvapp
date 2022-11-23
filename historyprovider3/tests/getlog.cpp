#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "src/getlog.h"
#include "test_vars.h"
#include "pretty_printers.h"

#include <doctest/doctest.h>

#include <shv/core/utils/shvgetlogparams.h>
#include <shv/core/utils/shvmemoryjournal.h>
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
			std::vector<int64_t> expected_timestamps;

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
			std::vector<int64_t> actual_timestamps;
			shv::core::utils::ShvMemoryJournal entries;
			entries.loadLog(getLog(readers, get_log_params));
			for (const auto& entry : entries.entries()) {
				actual_timestamps.push_back(entry.epochMsec);
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
			shv::core::utils::ShvMemoryJournal entries;
			entries.loadLog(getLog(readers, get_log_params));
			for (const auto& entry : entries.entries()) {
				actual_paths.push_back(entry.path);
			}

			REQUIRE(actual_paths == expected_paths);
		}
	}
}
