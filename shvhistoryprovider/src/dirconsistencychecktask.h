#pragma once

#include <QThread>
#include "datetimeinterval.h"
#include "checktype.h"

class DirConsistencyCheckTask : public QThread
{
	Q_OBJECT
	using Super = QThread;

public:
	DirConsistencyCheckTask(const QString &site_path, CheckType check_type, QObject *parent);

	Q_SIGNAL void checkCompleted(QVector<DateTimeInterval>);
	Q_SIGNAL void error(QString);

protected:
	void run() override;

private:
	QString m_sitePath;
	CheckType m_checkType;
};
