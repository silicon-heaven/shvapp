#include "dlgaddedituser.h"
#include "ui_dlgaddedituser.h"

#include "theapp.h"
#include "dlgselectroles.h"

#include <shv/broker/aclrole.h>

#include <QCryptographicHash>

static const std::string FORMAT_KEY = "format";
static const std::string PASSWORD_KEY = "password";
static const std::string ROLES_KEY = "roles";

static const std::string VALUE_METHOD = "value";
static const std::string SET_VALUE_METHOD = "setValue";

DlgAddEditUser::DlgAddEditUser(QWidget *parent, shv::iotqt::rpc::ClientConnection *rpc_connection, const std::string &acl_etc_users_node_path,
							   const std::string &acl_etc_roles_node_path, DlgAddEditUser::DialogType dt) :
	QDialog(parent),
	ui(new Ui::DlgAddEditUser),
	m_aclEtcUsersNodePath(acl_etc_users_node_path),
	m_aclEtcRolesNodePath(acl_etc_roles_node_path)
{
	ui->setupUi(this);
	m_dialogType = dt;
	bool edit_mode = (m_dialogType == DialogType::Edit);

	ui->leUserName->setReadOnly(edit_mode);
	ui->groupBox->setTitle(edit_mode ? tr("Edit user") : tr("New user"));
	setWindowTitle(edit_mode ? tr("Edit user dialog") : tr("New user dialog"));
	ui->lblPassword->setText(edit_mode ? tr("New password") : tr("Password"));

	m_rpcConnection = rpc_connection;

	if(m_rpcConnection == nullptr){
		ui->lblStatus->setText(tr("Connection to shv does not exist."));
	}

	connect(ui->tbShowPassword, &QToolButton::clicked, this, &DlgAddEditUser::onShowPasswordClicked);
	connect(ui->pbSelectRoles, &QPushButton::clicked, this, &DlgAddEditUser::onSelectRolesClicked);
}

DlgAddEditUser::~DlgAddEditUser()
{
	delete ui;
}

static std::string sha1_hex(const std::string &s)
{
	QCryptographicHash hash(QCryptographicHash::Algorithm::Sha1);
	hash.addData(s.data(), s.length());
	return std::string(hash.result().toHex().constData());
}

DlgAddEditUser::DialogType DlgAddEditUser::dialogType()
{
	return m_dialogType;
}

std::string DlgAddEditUser::user()
{
	return ui->leUserName->text().toStdString();
}

void DlgAddEditUser::setUser(const QString &user)
{
	ui->leUserName->setText(user);
	callGetUserSettings();
}

QString DlgAddEditUser::password()
{
	return ui->lePassword->text();
}

void DlgAddEditUser::accept()
{
	if (dialogType() == DialogType::Add){
		if ((!user().empty()) && (!password().isEmpty())){
			ui->lblStatus->setText(tr("Adding new user"));

			if (ui->chbCreateRole->isChecked()){
				callCreateRoleAndSetSettings(user());
			}
			else{
				callSetUserSettings();
			}
		}
		else {
			ui->lblStatus->setText(tr("User name or password is empty."));
		}
	}
	else if (dialogType() == DialogType::Edit){
		ui->lblStatus->setText(tr("Updating user ...") + QString::fromStdString(m_aclEtcUsersNodePath));

		if (ui->chbCreateRole->isChecked()){
			callCreateRoleAndSetSettings(user());
		}
		else{
			callSetUserSettings();
		}
	}
}

void DlgAddEditUser::onShowPasswordClicked()
{
	bool password_mode = (ui->lePassword->echoMode() == QLineEdit::EchoMode::Password);
	ui->lePassword->setEchoMode((password_mode) ? QLineEdit::EchoMode::Normal : QLineEdit::EchoMode::Password);
	ui->tbShowPassword->setIcon((password_mode) ? QIcon(":/shvspy/images/hide.svg") : QIcon(":/shvspy/images/show.svg"));
}

void DlgAddEditUser::onSelectRolesClicked()
{
	DlgSelectRoles dlg(this);
	dlg.init(m_rpcConnection, m_aclEtcRolesNodePath, roles());

	if (dlg.exec() == QDialog::Accepted){
		setRoles(dlg.selectedRoles());
	}
}

