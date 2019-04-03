#include "hnode.h"
#include "hscopeapp.h"
#include "appclioptions.h"
#include "hnodebroker.h"
#include "confignode.h"

#include <shv/coreqt/log.h>
#include <shv/iotqt/utils/shvpath.h>

#include <QDirIterator>

namespace cp = shv::chainpack;

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
			ret.value = (NodeStatus::Value)val.toInt();
		else if(val.isString()) {
			const std::string &val_str = val.toString();
			if(val_str.size()) {
				char c = val_str.at(0);
				switch (c) {
				case '0':
				case 'o':
				case 'O':
					ret.value = NodeStatus::Value::Ok;
					break;
				case '1':
				case 'w':
				case 'W':
					ret.value = NodeStatus::Value::Warning;
					break;
				case '2':
				case 'e':
				case 'E':
					ret.value = NodeStatus::Value::Error;
					break;
				default:
					ret.value = NodeStatus::Value::Unknown;
					break;
				}
			}
		}
		else {
			shvWarning() << "Invalid HNode::Status rpc value:" << val.toCpon();
			ret.value = NodeStatus::Value::Unknown;
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

const char *NodeStatus::valueToString(NodeStatus::Value val)
{
	switch (val) {
	case Value::Unknown: return "Unknown";
	case Value::Ok: return "Ok";
	case Value::Warning: return "Warning";
	case Value::Error: return "Error";
	}
	return "impossible";
}

const char *NodeStatus::valueToStringAbbr(NodeStatus::Value val)
{
	switch (val) {
	case Value::Unknown: return "???";
	case Value::Ok: return "OK";
	case Value::Warning: return "WARN";
	case Value::Error: return "ERR";
	}
	return "impossible";
}

//===========================================================
// HNode
//===========================================================
static const char METH_RELOAD[] = "reload";
static const char METH_STATUS[] = "status";
static const char METH_STATUS_CHANGED[] = "statusChanged";
static const char METH_OVERALL_STATUS[] = "overallStatus";
static const char METH_OVERALL_STATUS_CHANGED[] = "overallStatusChanged";
static std::vector<cp::MetaMethod> meta_methods {
	{cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::GRANT_BROWSE},
	{cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::GRANT_BROWSE},
	{METH_STATUS, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsGetter, cp::Rpc::GRANT_READ},
	{METH_STATUS_CHANGED, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsSignal, cp::Rpc::GRANT_READ},
	{METH_OVERALL_STATUS, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsGetter, cp::Rpc::GRANT_READ},
	{METH_OVERALL_STATUS_CHANGED, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsSignal, cp::Rpc::GRANT_READ},
	{METH_RELOAD, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::None, cp::Rpc::GRANT_CONFIG},
};

HNode::HNode(const std::string &node_id, HNode *parent)
	: Super(node_id, &meta_methods, parent)
{
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
	m_confignode = new ConfigNode(this);
	//m_confignode->values();
	//connect(this, &HNode::statusChanged, qobject_cast<HNode*>(parent()), &HNode::onChildStatusChanged);
	HNode *parent_hnode = qobject_cast<HNode*>(parent());
	if(parent_hnode)
		connect(this, &HNode::overallStatusChanged, parent_hnode, &HNode::updateOverallStatus);
}

void HNode::reload()
{
	qDeleteAll(findChildren<HNode*>(QString(), Qt::FindDirectChildrenOnly));
	qDeleteAll(findChildren<ConfigNode*>(QString(), Qt::FindDirectChildrenOnly));
	load();
}

shv::chainpack::RpcValue HNode::callMethod(const shv::iotqt::node::ShvNode::StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params)
{
	if(shv_path.empty()) {
		if(method == METH_STATUS) {
			return status().toRpcValue();
		}
		if(method == METH_OVERALL_STATUS) {
			return overallStatus().toRpcValue();
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
	if(st == m_status)
		return;
	//NodeStatus old_st = m_status;
	m_status = st;
	//shvWarning() << "emit" << shvPath() << "statusChanged" << st.toRpcValue().toCpon();
	emit statusChanged(shvPath(), st);
}

void HNode::setOverallStatus(const NodeStatus &st)
{
	if(st == m_overallStatus)
		return;
	//NodeStatus old_st = m_overallStatus;
	m_overallStatus = st;
	//shvWarning() << "emit" << shvPath() << "overallStatusChanged" << st.toRpcValue().toCpon();
	emit overallStatusChanged(shvPath(), st);
}

void HNode::updateOverallStatus()
{
	shvLogFuncFrame() << shvPath() << "status:" << status().toString();
	NodeStatus st = status();
	NodeStatus chst = overallChildrenStatus();
	if(chst.value > st.value) {
		st.value = chst.value;
		st.message = chst.message;
	}
	shvDebug() << "\t overall status:" << st.toString();
	setOverallStatus(st);
}

NodeStatus HNode::overallChildrenStatus()
{
	shvLogFuncFrame() << shvPath();
	NodeStatus ret;
	QList<HNode*> lst = findChildren<HNode*>(QString(), Qt::FindDirectChildrenOnly);
	for(HNode *chnd : lst) {
		const NodeStatus &chst = chnd->overallStatus();
		shvDebug().color(NecroLog::Color::Yellow) << "\t child:" << chnd->shvPath() << "overall status" << chst.toString();
		if(chst.value > ret.value) {
			ret.value = chst.value;
			ret.message = chst.message;
		}
	}
	return ret;
}

shv::chainpack::RpcValue HNode::configValueOnPath(const std::string &shv_path, const shv::chainpack::RpcValue &default_val) const
{
	shv::core::StringViewList lst = shv::iotqt::utils::ShvPath::split(shv_path);
	shv::chainpack::RpcValue val = m_confignode->valueOnPath(lst, !shv::core::Exception::Throw);
	if(val.isValid())
		return val;
	return default_val;
}


