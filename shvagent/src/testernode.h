#ifndef TESTERNODE_H
#define TESTERNODE_H

#include <shv/iotqt/node/shvnode.h>
#include <shv/chainpack/datachange.h>

class TesterNode : public shv::iotqt::node::MethodsTableNode
{
	Q_OBJECT

	using Super = shv::iotqt::node::MethodsTableNode;
public:
	TesterNode(shv::iotqt::node::ShvNode *parent);
};

class TesterPropertyNode : public shv::iotqt::node::MethodsTableNode
{
	Q_OBJECT

	using Super = shv::iotqt::node::MethodsTableNode;
public:
	TesterPropertyNode(const std::string &name, shv::iotqt::node::ShvNode *parent);

	shv::chainpack::RpcValue callMethod(const StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params) override;
private:
	shv::chainpack::DataChange m_dataChange;
};

#endif // TESTERNODE_H
