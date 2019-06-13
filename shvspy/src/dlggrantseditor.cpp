#include "dlggrantseditor.h"
#include "ui_dlggrantseditor.h"

#include "dlgaddeditgrants.h"
#include "shv/core/assert.h"
#include <QMessageBox>

DlgGrantsEditor::DlgGrantsEditor(QWidget *parent, shv::iotqt::rpc::ClientConnection *rpc_connection) :
	QDialog(parent),
	ui(new Ui::DlgGrantsEditor)
{
	ui->setupUi(this);

	SHV_ASSERT_EX(rpc_connection != nullptr, "Internal error");

	m_rpcConnection = rpc_connection;

	static constexpr double ROW_HEIGHT_RATIO = 1.3;
	static QStringList INFO_HEADER_NAMES {{ tr("Grant") }};

	ui->twGrants->setColumnCount(INFO_HEADER_NAMES.count());
	ui->twGrants->setHorizontalHeaderLabels(INFO_HEADER_NAMES);
	ui->twGrants->horizontalHeader()->setStretchLastSection(true);
	ui->twGrants->verticalHeader()->setDefaultSectionSize(ui->twGrants->fontMetrics().height() * ROW_HEIGHT_RATIO);
	ui->twGrants->verticalHeader()->setVisible(false);

	connect(ui->pbAddGrant, &QPushButton::clicked, this, &DlgGrantsEditor::onAddGrantClicked);
	connect(ui->pbDeleteGrant, &QPushButton::clicked, this, &DlgGrantsEditor::onDelGrantClicked);
	connect(ui->pbEditGrant, &QPushButton::clicked, this, &DlgGrantsEditor::onEditGrantClicked);
	connect(ui->twGrants, &QTableWidget::doubleClicked, this, &DlgGrantsEditor::onTableGrantDoubleClicked);
}

DlgGrantsEditor::~DlgGrantsEditor()
{
	delete ui;
}

void DlgGrantsEditor::init(const std::string &path)
{
	m_aclEtcNodePath = path + "/etc/acl/";
	listGrants();
}

std::string DlgGrantsEditor::aclEtcGrantsNodePath()
{
	return m_aclEtcNodePath + "grants";
}

std::string DlgGrantsEditor::aclEtcPathsNodePath()
{
	return m_aclEtcNodePath + "paths";
}

QString DlgGrantsEditor::selectedGrant()
{
	return (ui->twGrants->currentIndex().isValid()) ? ui->twGrants->currentItem()->text() : QString();
}

void DlgGrantsEditor::onAddGrantClicked()
{
	DlgAddEditGrants dlg(this, m_rpcConnection, m_aclEtcNodePath, DlgAddEditGrants::DialogType::Add);
	if (dlg.exec() == QDialog::Accepted){
		listGrants();
	}
}

void DlgGrantsEditor::onDelGrantClicked()
{
	QString grant = selectedGrant();

	if (grant.isEmpty()){
		ui->lblStatus->setText(tr("Select grant in the table above."));
		return;
	}

	ui->lblStatus->setText("");

	if (QMessageBox::question(this, tr("Delete grant"), tr("Do you really want to delete data and associated paths for grant ") + " " + grant + "?") == QMessageBox::Yes){
		int rqid = m_rpcConnection->nextRequestId();
		shv::iotqt::rpc::RpcResponseCallBack *cb = new shv::iotqt::rpc::RpcResponseCallBack(m_rpcConnection, rqid, this);

		cb->start(this, [this, grant](const shv::chainpack::RpcResponse &response) {
			if(response.isValid()){
				if(response.isError()) {
					ui->lblStatus->setText(tr("Failed to delete grant.") + " " + QString::fromStdString(response.error().toString()));
				}
				else{
					callDeleteGrantPaths(grant.toStdString());
					listGrants();
				}
			}
			else{
				ui->lblStatus->setText(tr("Request timeout expired"));
			}
		});

		m_rpcConnection->callShvMethod(rqid, aclEtcGrantsNodePath(), "delGrant", shv::chainpack::RpcValue::String(grant.toStdString()));
	}
}

void DlgGrantsEditor::onEditGrantClicked()
{
	QString grant = selectedGrant();

	if (grant.isEmpty()){
		ui->lblStatus->setText(tr("Select grant in the table above."));
		return;
	}

	ui->lblStatus->setText("");

	DlgAddEditGrants dlg(this, m_rpcConnection, m_aclEtcNodePath, DlgAddEditGrants::DialogType::Edit);
	dlg.init(grant);

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
				ui->lblStatus->setText(tr("Failed to load grants.") + " " + QString::fromStdString(response.error().toString()));
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

	m_rpcConnection->callShvMethod(rqid, aclEtcGrantsNodePath(), shv::chainpack::Rpc::METH_LS);
}

void DlgGrantsEditor::callDeleteGrantPaths(const std::string &gran_name)
{
	if (m_rpcConnection == nullptr)
		return;

	ui->lblStatus->setText("Deleting grant paths.");

	int rqid = m_rpcConnection->nextRequestId();
	shv::iotqt::rpc::RpcResponseCallBack *cb = new shv::iotqt::rpc::RpcResponseCallBack(m_rpcConnection, rqid, this);

	cb->start(this, [this](const shv::chainpack::RpcResponse &response) {
		if (response.isValid()){
			if(response.isError()) {
				ui->lblStatus->setText(tr("Failed to delete grant paths.") + QString::fromStdString(response.error().toString()));
			}
			else{
				ui->lblStatus->setText("");
			}
		}
		else{
			ui->lblStatus->setText(tr("Request timeout expired"));
		}
	});

	m_rpcConnection->callShvMethod(rqid, aclEtcPathsNodePath(), "delGrantPaths", shv::chainpack::RpcValue::String(gran_name));
}
