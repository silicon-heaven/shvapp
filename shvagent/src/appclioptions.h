#pragma once

#include <shv/iotqt/rpc/clientappclioptions.h>

#include <QSet>

class AppCliOptions : public shv::iotqt::rpc::ClientAppCliOptions
{
	Q_OBJECT
private:
	using Super = shv::iotqt::rpc::ClientAppCliOptions;

	//CLIOPTION_GETTER_SETTER(QString, l, setL, ocale)
public:
	AppCliOptions(QObject *parent = NULL);
	~AppCliOptions() Q_DECL_OVERRIDE {}
};

