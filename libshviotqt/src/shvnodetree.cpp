#include "shvnodetree.h"

#include <shv/coreqt/log.h>

namespace shv {
namespace iotqt {

ShvNodeTree::ShvNodeTree(QObject *parent)
	: QObject(parent)
	, m_root(new ShvNode(this))
{
	m_root->setNodeName(std::string()); // root node have to have empty nodeName for ShvNode::nodePath() proper function
	m_root->setObjectName("<ROOT>");
}

ShvNode *ShvNodeTree::mkdir(const ShvNode::String &path, ShvNode::String *path_rest)
{
	ShvNode::StringList lst = shv::core::String::split(path, '/');
	return mdcd(lst, path_rest, true);
}

ShvNode *ShvNodeTree::cd(const ShvNode::String &path, ShvNode::String *path_rest)
{
	ShvNode::StringList lst = shv::core::String::split(path, '/');
	//shvWarning() << path << "->" << shv::core::String::join(lst, '-');
	return mdcd(lst, path_rest, false);
}

ShvNode *ShvNodeTree::mdcd(const ShvNode::StringList &path, ShvNode::String *path_rest, bool create_dirs)
{
	ShvNode *ret = m_root;
	size_t ix;
	for (ix = 0; ix < path.size(); ++ix) {
		const ShvNode::String &s = path[ix];
		ShvNode *nd2 = ret->childNode(s);
		if(nd2 == nullptr) {
			if(create_dirs) {
				ret = new ShvNode(ret);
				ret->setNodeName(path[ix]);
			}
			else {
				//ret = nullptr;
				break;
			}
		}
		else {
			ret = nd2;
		}
	}
	if(path_rest) {
		auto path2 = ShvNode::StringList(path.begin() + ix, path.end());
		*path_rest = shv::core::String::join(path2, '/');
	}
	return ret;
}

bool ShvNodeTree::mount(const ShvNode::String &path, ShvNode *node)
{
	ShvNode::StringList lst = shv::core::String::split(path, '/');
	if(lst.empty()) {
		shvError() << "Cannot mount node on empty path:" << path;
		return false;
	}
	std::string last_id = lst.back();
	lst.pop_back();
	ShvNode *parent_nd = nullptr;
	if(lst.empty()) {
		parent_nd = m_root;
	}
	else {
		parent_nd = mdcd(lst, nullptr, true);
	}
	ShvNode *ch = parent_nd->childNode(last_id);
	if(ch) {
		shvError() << "Node exist allready:" << path;
		return false;
	}
	node->setParentNode(parent_nd);
	node->setNodeName(last_id);
	return true;
}

}}
