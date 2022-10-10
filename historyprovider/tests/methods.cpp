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

namespace std {
    doctest::String toString(const std::vector<int64_t>& values) {
		std::ostringstream res;
		res << "std::vector<int64_t>{";
		for (const auto& value : values) {
			res << value << ", ";
		}
		res << "}";
		return res.str().c_str();
    }
}

namespace shv::chainpack {

    doctest::String toString(const RpcValue& value) {
        return value.toCpon().c_str();
    }

    doctest::String toString(const RpcValue::List& value) {
        return RpcValue(value).toCpon().c_str();
    }

    doctest::String toString(const RpcMessage& value) {
        return value.toPrettyString().c_str();
    }
}

auto get_site_cache_dir(const std::string& site_path)
{
	return QDir{QString::fromStdString(shv::core::Utils::joinPath(HistoryApp::instance()->cliOptions()->journalCacheRoot(), site_path))};
}

void remove_cache_contents(const std::string& site_path)
{
	auto cache_dir = get_site_cache_dir(site_path);
	cache_dir.removeRecursively();
	cache_dir.mkpath(".");
}

auto get_cache_contents(const std::string& site_path)
{
	RpcValue::List res;
	auto cache_dir = get_site_cache_dir(site_path);
	auto entries = cache_dir.entryList(QDir::NoDotAndDotDot | QDir::Files);
	std::transform(entries.begin(), entries.end(), std::back_inserter(res), [&cache_dir] (const auto& entry) {
		RpcValue::List name_and_size;
		name_and_size.push_back(entry.toStdString());
		QFile file(entry);
		name_and_size.push_back(RpcValue::UInt(QFile(cache_dir.filePath(entry)).size()));
		return name_and_size;
	});

	return res;
}

using namespace std::string_literals;
const auto dummy_logfile = R"(2022-07-07T18:06:15.557Z	809779	APP_START	true		SHV_SYS	0	
2022-07-07T18:06:17.784Z	809781	zone1/system/sig/plcDisconnected	false		chng	2	
2022-07-07T18:06:17.784Z	809781	zone1/zone/Zone1/plcDisconnected	false		chng	2	
2022-07-07T18:06:17.869Z	809781	zone1/pme/TSH1-1/switchRightCounterPermanent	0u		chng	2	
)"s;

const auto dummy_logfile2 = R"(2022-07-07T18:06:17.872Z	809781	zone1/system/sig/plcDisconnected	false		chng	2	
2022-07-07T18:06:17.874Z	809781	zone1/zone/Zone1/plcDisconnected	false		chng	2	
2022-07-07T18:06:17.880Z	809781	zone1/pme/TSH1-1/switchRightCounterPermanent	0u		chng	2	
)"s;

const auto dummy_pushlog = RpcValue::fromCpon(R"(
<
  "dateTime":d"2022-09-15T13:30:04.293Z",
  "device":{"id":"historyprovider"},
  "fields":[
    {"name":"timestamp"},
    {"name":"path"},
    {"name":"value"},
    {"name":"shortTime"},
    {"name":"domain"},
    {"name":"valueFlags"},
    {"name":"userId"}
  ],
  "logParams":{"recordCountLimit":1000, "until":d"2022-07-07T18:06:17.870Z", "withPathsDict":true, "withSnapshot":false, "withTypeInfo":false},
  "logVersion":2,
  "pathsDict":i{1:"APP_START", 2:"zone1/system/sig/plcDisconnected", 3:"zone1/zone/Zone1/plcDisconnected", 4:"zone1/pme/TSH1-1/switchRightCounterPermanent"},
  "recordCount":4,
  "recordCountLimit":1000,
  "recordCountLimitHit":false,
  "since":d"2022-07-07T18:06:15.557Z",
  "until":d"2022-07-07T18:06:15.557Z",
  "withPathsDict":true,
  "withSnapshot":false
>[
  [d"2022-07-07T18:06:15.557Z", 1, true, null, "SHV_SYS", 0u, null],
]
)");

