#pragma once

#include "logtablemodel.h"

namespace shv { namespace chainpack { class RpcMessage; }}

class RpcNotificationsModel : public LogTableModel
{
	Q_OBJECT

	using Super = LogTableModel;
public:
	enum Columns {ColBroker = 0, ColShvPath, ColMethod, ColParams, ColCnt};
public:
	RpcNotificationsModel(QObject *parent = 0);

	QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
	int columnCount(const QModelIndex &) const override {return ColCnt;}

	void addLogRow(const std::string &broker_name, const shv::chainpack::RpcMessage &msg);
};
