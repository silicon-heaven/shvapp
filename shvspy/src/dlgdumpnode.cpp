#include "dlgdumpnode.h"
#include "ui_dlgdumpnode.h"
#include "servertreemodel/shvbrokernodeitem.h"

#include <QTextStream>

DlgDumpNode::DlgDumpNode(ShvNodeItem *root_node, QWidget *parent) :
	QDialog(parent),
	ui(new Ui::DlgDumpNode), m_rootNode(root_node)
{
	ui->setupUi(this);
	setModal(true);
}

DlgDumpNode::~DlgDumpNode()
{
	delete ui;
}

void DlgDumpNode::generateText()
{
	QString str;
	QTextStream out(&str);
	//generateText(out, 0, m_rootNode->nodeId(), 0);
	ui->textBrowser->setPlainText(str);
}
/*
void DlgDumpNode::generateText(QTextStream &out, int indent, const qfopcua::NodeId &node_id, int skip_chars)
{
	for (int i=0;i < indent;++i) {
		out << "\t";
	}
	QString nid = node_id.toString().mid(skip_chars);
	if (nid.startsWith('.')) {
		nid.remove(0,1);
	}
	out << nid << '\n';
	const qfopcua::NodeIdList lst = client()->getChildren(node_id);
	for (const auto &ndid : lst) {
		generateText(out, indent+1, ndid, node_id.toString().length());
	}
}

qfopcua::Client *DlgDumpNode::client()
{
	if (!m_client) {
		m_client = m_rootNode->serverNode()->clientConnection();
	}
	return m_client;
}
*/

