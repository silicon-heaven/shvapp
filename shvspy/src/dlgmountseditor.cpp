#include "dlgmountseditor.h"
#include "ui_dlgmountseditor.h"

#include "dlgaddeditmount.h"
#include "shv/core/assert.h"
#include <QMessageBox>

static const std::string VALUE_METHOD = "value";
static const std::string SET_VALUE_METHOD = "setValue";

DlgMountsEditor::DlgMountsEditor(QWidget *parent, shv::iotqt::rpc::ClientConnection *rpc_connection) :
	QDialog(parent),
	ui(new Ui::DlgMountsEditor)
{
	ui->setupUi(this);

	SHV_ASSERT_EX(rpc_connection != nullptr, "Internal error");

	m_rpcConnection = rpc_connection;

	static constexpr double ROW_HEIGHT_RATIO = 1.3;
	static QStringList INFO_HEADER_NAMES {{ tr("Mount") }};

	ui->twMounts->setColumnCount(INFO_HEADER_NAMES.count());
	ui->twMounts->setHorizontalHeaderLabels(INFO_HEADER_NAMES);
	ui->twMounts->horizontalHeader()->setStretchLastSection(true);
	ui->twMounts->verticalHeader()->setDefaultSectionSize(ui->twMounts->fontMetrics().height() * ROW_HEIGHT_RATIO);
	ui->twMounts->verticalHeader()->setVisible(false);

	connect(ui->pbAddMount, &QPushButton::clicked, this, &DlgMountsEditor::onAddMountClicked);
	connect(ui->pbDeleteMount, &QPushButton::clicked, this, &DlgMountsEditor::onDeleteMountClicked);
	connect(ui->pbEditMount, &QPushButton::clicked, this, &DlgMountsEditor::onEditMountClicked);
	connect(ui->twMounts, &QTableWidget::doubleClicked, this, &DlgMountsEditor::onTableMountDoubleClicked);

	setStatusText(QString());
}

DlgMountsEditor::~DlgMountsEditor()
{
	delete ui;
}

void DlgMountsEditor::init(const std::string &acl_node_path)
{
	m_aclEtcNodePath = acl_node_path;
	listMounts();
}

std::string DlgMountsEditor::aclEtcMountsNodePath()
{
	return m_aclEtcNodePath + "/mounts";
}

//std::string DlgMountsEditor::aclEtcAccessNodePath()
//{
//	return m_aclEtcNodePath + "/access";
//}

QString DlgMountsEditor::selectedMount()
{
	return (ui->twMounts->currentIndex().isValid()) ? ui->twMounts->currentItem()->text() : QString();
}

void DlgMountsEditor::onAddMountClicked()
{
	DlgAddEditMount dlg(this, m_rpcConnection, m_aclEtcNodePath, DlgAddEditMount::DialogType::Add);
	if (dlg.exec() == QDialog::Accepted){
		listMounts();
	}
}

void DlgMountsEditor::onDeleteMountClicked()
{
	std::string mount = selectedMount().toStdString();

	if (mount.empty()){
		setStatusText(tr("Select mount in the table above."));
		return;
	}

	setStatusText(QString());

	if (QMessageBox::question(this, tr("Delete mount"), tr("Do you really want to delete mount") + " " + QString::fromStdString(mount) + "?") == QMessageBox::Yes){
		int rqid = m_rpcConnection->nextRequestId();
		shv::iotqt::rpc::RpcResponseCallBack *cb = new shv::iotqt::rpc::RpcResponseCallBack(m_rpcConnection, rqid, this);

		cb->start(this, [this, mount](const shv::chainpack::RpcResponse &response) {
			if(response.isValid()){
				if(response.isError()) {
					setStatusText(tr("Failed to delete mount.") + " " + QString::fromStdString(response.error().toString()));
				}
				else{
					listMounts();
					setStatusText(QString());
				}
			}
			else{
				setStatusText(tr("Request timeout expired"));
			}
		});

		shv::chainpack::RpcValue::List params{shv::chainpack::RpcValue::String(mount), {}};
		m_rpcConnection->callShvMethod(rqid, aclEtcMountsNodePath(), SET_VALUE_METHOD, params);
	}
}

void DlgMountsEditor::onEditMountClicked()
{
	QString mount = selectedMount();

	if (mount.isEmpty()){
		setStatusText(tr("Select mount in the table above."));
		return;
	}

	setStatusText(QString());

	DlgAddEditMount dlg(this, m_rpcConnection, m_aclEtcNodePath, DlgAddEditMount::DialogType::Edit);
	dlg.init(mount);

	if (dlg.exec() == QDialog::Accepted){
		listMounts();
	}
}

void DlgMountsEditor::onTableMountDoubleClicked(QModelIndex ix)
{
	Q_UNUSED(ix);
	onEditMountClicked();
}

void DlgMountsEditor::listMounts()
{
	if (m_rpcConnection == nullptr)
		return;

	ui->twMounts->clearContents();
	ui->twMounts->setRowCount(0);

	int rqid = m_rpcConnection->nextRequestId();
	shv::iotqt::rpc::RpcResponseCallBack *cb = new shv::iotqt::rpc::RpcResponseCallBack(m_rpcConnection, rqid, this);

	cb->start(this, [this](const shv::chainpack::RpcResponse &response) {
		if(response.isValid()){
			if(response.isError()) {
				setStatusText(tr("Failed to load mounts.") + " " + QString::fromStdString(response.error().toString()));
			}
			else{
				if (response.result().isList()){
					shv::chainpack::RpcValue::List res = response.result().toList();

					for (size_t i = 0; i < res.size(); i++){
						ui->twMounts->insertRow(static_cast<int>(i));
						QTableWidgetItem *item = new QTableWidgetItem(QString::fromStdString(res.at(i).toStdString()));
						item->setFlags(item->flags() & ~Qt::ItemIsEditable);
						ui->twMounts->setItem(i, 0, item);
					}
				}
				setStatusText(QString());
			}
		}
		else{
			setStatusText(tr("Request timeout expired"));
		}
	});

	m_rpcConnection->callShvMethod(rqid, aclEtcMountsNodePath(), shv::chainpack::Rpc::METH_LS);
}

//void DlgMountsEditor::callDeleteAccessForMount(const std::string &mount)
//{
//	if (m_rpcConnection == nullptr)
//		return;

//	setStatusText(tr("Deleting access paths for mount:") + " " + QString::fromStdString(mount));

//	int rqid = m_rpcConnection->nextRequestId();
//	shv::iotqt::rpc::RpcResponseCallBack *cb = new shv::iotqt::rpc::RpcResponseCallBack(m_rpcConnection, rqid, this);

//	cb->start(this, [this](const shv::chainpack::RpcResponse &response) {
//		if (response.isValid()){
//			if(response.isError()) {
//				setStatusText(tr("Failed to delete access pahts.") + " " + QString::fromStdString(response.error().toString()));
//			}
//			else{
//				setStatusText(QString());
//			}
//		}
//		else{
//			setStatusText(tr("Request timeout expired"));
//		}
//	});

//	shv::chainpack::RpcValue::List params{shv::chainpack::RpcValue::String(mount), {}};
//	m_rpcConnection->callShvMethod(rqid, aclEtcAccessNodePath(), SET_VALUE_METHOD, params);
//}

void DlgMountsEditor::setStatusText(const QString &txt)
{
	if(txt.isEmpty()) {
		ui->lblStatus->hide();
	}
	else {
		ui->lblStatus->show();
		ui->lblStatus->setText(txt);
	}
}

