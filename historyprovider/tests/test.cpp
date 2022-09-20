#include "src/appclioptions.h"
#include "src/historyapp.h"
#include "tests/sites.h"

#include <shv/coreqt/log.h>
#include <shv/core/utils/shvjournalentry.h>
#include <shv/core/utils/shvmemoryjournal.h>
#include <shv/iotqt/rpc/deviceconnection.h>
#include <QCoroGenerator>

#include <QDir>
#include <QQueue>
#include <QTimer>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

namespace cp = shv::chainpack;
using cp::RpcValue;

#define mockDebug() shvCDebug("MockRpcConnection")
#define mockInfo() shvCInfo("MockRpcConnection")
#define mockError() shvCError("MockRpcConnection")

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

class MockRpcConnection : public shv::iotqt::rpc::DeviceConnection {
	Q_OBJECT

	using Super = shv::iotqt::rpc::DeviceConnection;

private:
	auto createResponse(const RpcValue& result)
	{
		cp::RpcResponse res;
		res.setRequestId(m_messageQueue.head().requestId());
		res.setResult(result);
		mockInfo() << "Sending response:" << res.toPrettyString();
		m_messageQueue.dequeue();
		return res;
	}

	void doRespond(const RpcValue& result)
	{
		emit rpcMessageReceived(createResponse(result));
	}

	void doRespondInEventLoop(const RpcValue& result)
	{
		QTimer::singleShot(0, [this, response = createResponse(result)] {emit rpcMessageReceived(response);});
	}

	auto createRequest(const std::string& path, const std::string& method, const RpcValue& params)
	{
		cp::RpcRequest req;
		req.setRequestId(nextRequestId());
		req.setAccessGrant(shv::chainpack::Rpc::ROLE_ADMIN);
		req.setShvPath(path);
		req.setParams(params);
		req.setMethod(method);
		mockInfo() << "Sending request:" << req.toPrettyString();
		return req;
	}

	void doRequest(const std::string& path, const std::string& method, const RpcValue& params)
	{
		emit rpcMessageReceived(createRequest(path, method, params));
	}

	void doRequestInEventLoop(const std::string& path, const std::string& method, const RpcValue& params)
	{
		QTimer::singleShot(0, [this, request = createRequest(path, method, params)] {emit rpcMessageReceived(request);});
	}

	auto createNotification(const std::string& path, const std::string& method, const RpcValue& params)
	{
		cp::RpcSignal sig;
		sig.setShvPath(path);
		sig.setMethod(method);
		sig.setParams(params);
		mockInfo() << "Sending signal:" << sig.toPrettyString();
		return sig;
	}

	void doNotify(const std::string& path, const std::string& method, const RpcValue& params)
	{
		emit rpcMessageReceived(createNotification(path, method, params));
	}

	void doNotifyInEventLoop(const std::string& path, const std::string& method, const RpcValue& params)
	{
		QTimer::singleShot(0, [this, notification = createNotification(path, method, params)] {emit rpcMessageReceived(notification);});
	}

	void advanceTest()
	{
		if (m_testDriverState == m_testDriver.end()) {
			CAPTURE("Client sent unexpected message after test end");
			CAPTURE(m_messageQueue.head());
			REQUIRE(false);
		}

		if (m_timeoutTimer) {
			m_timeoutTimer->stop();
			m_timeoutTimer->deleteLater();
			m_timeoutTimer = nullptr;
		}

		m_coroRunning = true;
		++m_testDriverState;
		m_coroRunning = false;
		if (m_testDriverState != m_testDriver.end()) {
			// I also need to dereference the value to trigger any unhandled exceptions.
			*m_testDriverState;
		}

		if (m_testDriverState == m_testDriver.end()) {
			// We'll wait a bit before ending to make sure the client isn't sending more messages.
			QTimer::singleShot(100, [] {
				HistoryApp::instance()->exit();
			});
		}
	}

	QCoro::Generator<int> m_testDriver;
	QCoro::Generator<int>::iterator m_testDriverState;
	QQueue<cp::RpcMessage> m_messageQueue;
	bool m_coroRunning = false;

	QTimer* m_timeoutTimer = nullptr;
public:
	MockRpcConnection()
		: shv::iotqt::rpc::DeviceConnection(nullptr)
		, m_testDriver(driver())
		, m_testDriverState(m_testDriver.begin())
	{
		connect(this, &MockRpcConnection::handleNextMessage, this, &MockRpcConnection::advanceTest, Qt::QueuedConnection);
	}

    void open() override
    {
        mockInfo() << "Client connected";
        m_connectionState.state = State::BrokerConnected;
        emit brokerConnectedChanged(true);
    }

	Q_SIGNAL void handleNextMessage();

	// I can't return void from the generator, so I'm returning ints.
	QCoro::Generator<int> driver();