void DlgAddEditUser::callCreateRoleAndSetSettings(const std::string &role_name)
{
	if (m_rpcConnection == nullptr)
		return;

	int rqid = m_rpcConnection->nextRequestId();
	shv::iotqt::rpc::RpcResponseCallBack *cb = new shv::iotqt::rpc::RpcResponseCallBack(m_rpcConnection, rqid, this);

	cb->start(this, [this](const shv::chainpack::RpcResponse &response) {
		if (response.isValid()){
			if(response.isError()) {
				ui->lblStatus->setText(tr("Failed to add role.") + QString::fromStdString(response.error().toString()));
			}
			else{
				callSetUserSettings();
			}
		}
		else{
			ui->lblStatus->setText(tr("Request timeout expired"));
		}
	});

	shv::broker::AclRole role(0);

	shv::chainpack::RpcValue::List params{role_name, role.toRpcValueMap()};
	m_rpcConnection->callShvMethod(rqid, m_aclEtcRolesNodePath, SET_VALUE_METHOD, params);
}

void DlgAddEditUser::callGetUserSettings()
{
	if (m_rpcConnection == nullptr)
		return;

	ui->lblStatus->setText(tr("Getting settings ..."));

	int rqid = m_rpcConnection->nextRequestId();
	shv::iotqt::rpc::RpcResponseCallBack *cb = new shv::iotqt::rpc::RpcResponseCallBack(m_rpcConnection, rqid, this);

	cb->start(this, [this](const shv::chainpack::RpcResponse &response) {
		if(response.isValid()){
			if(response.isError()) {
				ui->lblStatus->setText(QString::fromStdString(response.error().toString()));
			}
			else{
				m_remoteUserSettings = response.result().isMap() ? response.result().toMap() : shv::chainpack::RpcValue::Map();
				setRoles(m_remoteUserSettings.value(ROLES_KEY).toList());
				ui->lblStatus->setText("");
			}
		}
		else{
			ui->lblStatus->setText(tr("Request timeout expired"));
		}
	});

	m_rpcConnection->callShvMethod(rqid, userShvPath(), VALUE_METHOD);
}

void DlgAddEditUser::callSetUserSettings()
{
	if (m_rpcConnection == nullptr)
		return;

	int rqid = m_rpcConnection->nextRequestId();
	shv::iotqt::rpc::RpcResponseCallBack *cb = new shv::iotqt::rpc::RpcResponseCallBack(m_rpcConnection, rqid, this);

	cb->start(this, [this](const shv::chainpack::RpcResponse &response) {
		if (response.isValid()){
			if(response.isError()) {
				ui->lblStatus->setText(QString::fromStdString(response.error().toString()));
			}
			else{
				QDialog::accept();
			}
		}
		else{
			ui->lblStatus->setText(tr("Request timeout expired"));
		}
	});

	shv::chainpack::RpcValue::Map user_settings;

	if (m_dialogType == DialogType::Edit){
		user_settings = m_remoteUserSettings;
	}

	user_settings[ROLES_KEY] = roles();

	if (!password().isEmpty()){
		user_settings[PASSWORD_KEY] = shv::chainpack::RpcValue::Map{{FORMAT_KEY, "SHA1"}, {PASSWORD_KEY, sha1_hex(password().toStdString())}};
	}

	shv::chainpack::RpcValue::List params{user(), user_settings};
	m_rpcConnection->callShvMethod(rqid, aclUsersShvPath(), SET_VALUE_METHOD, params);
}

const std::string &DlgAddEditUser::aclUsersShvPath()
{
	return m_aclEtcUsersNodePath;
}

std::string DlgAddEditUser::userShvPath()
{
	return m_aclEtcUsersNodePath + '/' + user() + "/";
}

std::vector<std::string> DlgAddEditUser::roles()
{
	std::vector<std::string> roles;
	QStringList lst = ui->leRoles->text().split(",", QString::SplitBehavior::SkipEmptyParts);

	for (int i = 0; i < lst.count(); i++){
		roles.push_back(lst.at(i).trimmed().toStdString());
	}

	if (ui->chbCreateRole->isChecked())
		 roles.push_back(user());

	return roles;
}

void DlgAddEditUser::setRoles(const std::vector<std::string> &roles)
{
	QStringList roles_list;

	for (const std::string role : roles){
		roles_list.append(QString::fromStdString(role));
	}

	ui->leRoles->setText(roles_list.join(","));
}

void DlgAddEditUser::setRoles(const shv::chainpack::RpcValue::List &roles)
{
	QStringList roles_list;

	for (shv::chainpack::RpcValue role : roles){
		if (role.isString()){
			roles_list.append(QString::fromStdString(role.toString()));
		}
	}

	ui->leRoles->setText(roles_list.join(","));
}
