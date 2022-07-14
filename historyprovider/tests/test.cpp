#include "src/appclioptions.h"
#include "src/historyapp.h"
#include "tests/sites.h"

#include <shv/coreqt/log.h>
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

	void doRequest(const std::string& path, const std::string& method)
	{
		cp::RpcRequest req;
		req.setRequestId(nextRequestId());
		req.setAccessGrant(shv::chainpack::Rpc::ROLE_ADMIN);
		req.setShvPath(path);
		req.setMethod(method);
		mockInfo() << "Sending request:" << req.toPrettyString();
		emit rpcMessageReceived(req);
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

			if (m_testDriverState == m_testDriver.end()) {
				// We'll wait a second before ending to make sure the client isn't sending more messages.
				QTimer::singleShot(1000, [] {
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

	void expectResponse(const RpcValue& result)
	{
		REQUIRE(m_messageQueue.head().isResponse());
		REQUIRE(shv::chainpack::RpcResponse(m_messageQueue.head()).result() == result);
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

#define REQUEST(path, method) { \
	doRequest((path), (method)); \
	co_yield {}; \
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

auto get_cache_content(const std::string& site_path)
{
	RpcValue::List res;
	auto cache_dir = get_site_cache_dir(site_path);
	auto entries = cache_dir.entryList(QDir::NoDotAndDotDot);
	std::transform(entries.begin(), entries.end(), std::back_inserter(res), [] (const auto& entry) {
		RpcValue::List name_and_size;
		name_and_size.push_back(entry.toStdString());
		name_and_size.push_back(RpcValue::Int(QFile(entry).size()));
		return name_and_size;
	});

	return res;
}

auto join(const std::string& a, const std::string& b)
{
	return shv::core::Utils::joinPath(a, b);
}

QCoro::Generator<int> MockRpcConnection::driver()
try
{
	co_yield {};
	EXPECT_REQUEST("sites", "getSites");
	RESPOND(mock_sites::fin_slave_broker_sites);
	EXPECT_REQUEST(".broker/app", "subscribe");
	doRespond(true);

	DOCTEST_SUBCASE("syncLog")
	{
		RpcValue::List expected_cache_contents;
		std::string cache_path;

		DOCTEST_SUBCASE("fin slave HP")
		{
			cache_path = "shv/eyas/opc";
			REQUEST(join(cache_path, "shvjournal"), "syncLog");
			EXPECT_REQUEST(join(cache_path, "/.app/shvjournal"), "ls");

			DOCTEST_SUBCASE("Local cache empty")
			{
				remove_cache_contents(cache_path);
				DOCTEST_SUBCASE("Remote log is empty")
				{
					RESPOND(RpcValue::List());
				}
			}
		}

		EXPECT_RESPONSE("All files have been synced");
		REQUIRE(get_cache_contents(cache_path) == expected_cache_contents);
	}

	co_return;
} catch (...) {
	std::terminate();
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
