#ifndef ERRORLOGMODEL_H
#define ERRORLOGMODEL_H

#include "logtablemodelbase.h"

#include <necrolog.h>

class ErrorLogModel : public LogTableModelBase
{
	Q_OBJECT

	using Super = LogTableModelBase;
public:
	ErrorLogModel(QObject *parent = nullptr);

	enum Cols {Level, TimeStamp, Message, UserData, Count};

	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
	int columnCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;

	QVariant data(const QModelIndex &index, int role) const Q_DECL_OVERRIDE;

	NecroLog::Level level(int row) const;

	void addLogRow(NecroLog::Level level, const std::string &msg, const QVariant &user_data = QVariant());

};

#endif // ERRORLOGMODEL_H