const auto dummy_pushlog3 = RpcValue::fromCpon(R"(
<
  "dateTime":d"2022-09-15T13:30:04.293Z",
  "device":{"id":"historyprovider"},
  "fields":[
    {"name":"timestamp"},
    {"name":"path"},
    {"name":"value"},
    {"name":"shortTime"},
    {"name":"domain"},
    {"name":"valueFlags"},
    {"name":"userId"}
  ],
  "logParams":{"recordCountLimit":1000, "until":d"2022-07-07T18:06:17.870Z", "withPathsDict":true, "withSnapshot":false, "withTypeInfo":false},
  "logVersion":2,
  "pathsDict":i{1:"APP_START", 2:"zone1/system/sig/plcDisconnected", 3:"zone1/zone/Zone1/plcDisconnected", 4:"zone1/pme/TSH1-1/switchRightCounterPermanent"},
  "recordCount":4,
  "recordCountLimit":1000,
  "recordCountLimitHit":false,
  "since":d"2022-07-07T18:06:15.557Z",
  "until":d"2022-07-07T18:06:15.557Z",
  "withPathsDict":true,
  "withSnapshot":false
>[
  [d"2022-07-07T18:06:15.557Z", 2, true, null, "SHV_SYS", 0u, null],
]
)");

const auto dummy_pushlog2 = RpcValue::fromCpon(R"(
<
  "dateTime":d"2022-09-15T13:30:04.293Z",
  "device":{"id":"historyprovider"},
  "fields":[
    {"name":"timestamp"},
    {"name":"path"},
    {"name":"value"},
    {"name":"shortTime"},
    {"name":"domain"},
    {"name":"valueFlags"},
    {"name":"userId"}
  ],
  "logParams":{"recordCountLimit":1000, "until":d"2022-07-07T18:06:17.870Z", "withPathsDict":true, "withSnapshot":false, "withTypeInfo":false},
  "logVersion":2,
  "pathsDict":i{1:"APP_START", 2:"zone1/system/sig/plcDisconnected", 3:"zone1/zone/Zone1/plcDisconnected", 4:"zone1/pme/TSH1-1/switchRightCounterPermanent"},
  "recordCount":4,
  "recordCountLimit":1000,
  "recordCountLimitHit":false,
  "since":d"2022-07-07T18:06:15.554Z",
  "until":d"2022-07-07T18:06:15.554Z",
  "withPathsDict":true,
  "withSnapshot":false
>[
  [d"2022-07-07T18:06:15.554Z", 1, true, null, "SHV_SYS", 0u, null],
]
)");

const auto dummy_getlog_response = RpcValue::fromCpon(R"(
<
  "dateTime":d"2022-09-15T13:30:04.293Z",
  "device":{"id":"historyprovider"},
  "fields":[
    {"name":"timestamp"},
    {"name":"path"},
    {"name":"value"},
    {"name":"shortTime"},
    {"name":"domain"},
    {"name":"valueFlags"},
    {"name":"userId"}
  ],
  "logParams":{"recordCountLimit":1000, "until":d"2022-07-07T18:06:17.870Z", "withPathsDict":true, "withSnapshot":false, "withTypeInfo":false},
  "logVersion":2,
  "pathsDict":i{1:"APP_START", 2:"zone1/system/sig/plcDisconnected", 3:"zone1/zone/Zone1/plcDisconnected", 4:"zone1/pme/TSH1-1/switchRightCounterPermanent"},
  "recordCount":4,
  "recordCountLimit":1000,
  "recordCountLimitHit":false,
  "since":d"2022-07-07T18:06:15.557Z",
  "until":d"2022-07-07T18:06:17.870Z",
  "withPathsDict":true,
  "withSnapshot":false
>[
  [d"2022-07-07T18:06:15.557Z", 1, true, null, "SHV_SYS", 0u, null],
  [d"2022-07-07T18:06:17.784Z", 2, false, null, null, 2u, null],
  [d"2022-07-07T18:06:17.784Z", 3, false, null, null, 2u, null],
  [d"2022-07-07T18:06:17.869Z", 4, 0u, null, null, 2u, null]
]
)");

struct DummyFileInfo {
	QString fileName;
	std::string content;
};

void create_dummy_cache_files(const std::string& site_path, const std::vector<DummyFileInfo>& files)
{
	remove_cache_contents(site_path);
	auto cache_dir = get_site_cache_dir(site_path);

	for (const auto& file : files) {
		QFile qfile(cache_dir.filePath(file.fileName));
		qfile.open(QFile::WriteOnly);
		qfile.write(file.content.c_str());
	}
}

auto join(const std::string& a, const std::string& b)
{
	return shv::core::Utils::joinPath(a, b);
}

