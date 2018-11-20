#include "commonrpcclienthandle.h"

#include <shv/iotqt/node/shvnode.h>

#include <shv/coreqt/log.h>

#include <shv/core/stringview.h>
#include <shv/core/exception.h>

namespace rpc {

std::string CommonRpcClientHandle::Subscription::toRelativePath(const std::string &abs_path, bool &changed) const
{
	if(relativePath.empty()) {
		changed = false;
		return abs_path;
	}
	std::string ret = relativePath + abs_path.substr(absolutePath.size());
	changed = true;
	return ret;
}

static const std::string DDOT("../");

bool CommonRpcClientHandle::Subscription::isRelativePath(const std::string &path)
{
	shv::core::StringView p(path);
	return p.startsWith(DDOT);
}

std::string CommonRpcClientHandle::Subscription::toAbsolutePath(const std::string &mount_point, const std::string &rel_path)
{
	shv::core::StringView p(rel_path);
	size_t ddot_cnt = 0;
	while(p.startsWith(DDOT)) {
		ddot_cnt++;
		p = p.mid(DDOT.size());
	}
	std::string abs_path;
	if(ddot_cnt > 0 && !mount_point.empty()) {
		shv::core::StringViewList mpl = shv::iotqt::node::ShvNode::splitPath(mount_point);
		if(mpl.size() >= ddot_cnt) {
			mpl.resize(mpl.size() - ddot_cnt);
			abs_path = mpl.join('/') + '/' + p.toString();
			return abs_path;
		}
	}
	return rel_path;
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

void CommonRpcClientHandle::addSubscription(const std::string &rel_path, const std::string &method)
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
	Subscription subs(abs_path, rel_path, method);
	auto it = std::find(m_subscriptions.begin(), m_subscriptions.end(), subs);
	if(it == m_subscriptions.end()) {
		m_subscriptions.push_back(subs);
		//std::sort(m_subscriptions.begin(), m_subscriptions.end());
	}
	else {
		*it = subs;
	}
}

int CommonRpcClientHandle::isSubscribed(const std::string &path, const std::string &method) const
{
	shv::core::StringView shv_path(path);
	while(shv_path.length() && shv_path[0] == '/')
		shv_path = shv_path.mid(1);
	for (size_t i = 0; i < subscriptionCount(); ++i) {
		const Subscription &subs = subscriptionAt(i);
		if(subs.match(shv_path, method))
			return i;
	}
	return -1;
}

bool CommonRpcClientHandle::unsubscribeRejectedSignal(const std::string &path, const std::string &method)
{
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
		m_subscriptions.erase(m_subscriptions.begin() + most_explicit_subs_ix);
		return true;
	}
	return false;
}

} // namespace rpc
