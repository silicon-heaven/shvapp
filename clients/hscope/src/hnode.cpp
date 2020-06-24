#include "hnode.h"
#include "hscopeapp.h"
#include "appclioptions.h"
#include "hnodebroker.h"
#include "hnodeagent.h"
#include "confignode.h"

#include <shv/coreqt/log.h>
#include <shv/core/utils/shvpath.h>

#include <QDirIterator>

namespace cp = shv::chainpack;

static const char KEY_DISABLED[] = "disabled";

//===========================================================
// HNodeStatus
//===========================================================
NodeStatus NodeStatus::fromRpcValue(const shv::chainpack::RpcValue &val)
{
	NodeStatus ret;
	if(val.isMap()) {
		const shv::chainpack::RpcValue::Map &map = val.toMap();
		cp::RpcValue val = map.value("value");
		if(!val.isValid())
			val = map.value("val");
		if(val.isInt() || val.isUInt())
			ret.level = (NodeStatus::Level)val.toInt();
		else if(val.isString()) {
			const std::string &val_str = val.toString();
			if(val_str.size()) {
				char c = val_str.at(0);
				switch (c) {
				case '0':
				case 'o':
				case 'O':
					ret.level = NodeStatus::Level::Ok;
					break;
				case '1':
				case 'w':
				case 'W':
					ret.level = NodeStatus::Level::Warning;
					break;
				case '2':
				case 'e':
				case 'E':
					ret.level = NodeStatus::Level::Error;
					break;
				default:
					ret.level = NodeStatus::Level::Unknown;
					break;
				}
			}
		}
		else {
			shvWarning() << "Invalid HNode::Status rpc value:" << val.toCpon();
			ret.level = NodeStatus::Level::Unknown;
		}
		std::string msg = map.value("message").toString();
		if(msg.empty())
			msg = map.value("msg").toString();
		ret.message = msg;
	}
	else {
		shvWarning() << "Invalid HNode::Status rpc value:" << val.toCpon();
	}
	return ret;
}

const char *NodeStatus::levelToString(NodeStatus::Level val)
{
	switch (val) {
	case Level::Disabled: return "Disabled";
	case Level::Unknown: return "Unknown";
	case Level::Ok: return "Ok";
	//case Value::Info: return "Info";
	case Level::Warning: return "Warning";
	case Level::Error: return "Error";
	}
	return "impossible";
}

const char *NodeStatus::levelToStringAbbr(NodeStatus::Level val)
{
	switch (val) {
	case Level::Unknown: return "???";
	case Level::Disabled: return "DIS";
	case Level::Ok: return "OK";
	//case Value::Info: return "INF";
	case Level::Warning: return "WARN";
	case Level::Error: return "ERR";
	}
	return "impossible";
}

//===========================================================
// HNode
//===========================================================
static const char METH_RELOAD[] = "reload";
static const char METH_STATUS[] = "status";
static const char METH_STATUS_CHANGED[] = "statusChanged";
static const char METH_UPDATE_OVERALL_STATUS[] = "updateOverallStatus";
static const char METH_OVERALL_STATUS[] = "overallStatus";
static const char METH_OVERALL_STATUS_CHANGED[] = "overallStatusChanged";
static std::vector<cp::MetaMethod> meta_methods {
	{cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_BROWSE},
	{cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_BROWSE},
	{METH_STATUS, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsGetter, cp::Rpc::ROLE_READ},
	{METH_STATUS_CHANGED, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsSignal, cp::Rpc::ROLE_READ},
	{METH_OVERALL_STATUS, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsGetter, cp::Rpc::ROLE_READ},
	{METH_OVERALL_STATUS_CHANGED, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsSignal, cp::Rpc::ROLE_READ},
	{METH_UPDATE_OVERALL_STATUS, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsGetter, cp::Rpc::ROLE_COMMAND},
	{METH_RELOAD, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_COMMAND},
};

HNode::HNode(const std::string &node_id, HNode *parent)
	: Super(node_id, &meta_methods, parent)
{
	connect(this, &HNode::statusChanged, this, &HNode::updateOverallStatus);
	connect(this, &HNode::statusChanged, HScopeApp::instance(), &HScopeApp::onHNodeStatusChanged);
	connect(this, &HNode::overallStatusChanged, HScopeApp::instance(), &HScopeApp::onHNodeOverallStatusChanged);
}

std::string HNode::appConfigDir()
{
	HScopeApp *app = HScopeApp::instance();
	return app->cliOptions()->configDir();
}

std::string HNode::nodeConfigDir()
{
	return appConfigDir() + '/' + shvPath();
}
/*
std::string HNode::scriptsDir()
{
	return appConfigDir() + "/scripts";
}
*/
std::string HNode::templatesDir()
{
	return appConfigDir() + "/template";
}
/*
std::string HNode::nodeConfigFilePath()
{
	std::string fn = nodeConfigDir() + "/config.cpon";
	if(!QFile::exists(QString::fromStdString(fn))) {
		std::string tfn = templateFileName();
		if(!tfn.empty()) {
			fn = templatesDir() + '/' + tfn;
		}
	}
	return fn;
}
*/
std::string HNode::templateFileName()
{
	return QString::fromUtf8(metaObject()->className()).mid(QStringLiteral("HNode").length()).toLower().toStdString()
			+ ".config";
}

