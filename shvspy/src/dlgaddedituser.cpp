#include "dlgaddedituser.h"
#include "ui_addedituserdialog.h"

#include "theapp.h"


DlgAddEditUser::DlgAddEditUser(QWidget *parent, shv::iotqt::rpc::ClientConnection *rpc_connection, DlgAddEditUser::DialogType dt) :
	QDialog(parent),
	ui(new Ui::AddEditUserDialog)
{
	ui->setupUi(this);
	m_dialogType = dt;
	bool edit_mode = (m_dialogType == DialogType::DtEditUser);

	showPasswordItems(!edit_mode);
	ui->leUserName->setReadOnly(edit_mode);
	ui->groupBox->setTitle(edit_mode ? tr("Edit user") : tr("New user"));
	setWindowTitle(edit_mode ? tr("Edit user dialog") : tr("New user dialog"));

	m_rpcConection = rpc_connection;

	if(m_rpcConection == nullptr){
		ui->lblStatus->setText(tr("Connection to shv does not exist."));
	}

	connect(ui->pbChangePassword, &QPushButton::clicked, this, [this](){
		showPasswordItems(true);
		ui->lePassword->setFocus();
	});

	connect(ui->tbShowPassword, &QToolButton::clicked, this, &DlgAddEditUser::onShowPasswordClicked);
}

DlgAddEditUser::~DlgAddEditUser()
{
	delete ui;
}

DlgAddEditUser::DialogType DlgAddEditUser::dialogType()
{
	return m_dialogType;
}

QString DlgAddEditUser::user()
{
	return ui->leUserName->text();
}

void DlgAddEditUser::setUser(const QString &user)
{
	ui->leUserName->setText(user);
	callGetGrants();
}

QString DlgAddEditUser::password()
{
	return ui->lePassword->text();
}

void DlgAddEditUser::accept()
{
	if (dialogType() == DtAddUser){
		if ((!user().isEmpty()) && (!password().isEmpty())){
			ui->lblStatus->setText(tr("Adding new user ..."));
			callAddUser();
		}
		else {
			ui->lblStatus->setText(tr("User name or password is empty."));
		}
	}
	else if (dialogType() == DtEditUser){
		ui->lblStatus->setText(tr("Updating user ..."));
		if (!password().isEmpty()){
			callChangePassword();
		}

		callSetGrants();
	}
}

void DlgAddEditUser::onShowPasswordClicked()
{
	bool password_mode = (ui->lePassword->echoMode() == QLineEdit::EchoMode::Password);
	ui->lePassword->setEchoMode((password_mode) ? QLineEdit::EchoMode::Normal : QLineEdit::EchoMode::Password);
	ui->tbShowPassword->setIcon((password_mode) ? QIcon(":/images/hide.svg") : QIcon(":/images/show.svg"));
}

void DlgAddEditUser::callAddUser()
{
	if (m_rpcConection == nullptr)
		return;

	shv::chainpack::RpcValue::Map params;
	params["user"] = user().toStdString();
	params["password"] = "";//TheApp::sha1Hex(password().toStdString());
	params["grants"] = grants();

	int rqid = m_rpcConection->nextRequestId();
	shv::iotqt::rpc::RpcResponseCallBack *cb = new shv::iotqt::rpc::RpcResponseCallBack(m_rpcConection, rqid, this);

	cb->start(this, [this](const shv::chainpack::RpcResponse &response) {
		if (response.isValid()){
			if(response.isError()) {
				ui->lblStatus->setText(tr("Failed to add user.") + QString::fromStdString(response.error().toString()));
			}
			else{
				QDialog::accept();
			}
		}
		else{
			ui->lblStatus->setText(tr("Request timeout expired"));
		}
	});

//	m_rpcConection->callShvMethod(rqid, Application::BRCLAB_PROVIDER_USERS_PATH, "addUser", params);
}

void DlgAddEditUser::callChangePassword()
{
	if (m_rpcConection == nullptr)
		return;

	m_requestedRpcCallsCount++;

	int rqid = m_rpcConection->nextRequestId();
	shv::iotqt::rpc::RpcResponseCallBack *cb = new shv::iotqt::rpc::RpcResponseCallBack(m_rpcConection, rqid, this);

	cb->start(this, [this](const shv::chainpack::RpcResponse &response) {
		if (response.isValid()){
			if(response.isError()) {
				ui->lblStatus->setText(QString::fromStdString(response.error().toString()));
			}
			else{
				m_requestedRpcCallsCount--;
				callCommitChanges();
			}
		}
		else{
			ui->lblStatus->setText(tr("Request timeout expired"));
		}
	});

//	std::string path = Application::BRCLAB_PROVIDER_USERS_PATH + '/' + user().toStdString() + "/";
//	m_rpcConection->callShvMethod(rqid, path + "password", "set", Application::sha1Hex(password().toStdString()));
}

