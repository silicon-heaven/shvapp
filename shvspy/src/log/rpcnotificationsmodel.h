#pragma once

#include "logtablemodelbase.h"

namespace shv { namespace chainpack { class RpcMessage; }}

class RpcNotificationsModel : public LogTableModelBase
{
	Q_OBJECT

	using Super = LogTableModelBase;
public:
	enum Columns {ColBroker = 0, ColShvPath, ColMethod, ColParams, ColCnt};
public:
	RpcNotificationsModel(QObject *parent = nullptr);

	QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
	int columnCount(const QModelIndex &) const override {return ColCnt;}

	void addLogRow(const std::string &broker_name, const shv::chainpack::RpcMessage &msg);
};
