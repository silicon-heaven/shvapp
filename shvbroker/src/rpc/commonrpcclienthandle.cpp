#include "commonrpcclienthandle.h"

#include <shv/iotqt/node/shvnode.h>

#include <shv/coreqt/log.h>

#include <shv/core/stringview.h>
#include <shv/core/exception.h>

#define logSubscriptionsD() nCDebug("Subscr").color(NecroLog::Color::Yellow)
#define logSubsResolveD() nCDebug("SubsRes").color(NecroLog::Color::LightGreen)

namespace rpc {

//=====================================================================
// CommonRpcClientHandle::Subscription
//=====================================================================
std::string CommonRpcClientHandle::Subscription::toRelativePath(const std::string &abs_path) const
{
	if(relativePath.empty()) {
		return abs_path;
	}
	std::string ret = relativePath + abs_path.substr(absolutePath.size());
	return ret;
}

static const std::string DDOT_SLASH("../");
static const std::string DDOT("..");

CommonRpcClientHandle::Subscription::Subscription(const std::string &ap, const std::string &rp, const std::string &m)
	: relativePath(rp)
	, method(m)
{
	size_t ix1 = 0;
	while(ix1 < ap.size()) {
		if(ap[ix1] == '/')
			ix1++;
		else
			break;
	}
	size_t len = ap.size();
	while(len > 0) {
		if(ap[len - 1] == '/')
			len--;
		else
			break;
	}
	absolutePath = ap.substr(ix1, len);
}

bool CommonRpcClientHandle::Subscription::isRelativePath(const std::string &path)
{
	shv::core::StringView p(path);
	return p == DDOT || p.startsWith(DDOT_SLASH);
}

std::string CommonRpcClientHandle::Subscription::toAbsolutePath(const std::string &mount_point, const std::string &rel_path)
{
	shvDebug() << "mount point:" << mount_point << "rel path:" << rel_path;
	if(!isRelativePath(rel_path))
		return rel_path;

	shv::core::StringViewList plst = shv::iotqt::node::ShvNode::splitShvPath(mount_point);
	for(const auto &p : shv::iotqt::node::ShvNode::splitShvPath(rel_path))
		plst.push_back(p);
	while(true) {
		ssize_t ix = plst.indexOf(DDOT);
		if(ix <= 0)
			break;
		for(ssize_t i = ix; i < static_cast<ssize_t>(plst.size())-1; i++)
			plst[i-1] = plst[i+1];
		plst.resize(plst.size() - 2);
	}
	shv::core::StringView p(rel_path);
	size_t ddot_cnt = 0;
	while(p.startsWith(DDOT_SLASH)) {
		ddot_cnt++;
		p = p.mid(DDOT_SLASH.size());
	}
	std::string abs_path = plst.join('/');
	shvDebug() << "mount point:" << mount_point << "rel path:" << rel_path << "-->" << abs_path;
	return abs_path;
}
/*
bool CommonRpcClientHandle::Subscription::operator<(const CommonRpcClientHandle::Subscription &o) const
{
	int i = absolutePath.compare(o.absolutePath);
	if(i == 0)
		return method < o.method;
	return (i < 0);
}
*/
bool CommonRpcClientHandle::Subscription::operator==(const CommonRpcClientHandle::Subscription &o) const
{
	int i = absolutePath.compare(o.absolutePath);
	if(i == 0)
		return method == o.method;
	return false;
}

bool CommonRpcClientHandle::Subscription::match(const shv::core::StringView &shv_path, const shv::core::StringView &shv_method) const
{
	//shvInfo() << pathPattern << ':' << method << "match" << shv_path.toString() << ':' << shv_method.toString();// << "==" << true;
	if(shv_path.startsWith(absolutePath)) {
		if(shv_path.length() == absolutePath.length())
			return (method.empty() || shv_method == method);
		if(shv_path.length() > absolutePath.length()
				&& (absolutePath.empty() || shv_path[absolutePath.length()] == '/'))
			return (method.empty() || shv_method == method);
	}
	return false;
}
//=====================================================================
// CommonRpcClientHandle
//=====================================================================
CommonRpcClientHandle::CommonRpcClientHandle()
{
}

CommonRpcClientHandle::~CommonRpcClientHandle()
{
}

void CommonRpcClientHandle::addMountPoint(const std::string &mp)
{
	m_mountPoints.push_back(mp);
}

unsigned CommonRpcClientHandle::addSubscription(const std::string &rel_path, const std::string &method)
{
	std::string abs_path = rel_path;
	if(Subscription::isRelativePath(abs_path)) {
		const std::vector<std::string> &mps = mountPoints();
		if(mps.empty())
			SHV_EXCEPTION("Cannot subscribe relative path on unmounted device.");
		if(mps.size() > 1)
			SHV_EXCEPTION("Cannot subscribe relative path on device mounted to more than single node.");
		abs_path = Subscription::toAbsolutePath(mps[0], rel_path);
	}
	logSubscriptionsD() << "adding subscription for connection id:" << connectionId() << "path:" << abs_path << "method:" << method;
	Subscription subs(abs_path, rel_path, method);
	auto it = std::find(m_subscriptions.begin(), m_subscriptions.end(), subs);
	if(it == m_subscriptions.end()) {
		m_subscriptions.push_back(subs);
		//std::sort(m_subscriptions.begin(), m_subscriptions.end());
		return m_subscriptions.size() - 1;
	}
	else {
		*it = subs;
		return (it - m_subscriptions.begin());
	}
}

bool CommonRpcClientHandle::removeSubscription(const std::string &rel_path, const std::string &method)
{
	std::string abs_path = rel_path;
	if(Subscription::isRelativePath(abs_path)) {
		const std::vector<std::string> &mps = mountPoints();
		if(mps.empty())
			SHV_EXCEPTION("Cannot unsubscribe relative path on unmounted device.");
		if(mps.size() > 1)
			SHV_EXCEPTION("Cannot unsubscribe relative path on device mounted to more than single node.");
		abs_path = Subscription::toAbsolutePath(mps[0], rel_path);
	}
	logSubscriptionsD() << "removing subscription for connection id:" << connectionId() << "path:" << abs_path << "method:" << method;
	Subscription subs(abs_path, rel_path, method);
	auto it = std::find(m_subscriptions.begin(), m_subscriptions.end(), subs);
	if(it == m_subscriptions.end()) {
		return false;
	}
	else {
		m_subscriptions.erase(it);
		return true;
	}
}

int CommonRpcClientHandle::isSubscribed(const std::string &path, const std::string &method) const
{
	shv::core::StringView shv_path(path);
	while(shv_path.length() && shv_path[0] == '/')
		shv_path = shv_path.mid(1);
	while(shv_path.value(-1) == '/')
		shv_path = shv_path.mid(0, shv_path.length()-1);
	logSubsResolveD() << connectionId() << "checking if subcribed path:" << shv_path.toString() << "method:" << method;
	for (size_t i = 0; i < subscriptionCount(); ++i) {
		const Subscription &subs = subscriptionAt(i);
		logSubsResolveD() << "\tvs. path:" << subs.absolutePath << "method:" << subs.method;
		if(subs.match(shv_path, method)) {
			logSubsResolveD() << "\tHIT";
			return (int)i;
		}
	}
	return -1;
}

std::string CommonRpcClientHandle::toSubscribedPath(const Subscription &subs, const std::string &abs_path) const
{
	return subs.toRelativePath(abs_path);
}

bool CommonRpcClientHandle::rejectNotSubscribedSignal(const std::string &path, const std::string &method)
{
	logSubscriptionsD() << "unsubscribing rejected signal, shv_path:" << path << "method:" << method;
	int most_explicit_subs_ix = -1;
	size_t max_path_len = 0;
	shv::core::StringView shv_path(path);
	for (size_t i = 0; i < subscriptionCount(); ++i) {
		const Subscription &subs = subscriptionAt(i);
		if(subs.match(shv_path, method)) {
			if(subs.method.empty()) {
				most_explicit_subs_ix = i;
				break;
			}
			if(subs.absolutePath.size() > max_path_len) {
				max_path_len = subs.absolutePath.size();
				most_explicit_subs_ix = i;
			}
		}
	}
	if(most_explicit_subs_ix >= 0) {
		logSubscriptionsD() << "\t found subscription:" << m_subscriptions.at(most_explicit_subs_ix).toString();
		m_subscriptions.erase(m_subscriptions.begin() + most_explicit_subs_ix);
		return true;
	}
	logSubscriptionsD() << "\t not found";
	return false;
}

} // namespace rpc
