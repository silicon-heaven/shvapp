#include "application.h"
#include "appclioptions.h"
#include "devicemonitor.h"
#include "getlogmerge.h"
#include "rootnode.h"
#include "siteitem.h"

#include <shv/iotqt/rpc/deviceconnection.h>
#include <shv/iotqt/node/shvnodetree.h>
#include <shv/iotqt/rpc/rpc.h>
#include <shv/coreqt/log.h>
#include <shv/coreqt/exception.h>
#include <shv/chainpack/metamethod.h>

#include <QElapsedTimer>

namespace cp = shv::chainpack;

static const char METH_GET_VERSION[] = "version";
static const char METH_RELOAD_SITES[] = "reloadSites";
static const char METH_GET_UPTIME[] = "uptime";

static std::vector<cp::MetaMethod> root_meta_methods {
	{ cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, false, cp::Rpc::ROLE_BROWSE },
	{ cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, false, cp::Rpc::ROLE_BROWSE },
	{ cp::Rpc::METH_DEVICE_ID, cp::MetaMethod::Signature::RetVoid, false, cp::Rpc::ROLE_READ },
	{ cp::Rpc::METH_APP_NAME, cp::MetaMethod::Signature::RetVoid, false, cp::Rpc::ROLE_READ },
    { METH_GET_VERSION, cp::MetaMethod::Signature::RetVoid, false, cp::Rpc::ROLE_READ },
	{ METH_RELOAD_SITES, cp::MetaMethod::Signature::RetVoid, false, cp::Rpc::ROLE_COMMAND },
	{ METH_GET_UPTIME, cp::MetaMethod::Signature::RetVoid, false, cp::Rpc::ROLE_READ },
};

static std::vector<cp::MetaMethod> branch_meta_methods {
	{ cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, false, cp::Rpc::ROLE_BROWSE },
	{ cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, false, cp::Rpc::ROLE_BROWSE },
	{ cp::Rpc::METH_GET_LOG, cp::MetaMethod::Signature::RetParam, false, cp::Rpc::ROLE_READ },
};

static std::vector<cp::MetaMethod> leaf_meta_methods {
	{ cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, false, cp::Rpc::ROLE_BROWSE },
	{ cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, false, cp::Rpc::ROLE_BROWSE },
	{ cp::Rpc::METH_GET_LOG, cp::MetaMethod::Signature::RetParam, false, cp::Rpc::ROLE_READ },
};

RootNode::RootNode(QObject *parent)
	: Super(parent)
{
}

const std::vector<shv::chainpack::MetaMethod> &RootNode::metaMethods(const shv::iotqt::node::ShvNode::StringViewList &shv_path)
{
	if (shv_path.empty()) {
		return root_meta_methods;
	}
	else {
		if (!Application::instance()->deviceMonitor()->monitoredDevices().contains(QString::fromStdString(shv_path.join('/')))) {
			return branch_meta_methods;
		}
		return leaf_meta_methods;
	}
}

size_t RootNode::methodCount(const StringViewList &shv_path)
{
	return metaMethods(shv_path).size();
}

const shv::chainpack::MetaMethod *RootNode::metaMethod(const StringViewList &shv_path, size_t ix)
{
	const std::vector<cp::MetaMethod> &meta_methods = metaMethods(shv_path);
	if (meta_methods.size() <= ix) {
		SHV_EXCEPTION("Invalid method index: " + std::to_string(ix) + " of: " + std::to_string(meta_methods.size()));
	}
	return &(meta_methods[ix]);
}

shv::chainpack::RpcValue RootNode::ls(const shv::core::StringViewList &shv_path, const shv::chainpack::RpcValue &params)
{
	Q_UNUSED(params);
	return ls(shv_path, 0, Application::instance()->deviceMonitor()->sites());
}

cp::RpcValue RootNode::ls(const shv::core::StringViewList &shv_path, size_t index, const SiteItem *site_item)
{
	if (shv_path.size() - index == 0) {
		cp::RpcValue::List items;
		for (int i = 0; i < site_item->children().count(); ++i) {
			items.push_back(cp::RpcValue::fromValue(site_item->children()[i]->objectName()));
		}
		return items;
	}
	QString key = QString::fromStdString(shv_path[index].toString());
	SiteItem *child = site_item->findChild<SiteItem*>(key);
	if (child) {
		return ls(shv_path, index + 1, child);
	}
	return cp::RpcValue::List();
}

void RootNode::onRpcMessageReceived(const shv::chainpack::RpcMessage &msg)
{
	if (msg.isRequest()) {
		cp::RpcRequest rq(msg);
		cp::RpcResponse resp = cp::RpcResponse::forRequest(rq.metaData());
		try {
			rq.setShvPath(rq.shvPath().toString());
			shv::chainpack::RpcValue result = processRpcRequest(rq);
			if (result.isValid()) {
				resp.setResult(result);
			}
			else {
				return;
			}
		}
		catch (shv::core::Exception &e) {
			resp.setError(cp::RpcResponse::Error::create(cp::RpcResponse::Error::MethodCallException, e.message()));
		}
		if (resp.requestId().toInt() > 0) { // RPC calls with requestID == 0 does not expect response
			sendRpcMessage(resp);
		}
    }
}

cp::RpcValue RootNode::processRpcRequest(const cp::RpcRequest &rq)
{
	const cp::RpcValue::String method = rq.method().toString();

	if (method == cp::Rpc::METH_APP_NAME) {
		return Application::instance()->applicationName().toStdString();
	}
	if (method == cp::Rpc::METH_DEVICE_ID) {
		return Application::instance()->cliOptions()->deviceId();
	}
    else if (method == METH_GET_VERSION) {
        return Application::instance()->applicationVersion().toStdString();
    }
	else if (method == METH_GET_UPTIME) {
		return Application::instance()->uptime().toStdString();
	}
	else if (method == cp::Rpc::METH_GET_LOG) {
		return getLog(rpcvalue_cast<QString>(rq.shvPath()), rq.params());
	}
	else if (method == METH_RELOAD_SITES) {
		Application::instance()->deviceMonitor()->downloadSites();
		return true;
	}
	return Super::processRpcRequest(rq);
}

cp::RpcValue RootNode::getLog(const QString &shv_path, const shv::chainpack::RpcValue &params) const
{
	shv::core::utils::ShvGetLogParams log_params;
	if (params.isValid()) {
		if (!params.isMap()) {
			SHV_QT_EXCEPTION("Bad param format");
		}
		log_params = params;
	}

	if (log_params.since.isValid() && log_params.until.isValid() && log_params.since.toDateTime() > log_params.until.toDateTime()) {
		SHV_QT_EXCEPTION("since is after until");
	}
	static int static_request_no = 0;
	int request_no = static_request_no++;
	QElapsedTimer tm;
	tm.start();
	shvInfo() << "got request" << request_no << "for log" << shv_path << "with params:\n" << log_params.toRpcValue().toCpon("    ");
	GetLogMerge request(shv_path, log_params);
	try {
		shv::core::utils::ShvMemoryJournal &result = request.getLog();
		shvInfo() << "request number" << request_no << "finished in" << tm.elapsed() << "ms with" << result.entries().size() << "records";
		return result.getLog(log_params);
	}
	catch (const shv::core::Exception &e) {
		shvInfo() << "request number" << request_no << "finished in" << tm.elapsed() << "ms with error" << e.message();
		throw;
	}
}
