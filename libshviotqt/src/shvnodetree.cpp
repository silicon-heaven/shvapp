#include "shvnodetree.h"

#include <shv/core/string.h>

ShvNodeTree::ShvNodeTree(QObject *parent)
	: QObject(parent)
{

}

ShvNode *ShvNodeTree::mdcd(const ShvNode::String &path, ShvNode::String *path_rest, bool create_dirs)
{
	ShvNode::StringList lst = shv::core::String::split(path);
	ShvNode *ret = m_root;
	size_t ix;
	for (ix = 0; ix < lst.size(); ++ix) {
		const ShvNode::String &s = lst[ix];
		ShvNode *nd2 = ret->childNode(s);
		if(nd2 == nullptr) {
			if(create_dirs) {
				ret = new ShvNode(ret);
				ret->setNodeName(lst[ix]);
			}
			else {
				break;
			}
		}
		else {
			ret = nd2;
		}
	}
	if(path_rest) {
		lst = ShvNode::StringList(lst.begin() + ix, lst.end());
		*path_rest = shv::core::String::join(lst, '/');
	}
	return ret;
}
