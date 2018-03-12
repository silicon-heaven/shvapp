#ifndef DLGDUMPNODE_H
#define DLGDUMPNODE_H

#include <QDialog>

class QTextStream;

namespace Ui {
class DlgDumpNode;
}

class ShvNodeItem;

class DlgDumpNode : public QDialog
{
	Q_OBJECT

public:
	explicit DlgDumpNode(ShvNodeItem* root_node, QWidget *parent = 0);
	~DlgDumpNode();
	void generateText();
protected:
	//void generateText(QTextStream& out, int indent, const qfopcua::NodeId& node_id, int skip_chars);
	//qfopcua::Client* client();
private:
	Ui::DlgDumpNode *ui;
	ShvNodeItem* m_rootNode;
	//qfopcua::Client* m_client = nullptr;
};

#endif // DLGDUMPNODE_H
