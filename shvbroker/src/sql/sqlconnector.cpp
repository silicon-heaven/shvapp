#include "sqlconnector.h"
#include "../brokerapp.h"

#include <shv/coreqt/log.h>

#include <QSqlDatabase>
#include <QSqlError>

//namespace ecu = eyascore::utils;

namespace sql {

SqlConnector::SqlConnector(QObject *parent)
	: QObject(parent)
{

}

SqlConnector::~SqlConnector()
{
	close();
}

void SqlConnector::open(const QString &host, int port, const QString &password)
{
	shvLogFuncFrame() << host << port;
	close();
	QSqlDatabase db = QSqlDatabase::addDatabase("QPSQL");
	db.setHostName(host);
	db.setPort(port);
	db.setUserName("shvbroker");
	db.setDatabaseName("shv");
	db.setPassword(password);
	shvInfo().nospace() << "connecting to SQL server " << host << ':' << port << " ...";
	bool db_ok = false;
	do {
		bool open_ok = db.open();
		if(!open_ok) {
			QString msg = db.lastError().text();
			shvError() << "ERROR connecting to SQL server:" << msg;
			emit sqlServerError(msg);
			break;
		}
		shvInfo() << "connected to SQL server - OK";
		QString db_profile = BrokerApp::instance()->cliOptions()->profile();
		db_ok = checkDbSchema(db_profile);
		if(!db_ok)
			break;
		shvInfo() << "Server profile set to:" << db_profile;
		db_ok = checkDbTables();
		if(!db_ok)
			break;
		//db_ok = checkDbFs();
		//if(!db_ok)
		//	break;
	} while(false);
	if(!db_ok) {
		close();
	}
}

bool SqlConnector::isOpen() const
{
	return sqlConnection().isOpen();
}

QSqlDatabase SqlConnector::sqlConnection() const
{
	QSqlDatabase conn;// = qf::core::sql::Connection::forName();
	return conn;
}
#if 0
static QString variant_to_string(const QVariant &val)
{
	QVariant v = val;// = eyascore::rpc::Protocol::encodeVariant(val);
	/*
	if(v.type() == QVariant::List) {
		QJsonArray jsa = QJsonValue::fromVariant(v.toList()).toArray();
		QJsonDocument jsd(jsa);
		QByteArray ba = jsd.toJson(QJsonDocument::Compact);
		return QString::fromUtf8(ba);
	}
	if(v.type() == QVariant::Map) {
		QJsonObject jso = QJsonValue::fromVariant(v.toMap()).toObject();
		QJsonDocument jsd(jso);
		QByteArray ba = jsd.toJson(QJsonDocument::Compact);
		return QString::fromUtf8(ba);
	}
	*/
	return v.toString();
}
#endif
void SqlConnector::close()
{
	QSqlDatabase::removeDatabase(QSqlDatabase::defaultConnection);
}

bool SqlConnector::checkDbSchema(const QString &db_profile)
{
	return false;
	/*
	QSqlDatabase db = sqlConnection();
	bool set_profile_ok = db.setCurrentSchema(db_profile);
	if(!set_profile_ok) {
		set_profile_ok = db.createSchema(db_profile);
		if(set_profile_ok) {
			set_profile_ok = db.setCurrentSchema(db_profile);
		}
	}
	if(!set_profile_ok) {
		QString msg = "set schema to '%1' failed!";
		msg = msg.arg(db_profile);
		qfError() << "SQL Error: " + msg;
		emit sqlServerError(msg);
	}
	return set_profile_ok;
	*/
}

bool SqlConnector::checkDbTables()
{
#if 0
	qf::core::sql::Connection conn = sqlConnection();
	qf::core::sql::Query q(conn);
	try {
		qf::core::sql::Transaction transaction(conn);
		{
			QString table_name = "config";
			if(!conn.tableExists(table_name)) {
				shvInfo() << "Creating table:" << table_name;
				QString qs = "CREATE TABLE " QF_CARG(table_name)
							 "("
							 "  ckey character varying NOT NULL,"
							 "  cname character varying,"
							 "  cvalue character varying,"
							 "  ctype character varying,"
							 "  CONSTRAINT " QF_CARG(table_name) "_pkey PRIMARY KEY (ckey)"
							 ") WITH (OIDS=FALSE)";
				q.exec(qs, qf::core::Exception::Throw);
				qs = "INSERT INTO " QF_CARG(table_name) " (ckey, cname, cvalue, ctype) VALUES ('db.version', 'Data version', %1, 'int')";
				qs = qs.arg(qf::core::Utils::versionStringToInt(TheApp::instance()->versionString()));
				q.exec(qs, qf::core::Exception::Throw);
				qs = "INSERT INTO " QF_CARG(table_name) " (ckey, cname, cvalue, ctype) VALUES ('depot.operationalMode', 'Depot operational mode', 0, 'int')";
				q.exec(qs, qf::core::Exception::Throw);
			}
		}
		{
			QString table_name = "enumz";
			if(!conn.tableExists(table_name)) {
				shvInfo() << "Creating table:" << table_name;
				QString qs = "CREATE TABLE " QF_CARG(table_name)
							 "("
							 "  id serial NOT NULL,"
							 "  groupName varchar,"
							 "  groupId varchar,"
							 "  pos integer,"
							 "  abbreviation varchar,"
							 "  caption varchar,"
							 "  value varchar,"
							 "  comment varchar,"
							 "  CONSTRAINT " QF_CARG(table_name) "_pkey PRIMARY KEY (id),"
							 "  CONSTRAINT " QF_CARG(table_name) "_group_key UNIQUE (groupName, groupId)"
							 ") WITH (OIDS=FALSE)";
				q.exec(qs, qf::core::Exception::Throw);
			}
		}
		{
			QString table_name = "opcuaconnections";
			if(!conn.tableExists(table_name)) {
				shvInfo() << "Creating table:" << table_name;
				QString qs = "CREATE TABLE " QF_CARG(table_name)
						"("
						"  id serial NOT NULL,"
						"  serverGroup character varying,"
						"  serverType character varying DEFAULT 'opcua'," // tsc | opcua
						"  name character varying,"
						"  host character varying,"
						"  port integer,"
						"  userName character varying,"
						"  password character varying,"
						"  enabled boolean NOT NULL DEFAULT FALSE,"
						"  vncUrl character varying,"
						"  clientConfig json,"
						"  CONSTRAINT " QF_CARG(table_name) "_pkey PRIMARY KEY (id),"
						"  CONSTRAINT " QF_CARG(table_name) "_name_key UNIQUE (serverGroup, name, enabled)"
						") WITH (OIDS=FALSE)";
				q.exec(qs, qf::core::Exception::Throw);
				q.exec("COMMENT ON COLUMN " + table_name + ".clientConfig IS 'tsc config { \"sil3\": false|true, \"type\": \"L\"|\"R\"|\"F\"|\"RL\"|\"LR\" }"
					   "L: Single Left, R: Single Right, F: Fork, RL: Tandem RightLeft, LR: Tandem LeftRight'", qf::core::Exception::Throw);
			}
		}
		{
			QString table_name = "opcuavariables";
			if(!conn.tableExists(table_name)) {
				shvInfo() << "Creating table:" << table_name;
				QString qs = "CREATE TABLE " QF_CARG(table_name)
						"("
						"  id serial NOT NULL,"
						"  serverName character varying,"
						"  nodeId character varying,"
						"  eyasId character varying,"
						"  opcuaType character varying,"
						"  enabled boolean NOT NULL DEFAULT TRUE,"
						"  description character varying,"
						"  CONSTRAINT " QF_CARG(table_name) "_pkey PRIMARY KEY (id),"
						"  CONSTRAINT " QF_CARG(table_name) "_name_node_key UNIQUE (serverName, nodeId)"
						") WITH (OIDS=FALSE)";
				q.exec(qs, qf::core::Exception::Throw);
			}
		}
		{
			QString table_name = "opcuatypes";
			if(!conn.tableExists(table_name)) {
				shvInfo() << "Creating table:" << table_name;
				QString qs = "CREATE TABLE " QF_CARG(table_name)
						"("
						"  id serial NOT NULL,"
						"  opcuaType character varying,"
						"  opcuaProperty character varying,"
						"  eyasProperty character varying,"
						"  read boolean NOT NULL DEFAULT FALSE,"
						"  write boolean NOT NULL DEFAULT FALSE,"
						"  subscribe boolean NOT NULL DEFAULT FALSE,"
						"  valueType character varying,"
						"  CONSTRAINT " QF_CARG(table_name) "_pkey PRIMARY KEY (id),"
						"  CONSTRAINT " QF_CARG(table_name) "_type_prop_key UNIQUE (opcuaType, opcuaProperty)"
						") WITH (OIDS=FALSE)";
				q.exec(qs, qf::core::Exception::Throw);
			}
		}
		{
			QString table_name = "depotmodeljournal";
			if(!conn.tableExists(table_name)) {
				shvInfo() << "Creating table:" << table_name;
				QString qs = "CREATE TABLE " QF_CARG(table_name)
						"("
						"  id serial NOT NULL PRIMARY KEY,"
						"  path character varying,"
						"  value character varying,"
						"  appSrvVer integer,"
						"  appSrvTs timestamp with time zone NOT NULL DEFAULT now()" // allways use timestamp with timezone (timestamp is stored in UTC internaly)
						") WITH (OIDS=FALSE)";
				q.exec(qs, qf::core::Exception::Throw);
				//qs = "CREATE INDEX " QF_CARG(table_name) "_eyasid_prop_key ON " QF_CARG(table_name) " (eyasId, eyasProperty)";
				//q.exec(qs, qf::core::Exception::Throw);
				qs = "CREATE INDEX " QF_CARG(table_name) "_appSrvTs ON " QF_CARG(table_name) " (appSrvTs)";
				q.exec(qs, qf::core::Exception::Throw);
			}
		}
		{
			QString table_name = "tramclasses";
			if(!conn.tableExists(table_name)) {
				shvInfo() << "Creating table:" << table_name;
				QString qs = "CREATE TABLE " QF_CARG(table_name)
						"("
						"  id serial NOT NULL PRIMARY KEY,"
						"  name character varying,"
						"  classId character varying,"
						"  description character varying,"
						"  coachSizes jsonb NOT NULL DEFAULT '[10000, 8000, 10000]'::jsonb,"
						"  coachWidth integer NOT NULL DEFAULT 3000,"
						"  distinctCoaches boolean NOT NULL DEFAULT false,"
						"  CONSTRAINT " QF_CARG(table_name) "_name_key UNIQUE (name)"
						") WITH (OIDS=FALSE)";
				q.exec(qs, qf::core::Exception::Throw);
				qs = "COMMENT ON COLUMN " QF_CARG(table_name) ".coachSizes IS 'JSON array of coach sizes in milimeters, like [8000, 7000, 7000, 8000]'";
				q.exec(qs, qf::core::Exception::Throw);
				qs = "COMMENT ON COLUMN " QF_CARG(table_name) ".coachWidth IS 'coach width in milimeters'";
				q.exec(qs, qf::core::Exception::Throw);
			}
		}
		{
			QString table_name = "trams";
			if(!conn.tableExists(table_name)) {
				shvInfo() << "Creating table:" << table_name;
				QString qs = "CREATE TABLE " QF_CARG(table_name)
						"("
						"  id serial NOT NULL PRIMARY KEY,"
						"  tramNumber integer,"
						"  classId integer REFERENCES tramclasses (id) ON UPDATE RESTRICT ON DELETE RESTRICT,"
						//"  depotId integer,"
						"  depotPosition character varying,"
						"  description character varying,"
						"  attributes character varying,"
						"  CONSTRAINT " QF_CARG(table_name) "_tramNumber_key UNIQUE (tramNumber)"
						") WITH (OIDS=FALSE)";
				q.exec(qs, qf::core::Exception::Throw);
				//qs = "CREATE INDEX " QF_CARG(table_name) "_depotId_key ON " QF_CARG(table_name) " (depotId)";
				//q.exec(qs, qf::core::Exception::Throw);
			}
		}
		{
			QString table_name = "itineraries";
			if(!conn.tableExists(table_name)) {
				shvInfo() << "Creating table:" << table_name;
				QString qs = "CREATE TABLE " QF_CARG(table_name)
						"("
						"  id serial NOT NULL PRIMARY KEY,"
						"  tramId integer,"
						"  itinerary character varying,"
						"  createdTs timestamp with time zone,"
						"  finishedTs timestamp with time zone,"
						"  message character varying,"
						"  userName character varying,"
						"  dependsOnItineraryId integer,"
						"  runId integer"
						") WITH (OIDS=FALSE)";
				q.exec(qs, qf::core::Exception::Throw);
				qs = "CREATE INDEX " QF_CARG(table_name) "_finishedTs_key ON " QF_CARG(table_name) " (finishedTs)";
				q.exec(qs, qf::core::Exception::Throw);
				qs = "COMMENT ON COLUMN " QF_CARG(table_name) ".itinerary IS 'JSON array of Routes, like [[23, 1], [22, 0], [11, 3], ...]'";
				q.exec(qs, qf::core::Exception::Throw);
				//qs = "COMMENT ON COLUMN " QF_CARG(table_name) ".itineraryDisp in human readable format, like tram#1234:B12/16+C16/23+A23/10'";
				//q.exec(qs, qf::core::Exception::Throw);
			}
		}
		{
			QString table_name = "events";
			if(!conn.tableExists(table_name)) {
				shvInfo() << "Creating table:" << table_name;
				QString qs = "CREATE TYPE enum_severity AS ENUM ('debug', 'info', 'warning', 'error', 'fatal')";
				q.exec(qs, qf::core::Exception::Throw);

				qs = "CREATE TABLE " QF_CARG(table_name)
						"("
						"  id serial NOT NULL PRIMARY KEY,"
						"  userName character varying,"
						"  severity enum_severity,"
						"  category character varying,"
						"  message character varying,"
						"  localizedMessage character varying,"
						"  alertKind character varying,"
						"  path character varying,"
						"  createdTs timestamp with time zone NOT NULL DEFAULT now(),"
						"  data character varying"
						") WITH (OIDS=FALSE)";
				q.exec(qs, qf::core::Exception::Throw);
				qs = "CREATE INDEX " QF_CARG(table_name) "_createdTs_key ON " QF_CARG(table_name) " (createdTs)";
				q.exec(qs, qf::core::Exception::Throw);
				qs = "CREATE INDEX " QF_CARG(table_name) "_userName_key ON " QF_CARG(table_name) " (userName)";
				q.exec(qs, qf::core::Exception::Throw);
			}
		}
		{
			QString table_name = "runs";
			if(!conn.tableExists(table_name)) {
				shvInfo() << "Creating table:" << table_name;
				QString qs = "CREATE TABLE " QF_CARG(table_name)
						"("
						"  id serial NOT NULL PRIMARY KEY,"
						"  runNumber varchar,"
						"  runTable varchar," // HASTUS duty table number, see <ddmmyyyy>_ E_Despatch_Trips.csv
						"  runDate date,"
						"  tramNumber integer,"
						"  tramNumberLock boolean NOT NULL DEFAULT false,"
						"  tramClass varchar,"
						"  runOutTrack integer,"
						"  runOutTime timestamp with time zone,"
						"  runInTrack integer,"
						"  runInTime timestamp with time zone,"
						"  eyasRunInTime timestamp with time zone,"
						"  destination varchar,"
						"  runLength integer,"
						"  runDuration integer,"
						"  changeCode varchar,"
						"  scheduledRunOutTime integer,"
						"  manuallyAdjustedRunOutTime integer,"
						"  eyasRunOutTime timestamp with time zone,"
						"  avmRunOutTime timestamp with time zone,"
						"  hastusScheduledRunInTime integer,"
						"  avmDeviation int,"
						"  manuallyAdjustedRunInTime integer,"
						"  importId integer,"
						"  overlapedImportId integer,"
						"  outItineraryId integer,"
						"  inItineraryId integer,"
						"  runOutDisabled boolean NOT NULL DEFAULT false,"
						"  runInDisabled boolean NOT NULL DEFAULT false,"
						"  comment varchar,"
						"  tagId integer,"
						"  importTS timestamp with time zone"
						") WITH (OIDS=FALSE)";
				q.exec(qs, qf::core::Exception::Throw);
				qs = "CREATE INDEX " QF_CARG(table_name) "_runDate_key ON " QF_CARG(table_name) " (runDate)";
				q.exec(qs, qf::core::Exception::Throw);
				qs = "CREATE INDEX " QF_CARG(table_name) "_runOutTime_key ON " QF_CARG(table_name) " (runOutTime)";
				q.exec(qs, qf::core::Exception::Throw);
				qs = "CREATE INDEX " QF_CARG(table_name) "_runInTime_key ON " QF_CARG(table_name) " (runInTime)";
				q.exec(qs, qf::core::Exception::Throw);
				qs = "CREATE INDEX " QF_CARG(table_name) "_outItineraryId_key ON " QF_CARG(table_name) " (outItineraryId)";
				q.exec(qs, qf::core::Exception::Throw);
				qs = "CREATE INDEX " QF_CARG(table_name) "_inItineraryId_key ON " QF_CARG(table_name) " (inItineraryId)";
				q.exec(qs, qf::core::Exception::Throw);
			}
		}
		{
			QString table_name = "tags";
			if(!conn.tableExists(table_name)) {
				shvInfo() << "Creating table:" << table_name;
				QString qs = "CREATE TABLE " QF_CARG(table_name)
						"("
						"  id serial NOT NULL PRIMARY KEY,"
						"  category character varying,"
						"  position integer,"
						"  name character varying,"
						"  color character varying"
						") WITH (OIDS=FALSE)";
				q.exec(qs, qf::core::Exception::Throw);
				qs = "CREATE INDEX " QF_CARG(table_name) "_category ON " QF_CARG(table_name) " (category, position)";
				q.exec(qs, qf::core::Exception::Throw);
			}
		}
		{
			QString table_name = "depots";
			if(!conn.tableExists(table_name)) {
				shvInfo() << "Creating table:" << table_name;
				QString qs = "CREATE TABLE " QF_CARG(table_name)
						"("
						"  id serial NOT NULL PRIMARY KEY,"
						"  name character varying"
						") WITH (OIDS=FALSE)";
				q.exec(qs, qf::core::Exception::Throw);
				qs = "INSERT INTO " QF_CARG(table_name) "(name) VALUES ('Main Depot')";
				q.exec(qs, qf::core::Exception::Throw);
			}
		}
		{
			QString table_name = "users";
			if(!conn.tableExists(table_name)) {
				shvInfo() << "Creating table:" << table_name;
				QString qs = "CREATE TABLE " QF_CARG(table_name)
						"("
						"  name character varying NOT NULL PRIMARY KEY,"
						"  grants character varying,"
						"  fullName character varying,"
						"  password character varying"
						") WITH (OIDS=FALSE)";
				q.exec(qs, qf::core::Exception::Throw);
				qs = "INSERT INTO " + table_name + " (name, grants, fullName, password) VALUES (:name, :grants, :fullName, :password)";
				q.prepare(qs, qf::core::Exception::Throw);
				const char* users[][4] = {
					{"admin", "admin", "Eyas administrator", "9S5D6r3E5A4x1N0f3G"},
				};
				for(auto p : users) {
					q.bindValue(":name", p[0]);
					q.bindValue(":grants", p[1]);
					q.bindValue(":fullName", p[2]);
					q.bindValue(":password", p[3]);
					q.exec(qf::core::Exception::Throw);
				}
			}
		}
		{
			QString table_name = "usergrants";
			if(!conn.tableExists(table_name)) {
				shvInfo() << "Creating table:" << table_name;
				QString qs = "CREATE TABLE " QF_CARG(table_name)
						"("
						"  position integer,"
						"  name character varying NOT NULL PRIMARY KEY,"
						"  grants character varying,"
						"  caption character varying"
						") WITH (OIDS=FALSE)";
				q.exec(qs, qf::core::Exception::Throw);
				qs = "INSERT INTO " + table_name + " (position, name, grants, caption) VALUES (:position, :name, :grants, :caption)";
				q.prepare(qs, qf::core::Exception::Throw);
				const QString grants[][3] = {
					{ecu::UserGrant::STARTER, "", QT_TRANSLATE_NOOP("GrantCaption", "Starter")},
					{ecu::UserGrant::SUPER_STARTER, ecu::UserGrant::STARTER + "," + ecu::UserGrant::CHANGE_SWITCH, QT_TRANSLATE_NOOP("GrantCaption", "Senior starter")},
					{ecu::UserGrant::SERVICE, ecu::UserGrant::OPCUA + "," + ecu::UserGrant::PLC + "," + ecu::UserGrant::APP_SERVER + "," + ecu::UserGrant::SUPER_STARTER, "Elektroline service"},
					{ecu::UserGrant::ADMIN, "", ""},
					{ecu::UserGrant::TESTER, ecu::UserGrant::SERVICE, QT_TRANSLATE_NOOP("GrantCaption", "Elektroline tester")},
					{ecu::UserGrant::DEPOT_EDITOR, "", QT_TRANSLATE_NOOP("GrantCaption", "Invoke depot schema editor")},
					{ecu::UserGrant::TRAMS_CATALOG, "", QT_TRANSLATE_NOOP("GrantCaption", "Edit trams catalog")},
					{ecu::UserGrant::RUN_TABLE, "", QT_TRANSLATE_NOOP("GrantCaption", "Right to view run tables")},
					{ecu::UserGrant::OPCUA, "", ""},
					{ecu::UserGrant::PLC, "", ""},
					{ecu::UserGrant::USERS, "", QT_TRANSLATE_NOOP("GrantCaption", "Edit users")},
					{ecu::UserGrant::APP_SERVER, "", QT_TRANSLATE_NOOP("GrantCaption", "Manage application server")},
					{ecu::UserGrant::CHANGE_SWITCH, "", QT_TRANSLATE_NOOP("GrantCaption", "Manualy change switch position")},
					{ecu::UserGrant::DEVELOPER, ecu::UserGrant::SERVICE, QT_TRANSLATE_NOOP("GrantCaption", "EYAS developer")},
				};
				int n = 0;
				for(auto p : grants) {
					q.bindValue(":position", ++n);
					q.bindValue(":name", p[0]);
					q.bindValue(":grants", p[1]);
					q.bindValue(":caption", p[2]);
					q.exec(qf::core::Exception::Throw);
				}
			}
		}
		transaction.commit();
	}
	catch (const qf::core::Exception &e) {
		qfError() << "Check database error\n" << q.lastErrorText();
		return false;
	}
#endif
	return true;
}

}