    void sendMessage(const cp::RpcMessage& rpc_msg) override
    {
		auto msg_type =
			rpc_msg.isRequest() ? "message:" :
			rpc_msg.isResponse() ? "response:" :
			rpc_msg.isSignal() ? "signal:" :
			"<unknown message type>:";
		mockInfo() << "Got client" << msg_type << rpc_msg.toPrettyString();

		if (m_coroRunning) {
			throw std::logic_error("A client send a message while the test driver was running."
					" This can lead to unexpected behavior, because the test driver resumes on messages from the client and you can't resume the driver while it's already running.");
		}

		// For now we're not handling signals at all, so we'll skip them.
		if (rpc_msg.isSignal()) {
			return;
		}

		m_messageQueue.enqueue(rpc_msg);

		emit handleNextMessage();
    }
};

const auto COROUTINE_TIMEOUT = 3000;

#define SETUP_TIMEOUT { \
	m_timeoutTimer = new QTimer(); \
	connect(m_timeoutTimer, &QTimer::timeout, [] {throw std::runtime_error("The test timed out while waiting for a message from client.");}); \
	m_timeoutTimer->start(COROUTINE_TIMEOUT); \
}

#define REQUEST_YIELD(path, method, params) { \
	doRequestInEventLoop((path), (method), (params)); \
	SETUP_TIMEOUT; \
	co_yield {}; \
}

#define REQUEST(path, method, params) { \
	doRequest((path), (method), (params)); \
}

#define NOTIFY_YIELD(path, method, params) { \
	doNotifyInEventLoop((path), (method), (params)); \
	SETUP_TIMEOUT; \
	co_yield {}; \
}

#define NOTIFY(path, method, params) { \
	doNotify((path), (method), (params)); \
}

#define RESPOND(result) { \
	doRespond(result); \
}

#define RESPOND_YIELD(result) { \
	doRespondInEventLoop(result); \
	SETUP_TIMEOUT; \
	co_yield {}; \
}

#define EXPECT_REQUEST(pathStr, methodStr, ...) { \
	REQUIRE(!m_messageQueue.empty()); \
	CAPTURE(m_messageQueue.head()); \
	REQUIRE(m_messageQueue.head().isRequest()); \
	REQUIRE(m_messageQueue.head().shvPath().asString() == (pathStr)); \
	REQUIRE(m_messageQueue.head().method().asString() == (methodStr)); \
	__VA_OPT__(REQUIRE(shv::chainpack::RpcRequest(m_messageQueue.head()).params() == (__VA_ARGS__));) \
}

#define EXPECT_RESPONSE(expectedResult) { \
	REQUIRE(!m_messageQueue.empty()); \
	CAPTURE(m_messageQueue.head()); \
	REQUIRE(m_messageQueue.head().isResponse()); \
	REQUIRE(shv::chainpack::RpcResponse(m_messageQueue.head()).result() == (expectedResult)); \
	m_messageQueue.dequeue(); \
}

#define EXPECT_ERROR(expectedMsg) { \
	REQUIRE(!m_messageQueue.empty()); \
	CAPTURE(m_messageQueue.head()); \
	REQUIRE(m_messageQueue.head().isResponse()); \
	REQUIRE(shv::chainpack::RpcResponse(m_messageQueue.head()).errorString() == (expectedMsg)); \
	m_messageQueue.dequeue(); \
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

#define EXPECT_SUBSCRIPTION(path, method) { \
	EXPECT_REQUEST(".broker/app", "subscribe", make_sub_params(path, method)); \
	RESPOND(true); \
}

#define EXPECT_SUBSCRIPTION_YIELD(path, method) { \
	EXPECT_REQUEST(".broker/app", "subscribe", make_sub_params(path, method)); \
	RESPOND_YIELD(true); \
}

#define SEND_SITES_YIELD_AND_HANDLE_SUB(sitesStr, subPath) { \
	EXPECT_REQUEST("sites", "getSites", RpcValue()); \
	RESPOND_YIELD(sitesStr); \
	EXPECT_SUBSCRIPTION(subPath, "chng"); \
}

#define SEND_SITES_YIELD(sitesStr) { \
	EXPECT_REQUEST("sites", "getSites", RpcValue()); \
	RESPOND_YIELD(sitesStr); \
}

#define SEND_SITES(sitesStr) { \
	EXPECT_REQUEST("sites", "getSites", RpcValue()); \
	RESPOND(sitesStr); \
}

#define DRIVER_WAIT(msec) { \
	QTimer::singleShot(msec, [this] { advanceTest(); }); \
	co_yield {}; \
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
						RpcValue::List{ "dirty.log2", 101ul }
					}});
				}

				EXPECT_RESPONSE("All files have been synced");
				REQUIRE(get_cache_contents(cache_dir_path) == expected_cache_contents);
			}

			DOCTEST_SUBCASE("Don't download files older than we already have")
			{
				expected_cache_contents = RpcValue::List({{
					RpcValue::List{ "2022-07-07T18-06-15-557.log2", 0ul },
					RpcValue::List{ "2022-07-07T18-06-15-558.log2", 0ul }
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
				RpcValue::List{ "2022-07-07T18-06-15-557.log2", 201ul }
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
					RpcValue::List{ "2022-07-07T18-06-15-557.log2", 308ul }
				}});
			}

			RESPOND_YIELD(shv::chainpack::RpcValue::List());
		}

		EXPECT_RESPONSE("All files have been synced");
		REQUIRE(get_cache_contents(cache_dir_path) == expected_cache_contents);
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

#include "test.moc"
