#ifndef GLGLOGVIEW_H
#define GLGLOGVIEW_H

#include <shv/core/utils.h>

#include <QDialog>

namespace shv { namespace chainpack { class RpcValue; }}
namespace shv { namespace iotqt { namespace rpc { class ClientConnection; }}}
namespace shv { namespace visu { namespace timeline { class GraphWidget; class GraphModel; class Graph;}}}

namespace Ui {
class DlgLogView;
}

class LogModel;
class QSortFilterProxyModel;

class DlgLogView : public QDialog
{
	Q_OBJECT

	SHV_FIELD_IMPL(QString, s, S, hvPath)

public:
	explicit DlgLogView(QWidget *parent = nullptr);
	~DlgLogView();

private:
	void downloadLog();
	void loadSettings();
	void saveSettings();

	shv::iotqt::rpc::ClientConnection* rpcConnection();
	shv::chainpack::RpcValue getLogParams();
	void parseLog(shv::chainpack::RpcValue log);

	void showInfo(const QString &msg = QString(), bool is_error = false);
private:
	Ui::DlgLogView *ui;

	LogModel *m_logModel = nullptr;
	QSortFilterProxyModel *m_sortFilterProxy = nullptr;

	shv::visu::timeline::GraphModel *m_graphModel = nullptr;
	shv::visu::timeline::Graph *m_graph = nullptr;
	shv::visu::timeline::GraphWidget *m_graphWidget = nullptr;
};

#endif // GLGLOGVIEW_H
