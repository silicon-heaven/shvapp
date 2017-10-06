#pragma once

#include <QObject>

class QSqlDatabase;

namespace sql {

class SqlConnector : public QObject
{
	Q_OBJECT
public:
	explicit SqlConnector(QObject *parent = 0);
	~SqlConnector() Q_DECL_OVERRIDE;

	void open(const QString &host, int port, const QString &password);
	bool isOpen() const;
	Q_SIGNAL void sqlServerError(const QString &msg);
	Q_SIGNAL void openChanged(bool is_open);

	QSqlDatabase sqlConnection() const;

	//Q_SLOT void saveDepotModelJournal(const QStringList &path, const QVariant &val);
	//int saveDepotEvent(const eyascore::utils::LoggedDepotEvent &depot_event);

	QByteArray dbFsGet(const QString &path, bool *pok = nullptr);
	bool dbFsPut(const QString &path, const QByteArray &data, bool create_if_not_exist = true);
private:
	void close();
	bool checkDbSchema(const QString &db_profile);
	bool checkDbTables();
	bool checkDbFs();
};

}