auto make_sub_params(const std::string& path, const std::string& method)
{
	return shv::chainpack::RpcValue::fromCpon(R"({"method":")" + method + R"(","path":")" + path + R"("}")");
}

const auto ls_size_true = RpcValue::fromCpon(R"({"size": true})");
const auto read_offset_0 = RpcValue::fromCpon(R"({"offset":0})");

QCoro::Generator<int> MockRpcConnection::driver()
{
	co_yield {};

	// We want the test to download everything regardless of age. Ten years ought to be enough.
	HistoryApp::instance()->cliOptions()->setCacheInitMaxAge(60 /*seconds*/ * 60 /*minutes*/ * 24 /*hours*/ * 365 /*days*/ * 10 /*years*/);
	DOCTEST_SUBCASE("fin slave HP")
	{
		std::string cache_dir_path = "shv/eyas/opc";
		SEND_SITES_YIELD(mock_sites::fin_slave_broker);
		EXPECT_SUBSCRIPTION_YIELD(cache_dir_path, "mntchng");
		EXPECT_SUBSCRIPTION(cache_dir_path, "chng");
		REQUEST_YIELD("shvjournal", "syncLog", RpcValue());
		EXPECT_REQUEST(join(cache_dir_path, "/.app/shvjournal"), "lsfiles", ls_size_true);

		DOCTEST_SUBCASE("syncLog")
		{
			RpcValue::List expected_cache_contents;
			DOCTEST_SUBCASE("Remote and local - empty")
			{
				create_dummy_cache_files(cache_dir_path, {});
				RESPOND_YIELD(RpcValue::List());
			}

			DOCTEST_SUBCASE("Remote - has files, local - empty")
			{
				create_dummy_cache_files(cache_dir_path, {});
				expected_cache_contents = RpcValue::List({{
					RpcValue::List{ "2022-07-07T18-06-15-557.log2", dummy_logfile.size() }
				}});
				RESPOND_YIELD((RpcValue::List({{
					{ "2022-07-07T18-06-15-557.log2", "f", dummy_logfile.size() }
				}})));

				EXPECT_REQUEST("shv/eyas/opc/.app/shvjournal/2022-07-07T18-06-15-557.log2", "read", read_offset_0);
				RESPOND_YIELD(RpcValue::stringToBlob(dummy_logfile));
			}

			DOCTEST_SUBCASE("dirty log")
			{
				create_dummy_cache_files(cache_dir_path, {});
				// Respond to the initial `lsfiles` request.
				RESPOND_YIELD(RpcValue::List());

				DOCTEST_SUBCASE("HP accepts events and puts them into the log")
				{
					NOTIFY("shv/eyas/opc/power-on", "chng", true);
					NOTIFY("shv/eyas/opc/power-on", "chng", false);

					expected_cache_contents = RpcValue::List({{
						RpcValue::List{ "dirty.log2", 101UL }
					}});
				}

				EXPECT_RESPONSE("All files have been synced");
				REQUIRE(get_cache_contents(cache_dir_path) == expected_cache_contents);
			}

			DOCTEST_SUBCASE("Don't download files older than we already have")
			{
				expected_cache_contents = RpcValue::List({{
					RpcValue::List{ "2022-07-07T18-06-15-557.log2", 0UL },
					RpcValue::List{ "2022-07-07T18-06-15-558.log2", 0UL }
				}});
				create_dummy_cache_files(cache_dir_path, {
					{"2022-07-07T18-06-15-557.log2", ""},
					{"2022-07-07T18-06-15-558.log2", ""}
				});
				RESPOND_YIELD((RpcValue::List({{
					{ "2022-07-07T18-06-15-000.log2", "f", dummy_logfile.size() }
				}})));

				EXPECT_RESPONSE("All files have been synced");
				REQUIRE(get_cache_contents(cache_dir_path) == expected_cache_contents);
			}

			DOCTEST_SUBCASE("Don't download files older than one month")
			{
				HistoryApp::instance()->cliOptions()->setCacheInitMaxAge(60 /*seconds*/ * 60 /*minutes*/ * 24 /*hours*/ * 30 /*days*/);
				create_dummy_cache_files(cache_dir_path, {});
				RESPOND_YIELD((RpcValue::List({{
					{ "2022-07-07T18-06-15-000.log2", "f", dummy_logfile.size() }
				}})));

				EXPECT_RESPONSE("All files have been synced");
				REQUIRE(get_cache_contents(cache_dir_path) == expected_cache_contents);
			}
		}

		DOCTEST_SUBCASE("periodic syncLog")
		{
			// Respond to the initial syncLog request.
			RESPOND_YIELD(RpcValue::List());
			EXPECT_RESPONSE("All files have been synced");

			DOCTEST_SUBCASE("dirty log too old")
			{
				create_dummy_cache_files(cache_dir_path, {
					// Make a 100 second old entry.
					{"dirty.log2", QDateTime::currentDateTimeUtc().addSecs(-100).toString(Qt::DateFormat::ISODate).toStdString() + "	809781	zone1/zone/Zone1/plcDisconnected	false		chng	2	\n"}
				});

				DOCTEST_SUBCASE("synclog should not trigger")
				{
					HistoryApp::instance()->cliOptions()->setLogMaxAge(1000);
					NOTIFY("shv/eyas/opc/power-on", "chng", true); // Send an event so that HP checks for dirty log age.
					// Nothing happens afterwards, since max age is >10
				}

				DOCTEST_SUBCASE("synclog should trigger")
				{
					HistoryApp::instance()->cliOptions()->setLogMaxAge(10);
					NOTIFY_YIELD("shv/eyas/opc/power-on", "chng", true);
					EXPECT_REQUEST(join(cache_dir_path, "/.app/shvjournal"), "lsfiles", ls_size_true);
					RESPOND(RpcValue::List()); // We only test if the syncLog triggers.
				}
			}

			DOCTEST_SUBCASE("device mounted")
			{
				NOTIFY_YIELD(cache_dir_path, "mntchng", true);
				EXPECT_REQUEST(join(cache_dir_path, "/.app/shvjournal"), "lsfiles", ls_size_true);
				RESPOND(RpcValue::List()); // We only test if the syncLog triggers.
			}
		}
	}

	DOCTEST_SUBCASE("syncing from slave HP")
	{
		RpcValue::List expected_cache_contents;
		std::string cache_dir_path = "shv/fin/hel/tram/hel002";
		SEND_SITES_YIELD(mock_sites::fin_master_broker);
		EXPECT_SUBSCRIPTION_YIELD("shv/fin/hel/tram/hel002", "mntchng");
		EXPECT_SUBSCRIPTION("shv/fin/hel/tram/hel002", "chng");
		std::string master_shv_journal_path = "shvjournal";
		std::string slave_shv_journal_path = "shv/fin/hel/tram/hel002/.local/history/shvjournal";
		REQUEST_YIELD(master_shv_journal_path, "syncLog", RpcValue());
		EXPECT_REQUEST(slave_shv_journal_path, "lsfiles", ls_size_true);

		DOCTEST_SUBCASE("Remote - has files, local - empty")
		{
			// NOTE: I'm only really testing whether historyprovider correctly enters the .local broker.
			create_dummy_cache_files(cache_dir_path, {});
			expected_cache_contents = RpcValue::List({{
				RpcValue::List{ "2022-07-07T18-06-15-557.log2", dummy_logfile.size() }
			}});
			RESPOND_YIELD((RpcValue::List{{
				{ "2022-07-07T18-06-15-557.log2", "f", dummy_logfile.size() }
			}}));

			EXPECT_REQUEST(join(slave_shv_journal_path, "2022-07-07T18-06-15-557.log2"), "read", read_offset_0);
			RESPOND_YIELD(RpcValue::stringToBlob(dummy_logfile));
		}

		EXPECT_RESPONSE("All files have been synced");
		REQUIRE(get_cache_contents(cache_dir_path) == expected_cache_contents);
	}

	DOCTEST_SUBCASE("getLog")
	{
		std::string cache_dir_path = "shv/eyas/opc";
		SEND_SITES_YIELD(mock_sites::fin_slave_broker);
		EXPECT_SUBSCRIPTION_YIELD(cache_dir_path, "mntchng");
		EXPECT_SUBSCRIPTION(cache_dir_path, "chng");

		create_dummy_cache_files(cache_dir_path, {
			{ "2022-07-07T18-06-15-557.log2", dummy_logfile },
			{ "2022-07-07T18-06-15-558.log2", dummy_logfile2 }
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

	DOCTEST_SUBCASE("pushLog")
	{
		DOCTEST_SUBCASE("pushing logs")
		{
			std::string cache_dir_path = "shv/pushlog";
			create_dummy_cache_files(cache_dir_path, {});
			SEND_SITES_YIELD(mock_sites::pushlog_hp);
			EXPECT_SUBSCRIPTION(cache_dir_path, "mntchng");

			DOCTEST_SUBCASE("empty log")
			{
				REQUEST_YIELD(cache_dir_path, "pushLog", RpcValue::List());
				EXPECT_RESPONSE(true);
			}

			DOCTEST_SUBCASE("some logging")
			{
				REQUEST_YIELD(cache_dir_path, "pushLog", dummy_pushlog);
				EXPECT_RESPONSE(true);
				REQUIRE(get_cache_contents(cache_dir_path) == RpcValue::List({{
					RpcValue::List{ "2022-07-07T18-06-15-557.log2", 53UL }
				}}));

				DOCTEST_SUBCASE("pushLog rejects older pushlogs")
				{
					REQUEST_YIELD(cache_dir_path, "pushLog", dummy_pushlog2);
					EXPECT_RESPONSE(true);
					REQUIRE(get_cache_contents(cache_dir_path) == RpcValue::List({{
						RpcValue::List{ "2022-07-07T18-06-15-557.log2", 53UL }
					}}));
				}

				DOCTEST_SUBCASE("pushLog rejects pushlogs with same timestamp and same path")
				{
					REQUEST_YIELD(cache_dir_path, "pushLog", dummy_pushlog);
					EXPECT_RESPONSE(true);
					REQUIRE(get_cache_contents(cache_dir_path) == RpcValue::List({{
						RpcValue::List{ "2022-07-07T18-06-15-557.log2", 53UL }
					}}));
				}

				DOCTEST_SUBCASE("pushLog accepts pushlogs with same timestamp and different path")
				{
					REQUEST_YIELD(cache_dir_path, "pushLog", dummy_pushlog3);
					EXPECT_RESPONSE(true);
					REQUIRE(get_cache_contents(cache_dir_path) == RpcValue::List({{
						RpcValue::List{ "2022-07-07T18-06-15-557.log2", 129UL }
					}}));
				}

			}
		}

		DOCTEST_SUBCASE("HP directly above a pushlog don't have a syncLog method" )
		{
			std::string cache_dir_path = "shv/pushlog";
			SEND_SITES_YIELD(mock_sites::pushlog_hp);
			EXPECT_SUBSCRIPTION(cache_dir_path, "mntchng");
			REQUEST_YIELD(cache_dir_path, "syncLog", RpcValue());
			EXPECT_ERROR("RPC ERROR MethodCallException: Method: 'syncLog' on path 'shv/pushlog/' doesn't exist.");
		}

		DOCTEST_SUBCASE("master HP can syncLog pushlogs from slave HPs")
		{
			std::string cache_dir_path = "shv/master";
			SEND_SITES_YIELD(mock_sites::master_hp_with_slave_pushlog);
			EXPECT_SUBSCRIPTION_YIELD(cache_dir_path, "mntchng");
			EXPECT_SUBSCRIPTION(cache_dir_path, "chng");
			REQUEST_YIELD("shvjournal", "syncLog", RpcValue());
			EXPECT_REQUEST("shv/master/.local/history/shvjournal", "lsfiles", ls_size_true);
			RESPOND_YIELD(RpcValue::List());
			EXPECT_RESPONSE("All files have been synced");
		}

		DOCTEST_SUBCASE("periodic syncing")
		{
			HistoryApp::instance()->cliOptions()->setLogMaxAge(1);
			DOCTEST_SUBCASE("master HP")
			{
				std::string cache_dir_path = "shv/master/pushlog";
				SEND_SITES_YIELD(mock_sites::master_hp_with_slave_pushlog);
				EXPECT_SUBSCRIPTION_YIELD("shv/master", "mntchng");
				EXPECT_SUBSCRIPTION_YIELD("shv/master", "chng");
				// Test that HP will run lsfiles (sync) at least twice.
				EXPECT_REQUEST("shv/master/.local/history/shvjournal/pushlog", "lsfiles", ls_size_true);
				RESPOND_YIELD(RpcValue::List());
				EXPECT_REQUEST("shv/master/.local/history/shvjournal/pushlog", "lsfiles", ls_size_true);
				RESPOND(RpcValue::List());
			}

			DOCTEST_SUBCASE("slave HP shouldn't sync")
			{
				SEND_SITES_YIELD(mock_sites::pushlog_hp);
				EXPECT_SUBSCRIPTION("shv/pushlog", "mntchng");
				DRIVER_WAIT(1500);
				// Slave pushlog shouldn't sync. We should not have any messages here.
				CAPTURE(m_messageQueue.head());
				REQUIRE(m_messageQueue.empty());
			}
		}

		DOCTEST_SUBCASE("master HP should sync on mntchng")
		{
			SEND_SITES_YIELD(mock_sites::master_hp_with_slave_pushlog);
			EXPECT_SUBSCRIPTION_YIELD("shv/master", "mntchng");
			EXPECT_SUBSCRIPTION("shv/master", "chng");
			NOTIFY_YIELD("shv/master", "mntchng", true);
			EXPECT_REQUEST("shv/master/.local/history/shvjournal", "lsfiles", ls_size_true);
			RESPOND(RpcValue::List()); // We only test if the syncLog triggers.
		}

		DOCTEST_SUBCASE("slave HP shouldn't sync on mntchng")
		{
			SEND_SITES_YIELD(mock_sites::pushlog_hp);
			EXPECT_SUBSCRIPTION("shv/pushlog", "mntchng");
			NOTIFY("shv/pushlog", "mntchng", true);
			DRIVER_WAIT(100);
			CAPTURE(m_messageQueue.head());
			REQUIRE(m_messageQueue.empty());
		}
	}

	DOCTEST_SUBCASE("legacy getLog retrieval")
	{
		std::string cache_dir_path = "shv/legacy";
		SEND_SITES_YIELD(mock_sites::legacy_hp);
		EXPECT_SUBSCRIPTION_YIELD(cache_dir_path, "mntchng");
		EXPECT_SUBSCRIPTION(cache_dir_path, "chng");

		RpcValue::List expected_cache_contents;

		DOCTEST_SUBCASE("empty response")
		{
			create_dummy_cache_files(cache_dir_path, {});
			REQUEST_YIELD("shvjournal", "syncLog", RpcValue());
			EXPECT_REQUEST(cache_dir_path, "getLog");
			RESPOND_YIELD(shv::chainpack::RpcValue::List());
		}

		DOCTEST_SUBCASE("some response")
		{
			create_dummy_cache_files(cache_dir_path, {});
			REQUEST_YIELD("shvjournal", "syncLog", RpcValue());
			EXPECT_REQUEST(cache_dir_path, "getLog");
			RESPOND_YIELD(dummy_getlog_response);
			expected_cache_contents = RpcValue::List({{
				RpcValue::List{ "2022-07-07T18-06-15-557.log2", 201UL }
			}});
		}

		DOCTEST_SUBCASE("since param")
		{
			HistoryApp::instance()->cliOptions()->setCacheInitMaxAge(60 /*seconds*/ * 60 /*minutes*/ * 24 /*hours*/ * 30 /*days*/);
			DOCTEST_SUBCASE("empty cache")
			{
				create_dummy_cache_files(cache_dir_path, {});
				REQUEST_YIELD("shvjournal", "syncLog", RpcValue());
				EXPECT_REQUEST(cache_dir_path, "getLog");
				auto since_param_ms = shv::chainpack::RpcRequest(m_messageQueue.head()).params().asMap().value("since").toDateTime().msecsSinceEpoch();
				auto now_ms = shv::chainpack::RpcValue::DateTime::now().msecsSinceEpoch();
				REQUIRE(now_ms - since_param_ms < int64_t{1000} /*ms*/ * 60 /*seconds*/ * 60 /*minutes*/ * 24 /*hours*/ * 31 /*days*/ );
			}

			DOCTEST_SUBCASE("something in cache")
			{
				create_dummy_cache_files(cache_dir_path, {
					{ "2022-07-07T18-06-15-557.log2", dummy_logfile },
				});
				REQUEST_YIELD("shvjournal", "syncLog", RpcValue());
				EXPECT_REQUEST(cache_dir_path, "getLog");
				auto since_param_ms = shv::chainpack::RpcRequest(m_messageQueue.head()).params().asMap().value("since").toDateTime().msecsSinceEpoch();
				REQUIRE(since_param_ms == shv::chainpack::RpcValue::DateTime::fromUtcString("2022-07-07T18:06:17.870Z").msecsSinceEpoch());
				expected_cache_contents = RpcValue::List({{
					RpcValue::List{ "2022-07-07T18-06-15-557.log2", 308UL }
				}});
			}

			DOCTEST_SUBCASE("only dirty in cache")
			{
				create_dummy_cache_files(cache_dir_path, {
					{ "dirty.log2", dummy_logfile },
				});
				REQUEST_YIELD("shvjournal", "syncLog", RpcValue());
				EXPECT_REQUEST(cache_dir_path, "getLog");
				auto since_param_ms = shv::chainpack::RpcRequest(m_messageQueue.head()).params().asMap().value("since").toDateTime().msecsSinceEpoch();
				auto now_ms = shv::chainpack::RpcValue::DateTime::now().msecsSinceEpoch();
				REQUIRE(now_ms - since_param_ms < int64_t{1000} /*ms*/ * 60 /*seconds*/ * 60 /*minutes*/ * 24 /*hours*/ * 31 /*days*/ );
				expected_cache_contents = RpcValue::List({{
					RpcValue::List{ "dirty.log2", 308UL }
				}});
			}

			RESPOND_YIELD(shv::chainpack::RpcValue::List());
		}

		EXPECT_RESPONSE("All files have been synced");
		REQUIRE(get_cache_contents(cache_dir_path) == expected_cache_contents);
	}

	DOCTEST_SUBCASE("sanitizer")
	{
		RpcValue::List expected_cache_contents;
		DOCTEST_SUBCASE("sanitizeLog correctly removes stuff")
		{
			std::string cache_dir_path = "shv/eyas/opc";
			SEND_SITES_YIELD(mock_sites::fin_slave_broker);
			EXPECT_SUBSCRIPTION_YIELD(cache_dir_path, "mntchng");
			EXPECT_SUBSCRIPTION(cache_dir_path, "chng");

			create_dummy_cache_files(cache_dir_path, {
				{ "2022-07-07T18-06-15-557.log2", dummy_logfile },
				{ "2022-07-07T18-06-15-560.log2", dummy_logfile },
			});

			DOCTEST_SUBCASE("when size is within quota")
			{
				HistoryApp::instance()->setSingleCacheSizeLimit(5000);
				expected_cache_contents = RpcValue::List({{
					RpcValue::List{ "2022-07-07T18-06-15-557.log2", 308UL },
					RpcValue::List{ "2022-07-07T18-06-15-560.log2", 308UL }
				}});
			}

			DOCTEST_SUBCASE("when size is over quota")
			{
				HistoryApp::instance()->setSingleCacheSizeLimit(500);
				expected_cache_contents = RpcValue::List({{
					RpcValue::List{ "2022-07-07T18-06-15-560.log2", 308UL }
				}});
			}

			REQUEST_YIELD(cache_dir_path, "sanitizeLog", RpcValue());
			EXPECT_RESPONSE("Cache sanitization done");
			REQUIRE(get_cache_contents(cache_dir_path) == expected_cache_contents);
		}

		DOCTEST_SUBCASE("sanitizer runs periodically")
		{
			HistoryApp::instance()->cliOptions()->setJournalSanitizerInterval(1);
			SEND_SITES_YIELD(mock_sites::two_devices);
			EXPECT_SUBSCRIPTION_YIELD("shv/one", "mntchng");
			EXPECT_SUBSCRIPTION_YIELD("shv/one", "chng");
			EXPECT_SUBSCRIPTION_YIELD("shv/two", "mntchng");
			EXPECT_SUBSCRIPTION("shv/two", "chng");

			HistoryApp::instance()->setSingleCacheSizeLimit(500);

			create_dummy_cache_files("shv/one", {
				{ "2022-07-07T18-06-15-557.log2", dummy_logfile },
				{ "2022-07-07T18-06-15-560.log2", dummy_logfile },
			});
			create_dummy_cache_files("shv/two", {
				{ "2022-07-07T18-06-15-557.log2", dummy_logfile },
				{ "2022-07-07T18-06-15-560.log2", dummy_logfile },
			});
			expected_cache_contents = RpcValue::List({{
				RpcValue::List{ "2022-07-07T18-06-15-560.log2", 308UL }
			}});

			DRIVER_WAIT(3000);

			REQUIRE(get_cache_contents("shv/one") == expected_cache_contents);
			REQUIRE(get_cache_contents("shv/two") == expected_cache_contents);
		}
	}

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
