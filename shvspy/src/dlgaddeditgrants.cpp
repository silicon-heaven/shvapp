#include "dlgaddeditgrants.h"
#include "ui_dlgaddeditgrants.h"

#include "shv/core/log.h"

static const std::string WEIGHT = "weight";
static const std::string GRANTS = "grants";

DlgAddEditGrants::DlgAddEditGrants(QWidget *parent, shv::iotqt::rpc::ClientConnection *rpc_connection, const std::string &acl_etc_node_path, DlgAddEditGrants::DialogType dt) :
	QDialog(parent),
	ui(new Ui::DlgAddEditGrants),
	m_aclEtcNodePath(acl_etc_node_path)
{
	ui->setupUi(this);
	m_dialogType = dt;
	bool edit_mode = (m_dialogType == DialogType::Edit);

	ui->leGrantName->setEnabled(!edit_mode);
	ui->groupBox->setTitle(edit_mode ? tr("Edit grant") : tr("New grant"));
	setWindowTitle(edit_mode ? tr("Edit grant dialog") : tr("New grant dialog"));

	static constexpr double ROW_HEIGHT_RATIO = 1.3;
	static QStringList INFO_HEADER_NAMES {{ tr("Path"), tr("Grant"), tr("Weight")}};

	ui->twPaths->setColumnCount(INFO_HEADER_NAMES.count());
	ui->twPaths->setHorizontalHeaderLabels(INFO_HEADER_NAMES);
	ui->twPaths->horizontalHeader()->setStretchLastSection(true);
	ui->twPaths->horizontalHeader()->resizeSection(0, static_cast<int>(width() * 0.7));
	ui->twPaths->verticalHeader()->setDefaultSectionSize(static_cast<int>(ui->twPaths->fontMetrics().height() * ROW_HEIGHT_RATIO));
	ui->twPaths->verticalHeader()->setVisible(false);

	connect(ui->tbAddRow, &QToolButton::clicked, this, &DlgAddEditGrants::onAddRowClicked);
	connect(ui->tbDeleteRow, &QToolButton::clicked, this, &DlgAddEditGrants::onDeleteRowClicked);

	m_rpcConnection = rpc_connection;

	if(m_rpcConnection == nullptr){
		ui->lblStatus->setText(tr("Connection to shv does not exist."));
	}
}

DlgAddEditGrants::~DlgAddEditGrants()
{
	delete ui;
}

DlgAddEditGrants::DialogType DlgAddEditGrants::dialogType()
{
	return m_dialogType;
}

void DlgAddEditGrants::init(const QString &grant_name)
{
	ui->leGrantName->setText(grant_name);
	callGetGrantInfo();
	callGetGrantPaths();
}

QString DlgAddEditGrants::grantName()
{
	return ui->leGrantName->text();
}

std::string DlgAddEditGrants::aclEtcGrantsNodePath()
{
	return m_aclEtcNodePath + "grants";
}

std::string DlgAddEditGrants::aclEtcPathsNodePath()
{
	return m_aclEtcNodePath + "paths";
}

void DlgAddEditGrants::accept()
{
	if (dialogType() == DialogType::Add){
		if ((!grantName().isEmpty())){
			ui->lblStatus->setText(tr("Adding new grant ..."));
			callAddGrant();
			callSetGrantPaths();
		}
		else {
			ui->lblStatus->setText(tr("Grant name or grants is empty."));
		}
	}
	else if (dialogType() == DialogType::Edit){
		callEditGrant();
		callSetGrantPaths();
	}
}

void DlgAddEditGrants::callAddGrant()
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
				QDialog::accept();
			}
		}
		else{
			ui->lblStatus->setText(tr("Request timeout expired"));
		}
	});

	m_rpcConnection->callShvMethod(rqid, aclEtcGrantsNodePath(), "addGrant", params);
}

void DlgAddEditGrants::callGetGrants()
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

void DlgAddEditGrants::callGetWeight()
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

void DlgAddEditGrants::callEditGrant()
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
				QDialog::accept();
			}
		}
		else{
			ui->lblStatus->setText(tr("Request timeout expired"));
		}
	});

	m_rpcConnection->callShvMethod(rqid, aclEtcGrantsNodePath(), "editGrant", params);
}

