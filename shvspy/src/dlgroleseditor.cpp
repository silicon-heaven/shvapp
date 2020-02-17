#include "dlgroleseditor.h"
#include "ui_dlggrantseditor.h"

#include "dlgaddeditrole.h"
#include "shv/core/assert.h"
#include <QMessageBox>

DlgRolesEditor::DlgRolesEditor(QWidget *parent, shv::iotqt::rpc::ClientConnection *rpc_connection) :
	QDialog(parent),
	ui(new Ui::DlgRolesEditor)
{
	ui->setupUi(this);

	SHV_ASSERT_EX(rpc_connection != nullptr, "Internal error");

	m_rpcConnection = rpc_connection;

	static constexpr double ROW_HEIGHT_RATIO = 1.3;
	static QStringList INFO_HEADER_NAMES {{ tr("Role") }};

	ui->twRoles->setColumnCount(INFO_HEADER_NAMES.count());
	ui->twRoles->setHorizontalHeaderLabels(INFO_HEADER_NAMES);
	ui->twRoles->horizontalHeader()->setStretchLastSection(true);
	ui->twRoles->verticalHeader()->setDefaultSectionSize(ui->twRoles->fontMetrics().height() * ROW_HEIGHT_RATIO);
	ui->twRoles->verticalHeader()->setVisible(false);

	connect(ui->pbAddRole, &QPushButton::clicked, this, &DlgRolesEditor::onAddRoleClicked);
	connect(ui->pbDeleteRole, &QPushButton::clicked, this, &DlgRolesEditor::onDeleteRoleClicked);
	connect(ui->pbEditRole, &QPushButton::clicked, this, &DlgRolesEditor::onEditRoleClicked);
	connect(ui->twRoles, &QTableWidget::doubleClicked, this, &DlgRolesEditor::onTableRoleDoubleClicked);
}

DlgRolesEditor::~DlgRolesEditor()
{
	delete ui;
}

void DlgRolesEditor::init(const std::string &acl_node_path)
{
	m_aclEtcNodePath = acl_node_path;
	listRoles();
}

std::string DlgRolesEditor::aclEtcRolesNodePath()
{
	return m_aclEtcNodePath + "roles";
}

std::string DlgRolesEditor::aclEtcPathsNodePath()
{
	return m_aclEtcNodePath + "paths";
}

QString DlgRolesEditor::selectedRole()
{
	return (ui->twRoles->currentIndex().isValid()) ? ui->twRoles->currentItem()->text() : QString();
}

void DlgRolesEditor::onAddRoleClicked()
{
	DlgAddEditRole dlg(this, m_rpcConnection, m_aclEtcNodePath, DlgAddEditRole::DialogType::Add);
	if (dlg.exec() == QDialog::Accepted){
		listRoles();
	}
}

void DlgRolesEditor::onDeleteRoleClicked()
{
	std::string role = selectedRole().toStdString();

	if (role.empty()){
		ui->lblStatus->setText(tr("Select role in the table above."));
		return;
	}

	ui->lblStatus->setText("");

	if (QMessageBox::question(this, tr("Delete role"), tr("Do you really want to delete data and associated paths for role ") + " " + QString::fromStdString(role) + "?") == QMessageBox::Yes){
		int rqid = m_rpcConnection->nextRequestId();
		shv::iotqt::rpc::RpcResponseCallBack *cb = new shv::iotqt::rpc::RpcResponseCallBack(m_rpcConnection, rqid, this);

		cb->start(this, [this, role](const shv::chainpack::RpcResponse &response) {
			if(response.isValid()){
				if(response.isError()) {
					ui->lblStatus->setText(tr("Failed to delete role.") + " " + QString::fromStdString(response.error().toString()));
				}
				else{
					callDeletePathsForRole(role);
					listRoles();
				}
			}
			else{
				ui->lblStatus->setText(tr("Request timeout expired"));
			}
		});

		shv::chainpack::RpcValue::List params{shv::chainpack::RpcValue::String(role), {}};
		m_rpcConnection->callShvMethod(rqid, aclEtcRolesNodePath(), "setValue", params);
	}
}

void DlgRolesEditor::onEditRoleClicked()
{
	QString role = selectedRole();

	if (role.isEmpty()){
		ui->lblStatus->setText(tr("Select role in the table above."));
		return;
	}

	ui->lblStatus->setText("");

	DlgAddEditRole dlg(this, m_rpcConnection, m_aclEtcNodePath, DlgAddEditRole::DialogType::Edit);
	dlg.init(role);

	if (dlg.exec() == QDialog::Accepted){
		listRoles();
	}
}

void DlgRolesEditor::onTableRoleDoubleClicked(QModelIndex ix)
{
	Q_UNUSED(ix);
	onEditRoleClicked();
}

void DlgRolesEditor::listRoles()
{
	if (m_rpcConnection == nullptr)
		return;

	ui->twRoles->clearContents();
	ui->twRoles->setRowCount(0);

	int rqid = m_rpcConnection->nextRequestId();
	shv::iotqt::rpc::RpcResponseCallBack *cb = new shv::iotqt::rpc::RpcResponseCallBack(m_rpcConnection, rqid, this);

	cb->start(this, [this](const shv::chainpack::RpcResponse &response) {
		if(response.isValid()){
			if(response.isError()) {
				ui->lblStatus->setText(tr("Failed to load roles.") + " " + QString::fromStdString(response.error().toString()));
			}
			else{
				if (response.result().isList()){
					shv::chainpack::RpcValue::List res = response.result().toList();

					for (size_t i = 0; i < res.size(); i++){
						ui->twRoles->insertRow(static_cast<int>(i));
						QTableWidgetItem *item = new QTableWidgetItem(QString::fromStdString(res.at(i).toStdString()));
						item->setFlags(item->flags() & ~Qt::ItemIsEditable);
						ui->twRoles->setItem(i, 0, item);
					}
				}
			}
		}
		else{
			ui->lblStatus->setText(tr("Request timeout expired"));
		}
	});

	m_rpcConnection->callShvMethod(rqid, aclEtcRolesNodePath(), shv::chainpack::Rpc::METH_LS);
}

void DlgRolesEditor::callDeletePathsForRole(const std::string &role)
{
	if (m_rpcConnection == nullptr)
		return;

	ui->lblStatus->setText(tr("Deleting paths for role:") + " " + QString::fromStdString(role));

	int rqid = m_rpcConnection->nextRequestId();
	shv::iotqt::rpc::RpcResponseCallBack *cb = new shv::iotqt::rpc::RpcResponseCallBack(m_rpcConnection, rqid, this);

	cb->start(this, [this](const shv::chainpack::RpcResponse &response) {
		if (response.isValid()){
			if(response.isError()) {
				ui->lblStatus->setText(tr("Failed to delete paths.") + " " + QString::fromStdString(response.error().toString()));
			}
			else{
				ui->lblStatus->setText("");
			}
		}
		else{
			ui->lblStatus->setText(tr("Request timeout expired"));
		}
	});

	shv::chainpack::RpcValue::List params{shv::chainpack::RpcValue::String(role), {}};
	m_rpcConnection->callShvMethod(rqid, aclEtcPathsNodePath(), "setValue", params);
}