void HNode::load()
{
	//shvInfo() << "HNode::load" << shvPath();
	m_configNode = new ConfigNode(this);
	//m_confignode->values();
	//connect(this, &HNode::statusChanged, qobject_cast<HNode*>(parent()), &HNode::onChildStatusChanged);
	HNode *parent_hnode = qobject_cast<HNode*>(parent());
	if(parent_hnode)
		connect(this, &HNode::overallStatusChanged, parent_hnode, &HNode::updateOverallStatus);
	if(isDisabled()) {
		setStatus({NodeStatus::Level::Disabled, "Node is disabled"});
	}
}

void HNode::reload()
{
	qDeleteAll(findChildren<HNode*>(QString(), Qt::FindDirectChildrenOnly));
	qDeleteAll(findChildren<ConfigNode*>(QString(), Qt::FindDirectChildrenOnly));
	load();
}

bool HNode::isDisabled() const
{
	return configValueOnPath(KEY_DISABLED).toBool();
}

shv::chainpack::RpcValue HNode::callMethod(const shv::iotqt::node::ShvNode::StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params)
{
	if(shv_path.empty()) {
		if(method == METH_STATUS) {
			return status().toRpcValue();
		}
		if(method == METH_OVERALL_STATUS) {
			return NodeStatus::levelToString(overallStatus());
		}
		if(method == METH_UPDATE_OVERALL_STATUS) {
			updateOverallStatus();
			return true;
		}
		if(method == METH_RELOAD) {
			reload();
			return true;
		}
	}
	return Super::callMethod(shv_path, method, params);
}

shv::iotqt::rpc::DeviceConnection *HNode::appRpcConnection()
{
	return HScopeApp::instance()->rpcConnection();
}

HNodeBroker *HNode::parentBrokerNode()
{
	return findParent<HNodeBroker *>();
}

HNodeAgent *HNode::parentAgentNode()
{
	return findParent<HNodeAgent *>();
}

std::vector<std::string> HNode::lsConfigDir()
{
	std::vector<std::string> ret;
	QString node_cfg_dir = QString::fromStdString(nodeConfigDir());
	shvDebug() << "ls dir:" << node_cfg_dir;
	QDirIterator it(node_cfg_dir, QDir::AllDirs | QDir::NoDotAndDotDot | QDir::Readable, QDirIterator::NoIteratorFlags);
	while (it.hasNext()) {
		it.next();
		shvDebug() << "\t" << it.fileName();
		ret.push_back(it.fileName().toStdString());
	}
	return ret;
}

void HNode::setStatus(const NodeStatus &st)
{
	if(st.level == m_status.level) {
		// message is not compared to avoid cyclic status message emit on save severity level
		// like not-connected -> connect-error -> not-connected -> ...
		return;
	}
	//NodeStatus old_st = m_status;
	m_status = st;
	//shvWarning() << "emit" << shvPath() << "statusChanged" << st.toRpcValue().toCpon();
	emit statusChanged(shvPath(), st);
}

void HNode::setOverallStatus(const NodeStatus::Level &level)
{
	shvLogFuncFrame() << shvPath() << "old status:" << NodeStatus::levelToString(m_overallStatus) << "new:" << NodeStatus::levelToString(level);
	if(level == m_overallStatus) {
		return;
	}
	//NodeStatus old_st = m_overallStatus;
	m_overallStatus = level;
	//shvWarning() << "emit" << shvPath() << "overallStatusChanged" << st.toRpcValue().toCpon();
	emit overallStatusChanged(shvPath(), level);
}

void HNode::updateOverallStatus()
{
	shvLogFuncFrame() << shvPath() << "status:" << status().toString();
	NodeStatus::Level olev = status().level;
	//shvDebug() << "updateOverallStatus:" << shvPath() << "node status:" << st.toString();
	if(isDisabled()) {
		//shvInfo() << "updateOverallStatus:" << shvPath() << "node is disabled";
	}
	else {
		NodeStatus::Level chlev = overallChildrenStatus();
		if(chlev < olev) {
			olev = chlev;
		}
	}
	shvDebug() << "\t overall status:" << NodeStatus::levelToString(olev);
	setOverallStatus(olev);
}

NodeStatus::Level HNode::overallChildrenStatus()
{
	shvLogFuncFrame() << shvPath();
	NodeStatus::Level ret = NodeStatus::Level::Unknown;
	QList<HNode*> lst = findChildren<HNode*>(QString(), Qt::FindDirectChildrenOnly);
	for(HNode *chnd : lst) {
		NodeStatus::Level chlev = chnd->overallStatus();
		//shvDebug() << "\t child:" << chnd->shvPath() << "overall status" << chst.toString();
		if(chlev < ret) {
			ret = chlev;
		}
	}
	return ret;
}

shv::chainpack::RpcValue HNode::configValueOnPath(const std::string &shv_path, const shv::chainpack::RpcValue &default_val) const
{
	shv::core::StringViewList lst = shv::core::utils::ShvPath::split(shv_path);
	shv::chainpack::RpcValue val = m_configNode->valueOnPath(lst, !shv::core::Exception::Throw);
	if(val.isValid())
		return val;
	return default_val;
}


