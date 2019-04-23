#include "dlguserseditor.h"
#include "ui_dlguserseditor.h"

#include "dlgaddedituser.h"
#include "theapp.h"

#include <shv/iotqt/rpc/rpcresponsecallback.h>
#include <shv/chainpack/rpcvalue.h>
#include <shv/core/log.h>
#include <shv/core/assert.h>

#include <QDialogButtonBox>
#include <QMessageBox>
#include <QTableWidgetItem>

DlgUsersEditor::DlgUsersEditor(QWidget *parent, shv::iotqt::rpc::ClientConnection *rpc_connection) :
	QDialog(parent),
	ui(new Ui::DlgUsersEditor)
{
	ui->setupUi(this);

	SHV_ASSERT_EX(rpc_connection != nullptr, "Internal error");

	static constexpr double ROW_HEIGHT_RATIO = 1.3;
	static QStringList INFO_HEADER_NAMES {{ tr("User") }};

	ui->twUsers->setColumnCount(INFO_HEADER_NAMES.count());
	ui->twUsers->setHorizontalHeaderLabels(INFO_HEADER_NAMES);
	ui->twUsers->horizontalHeader()->setStretchLastSection(true);
	ui->twUsers->verticalHeader()->setDefaultSectionSize(ui->twUsers->fontMetrics().height() * ROW_HEIGHT_RATIO);
	ui->twUsers->verticalHeader()->setVisible(false);

	m_rpcConnection = rpc_connection;

	connect(ui->pbAddUser, &QPushButton::clicked, this, &DlgUsersEditor::onAddUserClicked);
	connect(ui->pbDeleteUser, &QPushButton::clicked, this, &DlgUsersEditor::onDelUserClicked);
	connect(ui->pbEditUser, &QPushButton::clicked, this, &DlgUsersEditor::onEditUserClicked);
	connect(ui->twUsers, &QTableWidget::doubleClicked, this, &DlgUsersEditor::onTableUsersDoubleClicked);
}

DlgUsersEditor::~DlgUsersEditor()
{
	delete ui;
}

void DlgUsersEditor::init(const std::string &path)
{
	m_aclEtcUsersNodePath = path + "/etc/acl/users";
	listUsers();
}

void DlgUsersEditor::listUsers()
{
	if (m_rpcConnection == nullptr)
		return;

	ui->twUsers->clearContents();
	ui->twUsers->setRowCount(0);

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
						ui->twUsers->insertRow(static_cast<int>(i));
						QTableWidgetItem *item = new QTableWidgetItem(QString::fromStdString(res.at(i).toStdString()));
						item->setFlags(item->flags() & ~Qt::ItemIsEditable);
						ui->twUsers->setItem(i, 0, item);
					}
				}
			}
		}
		else{
			ui->lblStatus->setText(tr("Request timeout expired"));
		}
	});

	m_rpcConnection->callShvMethod(rqid, m_aclEtcUsersNodePath, shv::chainpack::Rpc::METH_LS);
}

QString DlgUsersEditor::selectedUser()
{
	return (ui->twUsers->currentIndex().isValid()) ? ui->twUsers->currentItem()->text() : QString();
}

void DlgUsersEditor::onAddUserClicked()
{
	DlgAddEditUser dlg(this, m_rpcConnection, m_aclEtcUsersNodePath, DlgAddEditUser::DialogType::Add);
	if (dlg.exec() == QDialog::Accepted){
		listUsers();
	}
}

void DlgUsersEditor::onDelUserClicked()
{
	QString user = selectedUser();

	if (user.isEmpty()){
		ui->lblStatus->setText(tr("Select user in the table above."));
		return;
	}

	ui->lblStatus->setText("");

	if (QMessageBox::question(this, tr("Delete user"), tr("Do you really want to delete user") + " " + user) == QMessageBox::Yes){
		int rqid = m_rpcConnection->nextRequestId();
		shv::iotqt::rpc::RpcResponseCallBack *cb = new shv::iotqt::rpc::RpcResponseCallBack(m_rpcConnection, rqid, this);

		cb->start(this, [this](const shv::chainpack::RpcResponse &response) {
			if(response.isValid()){
				if(response.isError()) {
					ui->lblStatus->setText(tr("Failed to delete user.") + " " + QString::fromStdString(response.error().toString()));
				}
				else{
					listUsers();
				}
			}
			else{
				ui->lblStatus->setText(tr("Request timeout expired"));
			}
		});

		m_rpcConnection->callShvMethod(rqid, m_aclEtcUsersNodePath, "delUser", shv::chainpack::RpcValue::String(user.toStdString()));
	}
}

void DlgUsersEditor::onEditUserClicked()
{
	QString user = selectedUser();

	if (user.isEmpty()){
		ui->lblStatus->setText(tr("Select user in the table above."));
		return;
	}

	ui->lblStatus->setText("");

	DlgAddEditUser dlg(this, m_rpcConnection, m_aclEtcUsersNodePath, DlgAddEditUser::DialogType::Edit);
	dlg.setUser(user);

	if (dlg.exec() == QDialog::Accepted){
		listUsers();
	}
}

void DlgUsersEditor::onTableUsersDoubleClicked(QModelIndex ix)
{
	Q_UNUSED(ix);
	onEditUserClicked();
}

