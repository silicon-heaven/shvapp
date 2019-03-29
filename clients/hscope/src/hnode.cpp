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
// HNode::Status
//===========================================================
HNode::Status HNode::Status::fromRpcValue(const shv::chainpack::RpcValue &val)
{
	HNode::Status ret;
	if(val.isMap()) {
		const shv::chainpack::RpcValue::Map &map = val.toMap();
		cp::RpcValue val = map.value("value");
		if(!val.isValid())
			val = map.value("val");
		if(val.isInt() || val.isUInt())
			ret.value = (Status::Value)val.toInt();
		else if(val.isString()) {
			const std::string &val_str = val.toString();
			if(val_str.size()) {
				char c = val_str.at(0);
				switch (c) {
				case 'o':
				case 'O':
					ret.value = Status::Value::Ok;
					break;
				case 'w':
				case 'W':
					ret.value = Status::Value::Warning;
					break;
				case 'e':
				case 'E':
					ret.value = Status::Value::Error;
					break;
				default:
					ret.value = Status::Value::Unknown;
					break;
				}
			}
		}
		else {
			shvWarning() << "Invalid HNode::Status rpc value:" << val.toStdString();
			ret.value = Status::Value::Unknown;
		}
		std::string msg = map.value("message").toString();
		if(msg.empty())
			msg = map.value("msg").toString();
		ret.message = msg;
	}
	return ret;
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
	if(parent) {
		connect(this, &HNode::statusChanged, parent, &HNode::onChildStatusChanged);
	}
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

std::string HNode::scriptsDir()
{
	return appConfigDir() + "/scripts";
}

std::string HNode::templatesDir()
{
	return appConfigDir() + "/templates";
}

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

std::string HNode::templateFileName()
{
	return std::string();
}

void HNode::load()
{
	m_confignode = new ConfigNode(this);
	//m_confignode->values();
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

void HNode::onChildStatusChanged()
{
	updateOverallStatus();
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

void HNode::setStatus(const HNode::Status &st)
{
	if(st == m_status)
		return;
	Status old_st = m_status;
	m_status = st;
	if(old_st.value != Status::Value::Unknown)
		emit statusChanged(shvPath(), st);
}

void HNode::setOverallStatus(const HNode::Status &st)
{
	if(st == m_overallStatus)
		return;
	Status old_st = m_overallStatus;
	m_overallStatus = st;
	if(old_st.value != Status::Value::Unknown)
		emit overallStatusChanged(shvPath(), st);
}

void HNode::updateOverallStatus()
{
	Status new_st = status();
	if(new_st.value != Status::Value::Unknown) {
		Status chst = overallChildrenStatus();
		if(chst.value != Status::Value::Unknown) {
			if(chst.value > new_st.value) {
				new_st.value = chst.value;
				new_st.message = chst.message;
			}
			setOverallStatus(new_st);
		}
	}
	setOverallStatus(Status());
}

HNode::Status HNode::overallChildrenStatus()
{
	Status ret;
	bool unknown_reported = false;
	for(HNode *chnd : findChildren<HNode*>(QString(), Qt::FindDirectChildrenOnly)) {
		const Status &chst = chnd->status();
		unknown_reported = chst.value == Status::Value::Unknown;
		if(chst.value > ret.value) {
			ret.value = chst.value;
			ret.message = chst.message;
		}
	}
	return unknown_reported? Status(): ret;
}

shv::chainpack::RpcValue HNode::configValueOnPath(const std::string &shv_path, const shv::chainpack::RpcValue &default_val) const
{
	shv::core::StringViewList lst = shv::iotqt::utils::ShvPath::split(shv_path);
	shv::chainpack::RpcValue val = m_confignode->valueOnPath(lst, !shv::core::Exception::Throw);
	if(val.isValid())
		return val;
	return default_val;
}


