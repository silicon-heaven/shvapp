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
	void doRespond(const RpcValue& result)
	{
		cp::RpcResponse res;
		res.setRequestId(m_messageQueue.head().requestId());
		res.setResult(result);
		mockInfo() << "Sending response:" << res.toPrettyString();
		m_messageQueue.dequeue();
		emit rpcMessageReceived(res);
	}

	void doRequest(const std::string& path, const std::string& method, const RpcValue& params)
	{
		cp::RpcRequest req;
		req.setRequestId(nextRequestId());
		req.setAccessGrant(shv::chainpack::Rpc::ROLE_ADMIN);
		req.setShvPath(path);
		req.setParams(params);
		req.setMethod(method);
		mockInfo() << "Sending request:" << req.toPrettyString();
		emit rpcMessageReceived(req);
	}

	void doNotify(const std::string& path, const RpcValue& params)
	{
		cp::RpcSignal sig;
		sig.setShvPath(path);
		sig.setMethod("chng");
		sig.setParams(params);
		mockInfo() << "Sending signal:" << sig.toPrettyString();
		emit rpcMessageReceived(sig);
	}

	QCoro::Generator<int> m_testDriver;
	QCoro::Generator<int>::iterator m_testDriverState;
	QQueue<cp::RpcMessage> m_messageQueue;

public:
	MockRpcConnection()
		: shv::iotqt::rpc::DeviceConnection(nullptr)
		, m_testDriver(driver())
		, m_testDriverState(m_testDriver.begin())
	{
		connect(this, &MockRpcConnection::handleNextMessage, this, [this] {
			if (m_testDriverState == m_testDriver.end()) {
				CAPTURE("Client sent unexpected message after test end");
				CAPTURE(m_messageQueue.head());
				REQUIRE(false);
			}
			++m_testDriverState;
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
		}, Qt::QueuedConnection);
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

		// For now we're not handling testing at all, so we'll skip them.
		if (rpc_msg.isSignal()) {
			return;
		}

		m_messageQueue.enqueue(rpc_msg);
		emit handleNextMessage();
    }
};

#define REQUEST(path, method, params) { \
	doRequest((path), (method), (params)); \
	co_yield {}; \
}

#define NOTIFY(path, params) { \
	doNotify((path), (params)); \
}

#define RESPOND(result) { \
	doRespond(result); \
	co_yield {}; \
}

#define EXPECT_REQUEST(pathStr, methodStr) { \
	CAPTURE(m_messageQueue.head()); \
	REQUIRE(m_messageQueue.head().isRequest()); \
	REQUIRE(m_messageQueue.head().shvPath().asString() == (pathStr)); \
	REQUIRE(m_messageQueue.head().method().asString() == (methodStr)); \
}

#define EXPECT_RESPONSE(expectedResult) { \
	CAPTURE(m_messageQueue.head()); \
	REQUIRE(m_messageQueue.head().isResponse()); \
	REQUIRE(shv::chainpack::RpcResponse(m_messageQueue.head()).result() == (expectedResult)); \
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

#define SEND_SITES(sitesStr) { \
	EXPECT_REQUEST("sites", "getSites"); \
	RESPOND(sitesStr); \
	EXPECT_REQUEST(".broker/app", "subscribe"); \
	doRespond(true); \
}

QCoro::Generator<int> MockRpcConnection::driver()
{
	co_yield {};

	std::string cache_dir_path;
	DOCTEST_SUBCASE("syncLog")
	{
		RpcValue::List expected_cache_contents;

		DOCTEST_SUBCASE("fin slave HP")
		{
			SEND_SITES(mock_sites::fin_slave_broker_sites);
			cache_dir_path = "shv/eyas/opc";
			REQUEST(join(cache_dir_path, "shvjournal"), "syncLog", RpcValue());
			EXPECT_REQUEST(join(cache_dir_path, "/.app/shvjournal"), "lsfiles");

			DOCTEST_SUBCASE("Remote and local - empty")
			{
				create_dummy_cache_files(cache_dir_path, {});
				RESPOND(RpcValue::List());
			}

			DOCTEST_SUBCASE("Remote - has files, local - empty")
			{
				create_dummy_cache_files(cache_dir_path, {});
				expected_cache_contents = RpcValue::List({{
					RpcValue::List{ "2022-07-07T18-06-15-557.log2", dummy_logfile.size() }
				}});
				RESPOND((RpcValue::List({{
					{ "2022-07-07T18-06-15-557.log2", "f", dummy_logfile.size() }
				}})));

				EXPECT_REQUEST("shv/eyas/opc/.app/shvjournal/2022-07-07T18-06-15-557.log2", "read");
				RESPOND(RpcValue::stringToBlob(dummy_logfile));
			}

			DOCTEST_SUBCASE("dirty log")
			{
				create_dummy_cache_files(cache_dir_path, {});
				// Respond to the initial `lsfiles` request.
				RESPOND(RpcValue::List());

				DOCTEST_SUBCASE("HP accepts events and puts them into the log")
				{
					NOTIFY("shv/eyas/opc/power-on", true);
					NOTIFY("shv/eyas/opc/power-on", false);

					expected_cache_contents = RpcValue::List({{
						RpcValue::List{ "dirty.log2", 101ul }
					}});
				}
			}

		}

		DOCTEST_SUBCASE("syncing from slave HP")
		{
			SEND_SITES(mock_sites::fin_master_broker_sites);
			cache_dir_path = "shv/fin/hel/tram/hel002/eyas/opc";
			auto master_shv_journal_path = join(cache_dir_path, "shvjournal");
			auto slave_shv_journal_path = "shv/fin/hel/tram/hel002/.local/history/shv/eyas/opc/shvjournal";
			REQUEST(master_shv_journal_path, "syncLog", RpcValue());
			EXPECT_REQUEST(slave_shv_journal_path, "lsfiles");

			DOCTEST_SUBCASE("Remote - has files, local - empty")
			{
				// NOTE: I'm only really testing whether historyprovider correctly enters the .local broker.
				create_dummy_cache_files(cache_dir_path, {});
				expected_cache_contents = RpcValue::List({{
					RpcValue::List{ "2022-07-07T18-06-15-557.log2", dummy_logfile.size() }
				}});
				RESPOND((RpcValue::List{{
					{ "2022-07-07T18-06-15-557.log2", "f", dummy_logfile.size() }
				}}));

				EXPECT_REQUEST(join(slave_shv_journal_path, "2022-07-07T18-06-15-557.log2"), "read");
				RESPOND(RpcValue::stringToBlob(dummy_logfile));
			}
		}

		EXPECT_RESPONSE("All files have been synced");
		REQUIRE(get_cache_contents(cache_dir_path) == expected_cache_contents);
	}

	DOCTEST_SUBCASE("getLog")
	{
		SEND_SITES(mock_sites::fin_slave_broker_sites);

		cache_dir_path = "shv/eyas/opc";
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

		REQUEST(join(cache_dir_path, "shvjournal"), "getLog", get_log_params.toRpcValue());

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

	co_return;
}

DOCTEST_TEST_CASE("HistoryApp")
{
	NecroLog::registerTopic("MockRpcConnection", "");
	NecroLog::setTopicsLogTresholds(":D");
	QCoreApplication::setApplicationName("historyprovider tester");

	int argc;
	char* argv[1];
	AppCliOptions cli_opts;
	HistoryApp app(argc, argv, &cli_opts, new MockRpcConnection());
	app.exec();
}

#include "test.moc"
