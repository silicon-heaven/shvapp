#include "dlggrantseditor.h"
#include "ui_dlggrantseditor.h"

#include "dlgaddeditgrants.h"

DlgGrantsEditor::DlgGrantsEditor(QWidget *parent, shv::iotqt::rpc::ClientConnection *rpc_connection) :
	QDialog(parent),
	ui(new Ui::DlgGrantsEditor)
{
	ui->setupUi(this);
	m_rpcConnection = rpc_connection;

	static constexpr double ROW_HEIGHT_RATIO = 1.3;
	static QStringList INFO_HEADER_NAMES {{ tr("Grant") }};

	ui->twGrants->setColumnCount(INFO_HEADER_NAMES.count());
	ui->twGrants->setHorizontalHeaderLabels(INFO_HEADER_NAMES);
	ui->twGrants->horizontalHeader()->setStretchLastSection(true);
	ui->twGrants->verticalHeader()->setDefaultSectionSize(ui->twGrants->fontMetrics().height() * ROW_HEIGHT_RATIO);
	ui->twGrants->verticalHeader()->setVisible(false);

	if(m_rpcConnection != nullptr){
		connect(ui->pbAddGrant, &QPushButton::clicked, this, &DlgGrantsEditor::onAddGrantClicked);
		connect(ui->pbDeleteGrant, &QPushButton::clicked, this, &DlgGrantsEditor::onDelGrantClicked);
		connect(ui->pbEditGrant, &QPushButton::clicked, this, &DlgGrantsEditor::onEditGrantClicked);
		connect(ui->twGrants, &QTableWidget::doubleClicked, this, &DlgGrantsEditor::onTableGrantDoubleClicked);
	}
	else{
		ui->lblStatus->setText(tr("Connection to shv does not exist."));
	}
}

DlgGrantsEditor::~DlgGrantsEditor()
{
	delete ui;
}

void DlgGrantsEditor::init(const std::string &path)
{
	m_aclEtcGrantsNodePath = path + "/etc/acl/grants";
	listGrants();
}

QString DlgGrantsEditor::selectedGrant()
{
	return (ui->twGrants->currentIndex().isValid()) ? ui->twGrants->currentItem()->text() : QString();
}

void DlgGrantsEditor::onAddGrantClicked()
{

}

void DlgGrantsEditor::onDelGrantClicked()
{

}

void DlgGrantsEditor::onEditGrantClicked()
{
	QString grant = selectedGrant();

	if (grant.isEmpty()){
		ui->lblStatus->setText(tr("Select grant in the table above."));
		return;
	}

	ui->lblStatus->setText("");

	DlgAddEditGrants dlg(this, m_rpcConnection, m_aclEtcGrantsNodePath, DlgAddEditGrants::DtEdit);
	dlg.setGrantName(grant);

	if (dlg.exec() == QDialog::Accepted){
		listGrants();
	}

}

void DlgGrantsEditor::onTableGrantDoubleClicked(QModelIndex ix)
{
	Q_UNUSED(ix);
	onEditGrantClicked();
}

void DlgGrantsEditor::listGrants()
{
	if (m_rpcConnection == nullptr)
		return;

	ui->twGrants->clearContents();
	ui->twGrants->setRowCount(0);

	int rqid = m_rpcConnection->nextRequestId();
	shv::iotqt::rpc::RpcResponseCallBack *cb = new shv::iotqt::rpc::RpcResponseCallBack(m_rpcConnection, rqid, this);

	cb->start(this, [this](const shv::chainpack::RpcResponse &response) {
		if(response.isValid()){
			if(response.isError()) {
				ui->lblStatus->setText(tr("Failed to load users.") + " " + QString::fromStdString(response.error().toString()));
			}
			else{
				if (response.result().isList()){
					shv::chainpack::RpcValue::List res = response.result().toList();

					for (size_t i = 0; i < res.size(); i++){
						ui->twGrants->insertRow(static_cast<int>(i));
						QTableWidgetItem *item = new QTableWidgetItem(QString::fromStdString(res.at(i).toStdString()));
						item->setFlags(item->flags() & ~Qt::ItemIsEditable);
						ui->twGrants->setItem(i, 0, item);
					}
				}
			}
		}
		else{
			ui->lblStatus->setText(tr("Request timeout expired"));
		}
	});

	m_rpcConnection->callShvMethod(rqid, m_aclEtcGrantsNodePath, shv::chainpack::Rpc::METH_LS);
}
