#include "theapp.h"
#include "servertreemodel/servertreemodel.h"
#include "attributesmodel/attributesmodel.h"
#include "subscriptionsmodel/subscriptionsmodel.h"

#include <QSettings>

TheApp::TheApp(int &argc, char **argv, AppCliOptions *cli_opts)
	: Super(argc, argv)
	, m_cliOptions(cli_opts)
	, m_crypt(shv::core::utils::Crypt::createGenerator(17456, 3148, 2147483647))
{
	m_serverTreeModel = new ServerTreeModel(this);
	m_attributesModel = new AttributesModel(this);
	//m_subscriptionsModel = new SubscriptionsModel(this);
}

TheApp::~TheApp()
{
}