void DlgAddEditGrants::callGetGrantInfo()
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
					shv::chainpack::RpcValue::List nodes = response.result().toList();

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

std::string DlgAddEditGrants::grantNameShvPath()
{
	return aclEtcGrantsNodePath() + '/' + grantName().toStdString() + "/";
}

void DlgAddEditGrants::callGetGrantPaths()
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

	m_rpcConnection->callShvMethod(rqid, aclEtcPathsNodePath(), "getGrantPaths", shv::chainpack::RpcValue(grantName().toStdString()));
}

void DlgAddEditGrants::callSetGrantPaths()
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

			}
		}
		else{
			ui->lblStatus->setText(tr("Request timeout expired"));
		}
	});

	m_rpcConnection->callShvMethod(rqid, aclEtcPathsNodePath(), "setGrantPaths", paths());
}

void DlgAddEditGrants::setPaths(const shv::chainpack::RpcValue::Map &paths)
{
	ui->twPaths->clearContents();
	ui->twPaths->setRowCount(0);

	std::vector<std::string> keys = paths.keys();

	for (size_t i = 0; i < keys.size(); i++){
		int row = ui->twPaths->rowCount();
		const shv::chainpack::RpcValue::Map &path = paths.value(keys[i]).toMap();
		ui->twPaths->insertRow(row);
		ui->twPaths->setItem(row, ColPath, new QTableWidgetItem(QString::fromStdString((keys[i]))));
		ui->twPaths->setItem(row, ColGrant, new QTableWidgetItem(QString::fromStdString((path.value("grant").toStdString()))));
		ui->twPaths->setItem(row, ColWeight, new QTableWidgetItem(path.hasKey("weight") ? QString::number(path.value("weight").toInt()) : ""));
	}

	ui->lblStatus->setText("");
}

shv::chainpack::RpcValue::Map DlgAddEditGrants::paths()
{
	shv::chainpack::RpcValue::Map paths;

	for(int row = 0; row < ui->twPaths->rowCount(); row++){
		std::string path = ui->twPaths->item(row, ColPath)->data(Qt::DisplayRole).toString().toStdString();
		std::string grant = ui->twPaths->item(row, ColGrant)->data(Qt::DisplayRole).toString().toStdString();
		QString weight_str = ui->twPaths->item(row, ColWeight)->data(Qt::DisplayRole).toString();

		if (!path.empty() && !grant.empty()){
			shv::chainpack::RpcValue::Map path_settings;
			path_settings["grant"] = shv::chainpack::RpcValue(grant);

			if (!weight_str.isEmpty()){
				bool ok;
				int weight = weight_str.toInt(&ok);

				if(ok){
					path_settings["weight"] = weight;
				}
			}

			paths[path] = path_settings;
		}
	}

	return shv::chainpack::RpcValue::Map({{grantName().toStdString(), paths}});
}

shv::chainpack::RpcValue::Map DlgAddEditGrants::createParamsMap()
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

shv::chainpack::RpcValue::List DlgAddEditGrants::grants()
{
	shv::chainpack::RpcValue::List grants;
	QStringList lst = ui->leGrants->text().split(",", QString::SplitBehavior::SkipEmptyParts);

	for (int i = 0; i < lst.count(); i++){
		grants.push_back(shv::chainpack::RpcValue::String(lst.at(i).trimmed().toStdString()));
	}

	return grants;
}

void DlgAddEditGrants::setGrants(const shv::chainpack::RpcValue::List &grants)
{
	QString g;

	for (size_t i = 0; i < grants.size(); i++){
		if(i > 0)
			g += ',';
		g += QString::fromStdString(grants[i].toStdString());
	}

	ui->leGrants->setText(g);
}

void DlgAddEditGrants::onAddRowClicked()
{
	int ix = ui->twPaths->rowCount();
	ui->twPaths->insertRow(ix);
	ui->twPaths->setItem(ix, ColPath, new QTableWidgetItem(""));
	ui->twPaths->setItem(ix, ColGrant, new QTableWidgetItem(""));
	ui->twPaths->setItem(ix, ColWeight, new QTableWidgetItem(""));
}

void DlgAddEditGrants::onDeleteRowClicked()
{
	int current_row = ui->twPaths->currentRow();

	if (current_row >= 0){
		ui->twPaths->removeRow(current_row);
	}
}