void DlgAddEditUser::callSetGrants()
{
	if (m_rpcConection == nullptr)
		return;

	m_requestedRpcCallsCount++;

	int rqid = m_rpcConection->nextRequestId();
	shv::iotqt::rpc::RpcResponseCallBack *cb = new shv::iotqt::rpc::RpcResponseCallBack(m_rpcConection, rqid, this);

	cb->start(this, [this](const shv::chainpack::RpcResponse &response) {
		if (response.isValid()){
			if(response.isError()) {
				ui->lblStatus->setText(QString::fromStdString(response.error().toString()));
			}
			else{
				m_requestedRpcCallsCount--;
				callCommitChanges();
			}
		}
		else{
			ui->lblStatus->setText(tr("Request timeout expired"));
		}
	});

//	std::string path = Application::BRCLAB_PROVIDER_USERS_PATH + '/' + user().toStdString() + "/";
//	m_rpcConection->callShvMethod(rqid, path + "grants", "set", grants());

}

void DlgAddEditUser::callGetGrants()
{
	if (m_rpcConection == nullptr)
		return;

	ui->lblStatus->setText(tr("Getting settings ..."));

	int rqid = m_rpcConection->nextRequestId();
	shv::iotqt::rpc::RpcResponseCallBack *cb = new shv::iotqt::rpc::RpcResponseCallBack(m_rpcConection, rqid, this);

	cb->start(this, [this](const shv::chainpack::RpcResponse &response) {
		if(response.isValid()){
			if(response.isError()) {
				ui->lblStatus->setText(QString::fromStdString(response.error().toString()));
			}
			else{
				(response.result().isList())? setGrants(response.result().toList()) : setGrants(shv::chainpack::RpcValue::List());
				ui->lblStatus->setText("");
			}
		}
		else{
			ui->lblStatus->setText(tr("Request timeout expired"));
		}
	});

//	std::string path = Application::BRCLAB_PROVIDER_USERS_PATH + '/' + user().toStdString() + "/";
//	m_rpcConection->callShvMethod(rqid, path + "grants", "get");
}

void DlgAddEditUser::callCommitChanges()
{
	if ((m_requestedRpcCallsCount == 0) && (m_rpcConection != nullptr)){
		int rqid = m_rpcConection->nextRequestId();
		shv::iotqt::rpc::RpcResponseCallBack *cb = new shv::iotqt::rpc::RpcResponseCallBack(m_rpcConection, rqid, this);

		cb->start(this, [this](const shv::chainpack::RpcResponse &response) {
			if(response.isValid()){
				if(response.isError()) {
					ui->lblStatus->setText(QString::fromStdString(response.error().toString()));
				}
				else{
					QDialog::accept();
				}
			}
			else {
				ui->lblStatus->setText(tr("Request timeout expired"));
			}
		});

//		m_rpcConection->callShvMethod(rqid, Application::BRCLAB_PROVIDER_USERS_PATH, "commitChanges");
	}
}

void DlgAddEditUser::showPasswordItems(bool visible)
{
	ui->pbChangePassword->setVisible(!visible);
	ui->lePassword->setVisible(visible);
	ui->tbShowPassword->setVisible(visible);
}

shv::chainpack::RpcValue::List DlgAddEditUser::grants()
{
	shv::chainpack::RpcValue::List grants;
	QStringList lst = ui->leGrants->text().split(",", QString::SplitBehavior::SkipEmptyParts);

	for (int i = 0; i < lst.count(); i++){
		grants.push_back(shv::chainpack::RpcValue::String(lst.at(i).trimmed().toStdString()));
	}

	return grants;
}

void DlgAddEditUser::setGrants(const shv::chainpack::RpcValue::List &grants)
{
	QString g;

	for (size_t i = 0; i < grants.size(); i++){
		if(i > 0)
			g += ',';
		g += QString::fromStdString(grants[i].toStdString());
	}

	ui->leGrants->setText(g);
}
