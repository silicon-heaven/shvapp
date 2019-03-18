#include "thresholdsdialog.h"
#include "ui_thresholdsdialog.h"
#include "jn50viewapp.h"
#include "appclioptions.h"

#include <shv/iotqt/rpc/clientconnection.h>
#include <shv/iotqt/rpc/rpcresponsecallback.h>

#include <shv/coreqt/log.h>

namespace cp = shv::chainpack;

ThresholdsDialog::ThresholdsDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::ThresholdsDialog)
{
	ui->setupUi(this);

	loadTreshold("voltageTreshold", ui->edU);
	loadTreshold("currentTreshold", ui->edI);
	loadTreshold("powerTreshold", ui->edW);
	loadTreshold("energyTreshold", ui->edE);
}

ThresholdsDialog::~ThresholdsDialog()
{
	delete ui;
}

void ThresholdsDialog::loadTreshold(const std::string &shv_name, QSpinBox *editor)
{
	Jn50ViewApp *app = Jn50ViewApp::instance();
	std::string path = app->cliOptions()->converterShvPath() + "/settings/" + shv_name;
	shv::iotqt::rpc::ClientConnection *conn = app->rpcConnection();
	int rq_id = conn->callShvMethod(path, cp::Rpc::METH_GET);
	shv::iotqt::rpc::RpcResponseCallBack *cb = new shv::iotqt::rpc::RpcResponseCallBack(conn, rq_id, this);
	connect(cb, &shv::iotqt::rpc::RpcResponseCallBack::finished, this, [editor, path](const cp::RpcResponse &resp) {
		if(resp.isValid()) {
			if(resp.isError())
				shvWarning() << "GET" << path << "RPC request error:" << resp.error().toString();
			else
				editor->setValue(resp.result().toInt());
		}
		else {
			shvWarning() << "GET" << path << "RPC request timeout";
		}
	});

}
