#include "dlgaddeditgrant.h"
#include "ui_dlgaddeditgrant.h"

#include "shv/core/log.h"

static const std::string WEIGHT = "weight";
static const std::string GRANTS = "grants";

DlgAddEditGrant::DlgAddEditGrant(QWidget *parent, shv::iotqt::rpc::ClientConnection *rpc_connection, const std::string &acl_etc_node_path, DlgAddEditGrant::DialogType dt) :
	QDialog(parent),
	ui(new Ui::DlgAddEditGrant),
	m_aclEtcNodePath(acl_etc_node_path)
{
	ui->setupUi(this);
	m_dialogType = dt;
	bool edit_mode = (m_dialogType == DialogType::Edit);

	ui->leGrantName->setEnabled(!edit_mode);
	ui->groupBox->setTitle(edit_mode ? tr("Edit grant") : tr("New grant"));
	setWindowTitle(edit_mode ? tr("Edit grant dialog") : tr("New grant dialog"));

	ui->tvPaths->setModel(&m_pathsModel);
	ui->tvPaths->horizontalHeader()->resizeSections(QHeaderView::ResizeToContents);
	ui->tvPaths->verticalHeader()->setDefaultSectionSize(static_cast<int>(fontMetrics().height() * 1.3));
	ui->tvPaths->setItemDelegate(new PathsTableItemDelegate(this));

	connect(ui->tbAddRow, &QToolButton::clicked, this, &DlgAddEditGrant::onAddRowClicked);
	connect(ui->tbDeleteRow, &QToolButton::clicked, this, &DlgAddEditGrant::onDeleteRowClicked);

	m_rpcConnection = rpc_connection;

	if(m_rpcConnection == nullptr){
		ui->lblStatus->setText(tr("Connection to shv does not exist."));
	}
}

DlgAddEditGrant::~DlgAddEditGrant()
{
	delete ui;
}

DlgAddEditGrant::DialogType DlgAddEditGrant::dialogType()
{
	return m_dialogType;
}

void DlgAddEditGrant::init(const QString &grant_name)
{
	ui->leGrantName->setText(grant_name);
	callGetGrantInfo();
	callGetGrantPaths();
}

QString DlgAddEditGrant::grantName()
{
	return ui->leGrantName->text();
}

std::string DlgAddEditGrant::aclEtcGrantsNodePath()
{
	return m_aclEtcNodePath + "grants";
}

std::string DlgAddEditGrant::aclEtcPathsNodePath()
{
	return m_aclEtcNodePath + "paths";
}

void DlgAddEditGrant::accept()
{
	if (dialogType() == DialogType::Add){
		if ((!grantName().isEmpty())){
			ui->lblStatus->setText(tr("Adding new grant ..."));
			callAddGrant();
		}
		else {
			ui->lblStatus->setText(tr("Grant name or grants is empty."));
		}
	}
	else if (dialogType() == DialogType::Edit){
		callEditGrant();
	}
}

void DlgAddEditGrant::callAddGrant()
{
	if (m_rpcConnection == nullptr)
		return;

	shv::chainpack::RpcValue::Map params = createParamsMap();

	int rqid = m_rpcConnection->nextRequestId();
	shv::iotqt::rpc::RpcResponseCallBack *cb = new shv::iotqt::rpc::RpcResponseCallBack(m_rpcConnection, rqid, this);

	cb->start(this, [this](const shv::chainpack::RpcResponse &response) {
		if (response.isValid()){
			if(response.isError()) {
				ui->lblStatus->setText(tr("Failed to add grant.") + QString::fromStdString(response.error().toString()));
			}
			else{
				callSetGrantPaths();
			}
		}
		else{
			ui->lblStatus->setText(tr("Request timeout expired"));
		}
	});

	m_rpcConnection->callShvMethod(rqid, aclEtcGrantsNodePath(), "addGrant", params);
}

void DlgAddEditGrant::callGetGrants()
{
	if (m_rpcConnection == nullptr)
		return;

	ui->lblStatus->setText(tr("Loading grants ..."));

	int rqid = m_rpcConnection->nextRequestId();
	shv::iotqt::rpc::RpcResponseCallBack *cb = new shv::iotqt::rpc::RpcResponseCallBack(m_rpcConnection, rqid, this);

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

	m_rpcConnection->callShvMethod(rqid, grantNameShvPath() + GRANTS, "get");
}

void DlgAddEditGrant::callGetWeight()
{
	if (m_rpcConnection == nullptr)
		return;

	ui->lblStatus->setText(tr("Loading weight ..."));

	int rqid = m_rpcConnection->nextRequestId();
	shv::iotqt::rpc::RpcResponseCallBack *cb = new shv::iotqt::rpc::RpcResponseCallBack(m_rpcConnection, rqid, this);

	cb->start(this, [this](const shv::chainpack::RpcResponse &response) {
		if(response.isValid()){
			if(response.isError()) {
				ui->lblStatus->setText(QString::fromStdString(response.error().toString()));
			}
			else{
				ui->sbWeight->setValue((response.result().isInt())? response.result().toInt() : -1);
				ui->lblStatus->setText("");
			}
		}
		else{
			ui->lblStatus->setText(tr("Request timeout expired"));
		}
	});

	m_rpcConnection->callShvMethod(rqid, grantNameShvPath() + WEIGHT, "get");
}

