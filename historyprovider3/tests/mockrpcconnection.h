#pragma once
#include <shv/coreqt/log.h>
#include <shv/iotqt/rpc/deviceconnection.h>
#include <doctest/doctest.h>

#include <QCoroGenerator>

#include <QTimer>
#include <QQueue>

#define mockDebug() shvCDebug("MockRpcConnection")
#define mockInfo() shvCInfo("MockRpcConnection")
#define mockError() shvCError("MockRpcConnection")

class MockRpcConnection : public shv::iotqt::rpc::DeviceConnection {
	Q_OBJECT

	using Super = shv::iotqt::rpc::DeviceConnection;

private:
	shv::chainpack::RpcResponse createResponse(const shv::chainpack::RpcValue& result);
	void doRespond(const shv::chainpack::RpcValue& result);
	void doRespondInEventLoop(const shv::chainpack::RpcValue& result);
	shv::chainpack::RpcRequest createRequest(const std::string& path, const std::string& method, const shv::chainpack::RpcValue& params);
	void doRequest(const std::string& path, const std::string& method, const shv::chainpack::RpcValue& params);
	void doRequestInEventLoop(const std::string& path, const std::string& method, const shv::chainpack::RpcValue& params);
	shv::chainpack::RpcSignal createNotification(const std::string& path, const std::string& method, const shv::chainpack::RpcValue& params);
	void doNotify(const std::string& path, const std::string& method, const shv::chainpack::RpcValue& params);
	void doNotifyInEventLoop(const std::string& path, const std::string& method, const shv::chainpack::RpcValue& params);
	void advanceTest();

	void setBrokerConnected(bool state);

	QCoro::Generator<int> m_testDriver;
	QCoro::Generator<int>::iterator m_testDriverState;
	QQueue<shv::chainpack::RpcMessage> m_messageQueue;
	bool m_coroRunning = false;
	QTimer* m_timeoutTimer = nullptr;
public:
	MockRpcConnection();
    void open() override;
	Q_SIGNAL void handleNextMessage();
	// I can't return void from the generator, so I'm returning ints.
	QCoro::Generator<int> driver();
    void sendMessage(const shv::chainpack::RpcMessage& rpc_msg) override;
};

const auto COROUTINE_TIMEOUT = 3000;

shv::chainpack::RpcValue make_sub_params(const std::string& path, const std::string& method);

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

#define EXPECT_SIGNAL(pathStr, methodStr, ...) { \
	REQUIRE(!m_messageQueue.empty()); \
	CAPTURE(m_messageQueue.head()); \
	REQUIRE(m_messageQueue.head().isSignal()); \
	REQUIRE(m_messageQueue.head().shvPath().asString() == (pathStr)); \
	REQUIRE(m_messageQueue.head().method().asString() == (methodStr)); \
	__VA_OPT__(REQUIRE(shv::chainpack::RpcRequest(m_messageQueue.head()).params() == (__VA_ARGS__));) \
	m_messageQueue.dequeue(); \
}

#define EXPECT_ERROR(expectedMsg) { \
	REQUIRE(!m_messageQueue.empty()); \
	CAPTURE(m_messageQueue.head()); \
	REQUIRE(m_messageQueue.head().isResponse()); \
	REQUIRE(shv::chainpack::RpcResponse(m_messageQueue.head()).errorString() == (expectedMsg)); \
	m_messageQueue.dequeue(); \
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
