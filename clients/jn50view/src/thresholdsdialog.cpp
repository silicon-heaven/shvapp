#include "thresholdsdialog.h"
#include "ui_thresholdsdialog.h"
#include "jn50viewapp.h"
#include "appclioptions.h"

#include <shv/iotqt/rpc/clientconnection.h>
#include <shv/iotqt/rpc/rpcresponsecallback.h>

#include <shv/coreqt/log.h>

#include <QDialogButtonBox>
#include <QMessageBox>
#include <QPushButton>

namespace cp = shv::chainpack;

ThresholdsDialog::ThresholdsDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::ThresholdsDialog)
{
	ui->setupUi(this);

	connect(ui->buttonBox, &QDialogButtonBox::clicked, [this](QAbstractButton *bt) {
		shvDebug() << "QDialogButtonBox::clicked";
		if(bt == ui->buttonBox->button(QDialogButtonBox::Apply)) {
			saveTresholdIfEdited("voltageTreshold", ui->edU);
			saveTresholdIfEdited("currentTreshold", ui->edI);
			saveTresholdIfEdited("powerTreshold", ui->edW);
			saveTresholdIfEdited("energyTreshold", ui->edE);
		}
	});

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
	shvLogFuncFrame();
	//m_origTresholds[shv_name] = -1;
	editor->setValue(editor->minimum());
	Jn50ViewApp *app = Jn50ViewApp::instance();
	std::string path = app->cliOptions()->converterShvPath() + "/settings/" + shv_name;
	shv::iotqt::rpc::ClientConnection *conn = app->rpcConnection();
	int rq_id = conn->nextRequestId();
	shv::iotqt::rpc::RpcResponseCallBack *cb = new shv::iotqt::rpc::RpcResponseCallBack(conn, rq_id, this);
	cb->start(this, [this, editor, path, shv_name](const cp::RpcResponse &resp) {
		if(resp.isValid()) {
			if(resp.isError())
				shvWarning() << "GET" << path << "RPC request error:" << resp.error().toString();
			else {
				shvDebug() << "GET" << path << "RPC request SUCCESS";
				editor->setValue(resp.result().toInt());
				m_origTresholds[shv_name] = editor->value();
			}
		}
		else {
			shvWarning() << "GET" << path << "RPC request timeout";
		}
	});
	conn->callShvMethod(rq_id, path, cp::Rpc::METH_GET);
}

void ThresholdsDialog::saveTresholdIfEdited(const std::string &shv_name, QSpinBox *editor)
{
	shvLogFuncFrame();
	if(m_origTresholds[shv_name] == editor->value())
		return;
	Jn50ViewApp *app = Jn50ViewApp::instance();
	std::string path = app->cliOptions()->converterShvPath() + "/settings/" + shv_name;
	shv::iotqt::rpc::ClientConnection *conn = app->rpcConnection();
	int rq_id = conn->nextRequestId();
	shv::iotqt::rpc::RpcResponseCallBack *cb = new shv::iotqt::rpc::RpcResponseCallBack(conn, rq_id, this);
	cb->start(this, [this, editor, path, shv_name](const cp::RpcResponse &resp) {
		if(resp.isValid()) {
			if(resp.isError())
				shvWarning() << "SET" << path << "RPC request error:" << resp.error().toString();
			else {
				shvDebug() << "SET" << path << "RPC request SUCCESS";
				loadTreshold(shv_name, editor);
				return;
			}
		}
		else {
			shvWarning() << "SET" << path << "RPC request timeout";
		}
		QMessageBox::warning(this, "JN50 View", "Chyba komunikace při ukládání hodnoty: " + QString::fromStdString(shv_name));
	});
	conn->callShvMethod(rq_id, path, cp::Rpc::METH_SET, editor->value());
}