void DlgAddEditGrant::callEditGrant()
{
	if (m_rpcConnection == nullptr)
		return;

	shv::chainpack::RpcValue::Map params = createParamsMap();

	ui->lblStatus->setText("Updating grant");

	int rqid = m_rpcConnection->nextRequestId();
	shv::iotqt::rpc::RpcResponseCallBack *cb = new shv::iotqt::rpc::RpcResponseCallBack(m_rpcConnection, rqid, this);

	cb->start(this, [this](const shv::chainpack::RpcResponse &response) {
		if (response.isValid()){
			if(response.isError()) {
				ui->lblStatus->setText(tr("Failed to edit grant.") + QString::fromStdString(response.error().toString()));
			}
			else{
				callSetGrantPaths();
			}
		}
		else{
			ui->lblStatus->setText(tr("Request timeout expired"));
		}
	});

	m_rpcConnection->callShvMethod(rqid, aclEtcGrantsNodePath(), "editGrant", params);
}

void DlgAddEditGrant::callGetGrantInfo()
{
	if(m_rpcConnection == nullptr){
		return;
	}

	int rqid = m_rpcConnection->nextRequestId();
	shv::iotqt::rpc::RpcResponseCallBack *cb = new shv::iotqt::rpc::RpcResponseCallBack(m_rpcConnection, rqid, this);

	cb->start(this, [this](const shv::chainpack::RpcResponse &response) {
		if (response.isValid()){
			if (response.isError()) {
				ui->lblStatus->setText(tr("Failed to call method ls.") + QString::fromStdString(response.error().toString()));
			}
			else{
				if (response.result().isList()){
					const shv::chainpack::RpcValue::List &nodes = response.result().toList();

					for (size_t i = 0; i < nodes.size(); i++){
						if (nodes.at(i).toStdString() == GRANTS){
							callGetGrants();
						}
						if (nodes.at(i).toStdString() == WEIGHT){
							callGetWeight();
						}
					}
				}
			}
		}
		else{
			ui->lblStatus->setText(tr("Request timeout expired"));
		}
	});

	m_rpcConnection->callShvMethod(rqid, grantNameShvPath(), "ls");
}

std::string DlgAddEditGrant::grantNameShvPath()
{
	return aclEtcGrantsNodePath() + '/' + grantName().toStdString() + "/";
}

void DlgAddEditGrant::callGetGrantPaths()
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
				(response.result().isMap())? m_pathsModel.setPaths(response.result().toMap()) : m_pathsModel.setPaths(shv::chainpack::RpcValue::Map());
				ui->tvPaths->resizeColumnsToContents();
				ui->lblStatus->setText("");
			}
		}
		else{
			ui->lblStatus->setText(tr("Request timeout expired"));
		}
	});

	m_rpcConnection->callShvMethod(rqid, aclEtcPathsNodePath(), "getGrantPaths", shv::chainpack::RpcValue(grantName().toStdString()));
}

void DlgAddEditGrant::callSetGrantPaths()
{
	if (m_rpcConnection == nullptr)
		return;
	

	ui->lblStatus->setText(tr("Updating paths ..."));

	int rqid = m_rpcConnection->nextRequestId();
	shv::iotqt::rpc::RpcResponseCallBack *cb = new shv::iotqt::rpc::RpcResponseCallBack(m_rpcConnection, rqid, this);

	cb->start(this, [this](const shv::chainpack::RpcResponse &response) {
		if(response.isValid()){
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

	shv::chainpack::RpcValue::List paths{grantName().toStdString(), m_pathsModel.paths()};
	m_rpcConnection->callShvMethod(rqid, aclEtcPathsNodePath(), "setGrantPaths", paths);
}

shv::chainpack::RpcValue::Map DlgAddEditGrant::createParamsMap()
{
	shv::chainpack::RpcValue::Map params;
	params["grantName"] = grantName().toStdString();

	shv::chainpack::RpcValue::List g = grants();
	if(!g.empty()){
		params[GRANTS] = g;
	}

	int weight = ui->sbWeight->value();
	if (weight > -1){
		params[WEIGHT] = weight;
	}
	return params;
}

shv::chainpack::RpcValue::List DlgAddEditGrant::grants()
{
	shv::chainpack::RpcValue::List grants;
	QStringList lst = ui->leGrants->text().split(",", QString::SplitBehavior::SkipEmptyParts);

	for (int i = 0; i < lst.count(); i++){
		grants.push_back(shv::chainpack::RpcValue::String(lst.at(i).trimmed().toStdString()));
	}

	return grants;
}

void DlgAddEditGrant::setGrants(const shv::chainpack::RpcValue::List &grants)
{
	QString g;

	for (size_t i = 0; i < grants.size(); i++){
		if(i > 0)
			g += ',';
		g += QString::fromStdString(grants[i].toStdString());
	}

	ui->leGrants->setText(g);
}

void DlgAddEditGrant::onAddRowClicked()
{
	m_pathsModel.addPath();
}

void DlgAddEditGrant::onDeleteRowClicked()
{
	m_pathsModel.deletePath(ui->tvPaths->currentIndex().row());
}
