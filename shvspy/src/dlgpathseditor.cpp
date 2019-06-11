#include "dlgpathseditor.h"
#include "ui_dlgpathseditor.h"

#include <shv/core/log.h>

#include <QTableWidgetItem>

DlgPathsEditor::DlgPathsEditor(QWidget *parent, shv::iotqt::rpc::ClientConnection *rpc_connection, const std::string &acl_etc_node_path) :
	QDialog(parent),
	ui(new Ui::DlgPathsEditor)
{
	ui->setupUi(this);

	m_aclEtcPathsNodePath = acl_etc_node_path + "paths";
	m_rpcConnection = rpc_connection;

	static constexpr double ROW_HEIGHT_RATIO = 1.3;
	static QStringList INFO_HEADER_NAMES {{ tr("Path"), tr("Grant") }};

	ui->twPaths->setColumnCount(INFO_HEADER_NAMES.count());
	ui->twPaths->setHorizontalHeaderLabels(INFO_HEADER_NAMES);
	ui->twPaths->horizontalHeader()->setStretchLastSection(true);
	ui->twPaths->horizontalHeader()->resizeSection(0, width() * 0.8);
	ui->twPaths->verticalHeader()->setDefaultSectionSize(ui->twPaths->fontMetrics().height() * ROW_HEIGHT_RATIO);
	ui->twPaths->verticalHeader()->setVisible(false);
}

DlgPathsEditor::~DlgPathsEditor()
{
	delete ui;
}

void DlgPathsEditor::init(const QString &grant_id)
{
	ui->leGrantId->setText(grant_id);
	callGetPaths();
}

QString DlgPathsEditor::pathId()
{
	return ui->leGrantId->text();
}

void DlgPathsEditor::setPaths(const shv::chainpack::RpcValue::Map &paths)
{
	ui->twPaths->clearContents();
	ui->twPaths->setRowCount(0);

/*	if(!paths.isMap()){
		ui->lblStatus->setText(tr("Invalid return type of method getPath"));
		return;
	}
*/

	std::vector<std::string> keys = paths.keys();

	for (size_t i = 0; i < keys.size(); i++){
		const shv::chainpack::RpcValue::Map &path = paths.value(keys[i]).toMap();

		ui->twPaths->insertRow(static_cast<int>(i));
		QTableWidgetItem *item = new QTableWidgetItem(QString::fromStdString((keys[i])));
		item->setFlags(item->flags() & ~Qt::ItemIsEditable);
		ui->twPaths->setItem(i, 0, item);

		item = new QTableWidgetItem(QString::fromStdString((path.value("grant").toStdString())));
		item->setFlags(item->flags() & ~Qt::ItemIsEditable);
		ui->twPaths->setItem(i, 1, item);
	}

	ui->lblStatus->setText("");
}

void DlgPathsEditor::callGetPaths()
{
	if (m_rpcConnection == nullptr)
		return;

	ui->lblStatus->setText(tr("Getting paths ..."));

	int rqid = m_rpcConnection->nextRequestId();
	shv::iotqt::rpc::RpcResponseCallBack *cb = new shv::iotqt::rpc::RpcResponseCallBack(m_rpcConnection, rqid, this);

	cb->start(this, [this](const shv::chainpack::RpcResponse &response) {
		if(response.isValid()){
			if(response.isError()) {
				ui->lblStatus->setText(QString::fromStdString(response.error().toString()));
			}
			else{
				(response.result().isMap())? setPaths(response.result().toMap()) : setPaths(shv::chainpack::RpcValue::Map());
			}
		}
		else{
			ui->lblStatus->setText(tr("Request timeout expired"));
		}
	});

	m_rpcConnection->callShvMethod(rqid,  m_aclEtcPathsNodePath, "getPath", shv::chainpack::RpcValue(pathId().toStdString()));
}
