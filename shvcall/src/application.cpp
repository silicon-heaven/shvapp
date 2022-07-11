#include "application.h"
#include "appclioptions.h"

#include <shv/iotqt/rpc/rpccall.h>
#include <shv/iotqt/rpc/rpc.h>
#include <shv/coreqt/log.h>

namespace cp = shv::chainpack;
namespace si = shv::iotqt;

Application::Application(int &argc, char **argv, AppCliOptions *cli_opts)
	: Super(argc, argv)
	, m_cliOptions(cli_opts)
	, m_status(EXIT_SUCCESS)
{
	m_rpcConnection = new si::rpc::ClientConnection(this);

	if(!cli_opts->user_isset())
		cli_opts->setUser("iot");
	if(!cli_opts->password_isset())
		cli_opts->setPassword("lub42DUB");
	m_rpcConnection->setCliOptions(cli_opts);

	connect(m_rpcConnection, &si::rpc::ClientConnection::stateChanged, this, &Application::onShvStateChanged);
	shvInfo() << "Connecting to SHV Broker";
	m_rpcConnection->open();
}

QCoro::Task<> Application::onShvStateChanged()
{
	if (m_rpcConnection->state() == si::rpc::ClientConnection::State::BrokerConnected) {
		shvInfo() << "SHV Broker connected";
		si::rpc::RpcCall *call = si::rpc::RpcCall::create(m_rpcConnection)
								 ->setShvPath(QString::fromStdString(m_cliOptions->path()))
								 ->setMethod(QString::fromStdString(m_cliOptions->method()))
								 ->setParams(m_cliOptions->params().empty() ? cp::RpcValue() : cp::RpcValue::fromCpon(m_cliOptions->params()));
		call->start();
		auto [result, error] = co_await qCoro(call, &si::rpc::RpcCall::maybeResult);
		if (!error.isEmpty()) {
			shvInfo() << error;
			m_status = EXIT_FAILURE;
			m_rpcConnection->close();
			co_return;
		}

		printf("%s\n", result.toCpon("\t").c_str());
		m_rpcConnection->close();
	}
	else if (m_rpcConnection->state() == shv::iotqt::rpc::ClientConnection::State::NotConnected) {
		shvInfo() << "SHV Broker disconnected";
		exit(m_status);
	}
}
