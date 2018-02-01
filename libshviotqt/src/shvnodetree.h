#pragma once

#include "shvnode.h"

#include <shv/core/string.h>

#include <QObject>

namespace shv {
namespace iotqt {

class ShvNode;

class SHVIOTQT_DECL_EXPORT ShvNodeTree : public QObject
{
	Q_OBJECT
public:
	explicit ShvNodeTree(QObject *parent = nullptr);

	ShvNode* mkdir(const ShvNode::String &path, ShvNode::String *path_rest = nullptr);
	ShvNode* cd(const ShvNode::String &path, ShvNode::String *path_rest = nullptr);
	bool mount(const ShvNode::String &path, ShvNode *node);
protected:
	ShvNode* mdcd(const ShvNode::StringList &path, ShvNode::String *path_rest, bool create_dirs);
protected:
	//std::map<std::string, ShvNode*> m_root;
	ShvNode* m_root = nullptr;
};

}}
