#include "errorlogmodel.h"

#include <QDateTime>
#include <QColor>

ErrorLogModel::ErrorLogModel(QObject *parent)
	: Super(parent)
{

}

QVariant ErrorLogModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
		switch (section) {
		case Cols::Level:
			return tr("Level");
		case Cols::TimeStamp:
			return tr("Time stamp");
		case Cols::Message:
			return tr("Message");
		case Cols::UserData:
			return tr("Data");
		};
		return Super::headerData(section, orientation, role);
	}
	return Super::headerData(section, orientation, role);
}

int ErrorLogModel::columnCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent)
	return Cols::Count;
}

QVariant ErrorLogModel::data(const QModelIndex &index, int role) const
{
	if (index.row() < 0 || index.row() >= rowCount()) {
		return QVariant();
	}
	switch (role) {
	case Qt::DisplayRole: {
		if(index.column() == Cols::Level) {
			return NecroLog::levelToString(static_cast<NecroLog::Level>(Super::data(index, role).toInt()));
		}
		if(index.column() == Cols::TimeStamp) {
			return Super::data(index, role).toDateTime().toString(Qt::ISODateWithMs);
		}
		break;
	}
	case Qt::BackgroundRole: {
		if(level(index.row()) == NecroLog::Level::Error)
			return QColor("lightsalmon");
		break;
	}
	};
	return Super::data(index, role);
}

NecroLog::Level ErrorLogModel::level(int row) const
{
	return static_cast<NecroLog::Level>(data(index(row, Cols::Level), Qt::EditRole).toInt());
}

void ErrorLogModel::addLogRow(NecroLog::Level level, const std::string &msg, const QVariant &user_data)
{
	LogTableModelRow row(Cols::Count);
	row[Cols::Level] = (int)level;
	row[Cols::Message] = QString::fromStdString(msg);
	row[Cols::TimeStamp] = QDateTime::currentDateTime();
	row[Cols::UserData] = user_data;
	Super::addLogRow(row);
}
