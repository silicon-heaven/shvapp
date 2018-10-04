#include "lublicator.h"

#include <shv/iotqt/node//shvnodetree.h>

#include <shv/chainpack/rpcmessage.h>
#include <shv/chainpack/metamethod.h>
#include <shv/core/string.h>
#include <shv/core/stringview.h>
#include <shv/core/exception.h>

#include <shv/coreqt/log.h>
#include <shv/coreqt/exception.h>

#include <QDir>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QStandardPaths>
#include <QVariant>

namespace cp = shv::chainpack;
namespace iot = shv::iotqt;

const char *Lublicator::PROP_STATUS = "status";
const char *Lublicator::METH_DEVICE_ID = "deviceId";
const char *Lublicator::METH_CMD_ON = "cmdOn";
const char *Lublicator::METH_CMD_OFF = "cmdOff";
const char *Lublicator::METH_GET_LOG = "getLog";
const char METH_BATTERY_VOLTAGE[] = "batteryVoltage";
const char METH_SIM_SET_BATTERY_VOLTAGE[] = "sim_setBatteryVoltage";
const char METH_BATTERY_LOW_TRESHOLD[] = "batteryLowTreshold";
const char METH_SET_BATTERY_LOW_TRESHOLD[] = "setBatteryLowTreshold";
const char METH_BATTERY_HIGH_TRESHOLD[] = "batteryHighTreshold";
const char METH_SET_BATTERY_HIGH_TRESHOLD[] = "setBatteryHighTreshold";

Lublicator::Lublicator(const std::string &node_id, ShvNode *parent)
	: Super(parent)
{
	setNodeId(node_id);
	QString sql_dir = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
	sql_dir += "/log/revitest";
	QDir().mkpath(sql_dir);
	QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", sqlConnectionName());
	QString db_name = sqlConnectionName() + ".log";
	db.setDatabaseName(sql_dir + '/' + db_name);
	if(!db.open()) {
		shvError() << "Cannot open log SQLITE database file:" << db.databaseName();
	}
	else {
		shvInfo() << "Opening log SQLITE database file:" << db.databaseName() << "... OK";
	}
	QSqlQuery q = logQuery();
	if(!q.exec("SELECT * from shvlog LIMIT 1")) {
		shvInfo() << "DATABASE init - creating tables";
		q.exec("DROP TABLE shvlog");
		bool ok = q.exec("CREATE TABLE shvlog (id INTEGER PRIMARY KEY autoincrement, ts DATETIME, path TEXT, val TEXT)");
		if(!ok)
			shvError() << db_name << "Cannot create database shvlog";
	}

	connect(this, &Lublicator::valueChanged, this, &Lublicator::addLogEntry);

	addLogEntry(PROP_STATUS, status());
}

unsigned Lublicator::status() const
{
	return m_status;
}

bool Lublicator::setStatus(unsigned stat)
{
	unsigned old_stat = status();
	if(old_stat != stat) {
		m_status = stat;
		emit valueChanged(PROP_STATUS, stat);
		emit propertyValueChanged(nodeId() + '/' + PROP_STATUS, stat);
		return true;
	}
	return false;
}

static std::vector<cp::MetaMethod> meta_methods_device {
	{cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, false},
	{cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, false},
	{Lublicator::METH_DEVICE_ID, cp::MetaMethod::Signature::RetVoid, false},
	{Lublicator::METH_CMD_ON, cp::MetaMethod::Signature::VoidVoid, false},
	{Lublicator::METH_CMD_OFF, cp::MetaMethod::Signature::VoidVoid, false},
	{Lublicator::METH_GET_LOG, cp::MetaMethod::Signature::RetParam, false},
	{METH_BATTERY_LOW_TRESHOLD, cp::MetaMethod::Signature::RetVoid, false},
	{METH_SET_BATTERY_LOW_TRESHOLD, cp::MetaMethod::Signature::VoidParam, false},
	{METH_BATTERY_HIGH_TRESHOLD, cp::MetaMethod::Signature::RetVoid, false},
	{METH_SET_BATTERY_HIGH_TRESHOLD, cp::MetaMethod::Signature::VoidParam, false},
	{METH_BATTERY_VOLTAGE, cp::MetaMethod::Signature::RetVoid, false},
	{METH_SIM_SET_BATTERY_VOLTAGE, cp::MetaMethod::Signature::VoidParam, false},
};

static std::vector<cp::MetaMethod> meta_methods_status {
	{cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, false},
	{cp::Rpc::METH_GET, cp::MetaMethod::Signature::RetVoid, false},
};

size_t Lublicator::methodCount(const StringViewList &shv_path)
{
	if(shv_path.empty())
		return meta_methods_device.size();
	if(shv_path.size() == 1)
		return meta_methods_status.size();
	return 0;
}

const shv::chainpack::MetaMethod *Lublicator::metaMethod(const StringViewList &shv_path, size_t ix)
{
	if(shv_path.empty())
		return &(meta_methods_device.at(ix));
	if(shv_path.size() == 1)
		return &(meta_methods_status.at(ix));
	return nullptr;
}

shv::chainpack::RpcValue Lublicator::hasChildren(const StringViewList &shv_path)
{
	return shv_path.empty();
}

shv::iotqt::node::ShvNode::StringList Lublicator::childNames(const StringViewList &shv_path)
{
	//shvLogFuncFrame() << shvPath() << "for:" << shv_path;
	if(shv_path.empty())
		return shv::iotqt::node::ShvNode::StringList{PROP_STATUS};
	return shv::iotqt::node::ShvNode::StringList{};
}

shv::chainpack::RpcValue Lublicator::callMethod(const StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params)
{
	if(shv_path.empty()) {
		if(method == cp::Rpc::METH_GET) {
			return status();
		}
		if(method == METH_DEVICE_ID) {
			//shvError() << "device id:" << nodeId();
			return nodeId();
		}
		if(method == METH_CMD_ON) {
			unsigned stat = status();
			stat &= ~(unsigned)Status::PosInternalOff;
			stat &= ~(unsigned)Status::PosOff;
			stat |= (unsigned)Status::PosMiddle;
			setStatus(stat);
			stat &= ~(unsigned)Status::PosMiddle;
			stat |= (unsigned)Status::PosInternalOn;
			stat |= (unsigned)Status::PosOn;
			setStatus(stat);
			return true;
		}
		if(method == METH_CMD_OFF) {
			unsigned stat = status();
			stat &= ~(unsigned)Status::PosInternalOn;
			stat &= ~(unsigned)Status::PosOn;
			stat |= (unsigned)Status::PosMiddle;
			setStatus(stat);
			stat &= ~(unsigned)Status::PosMiddle;
			stat &= ~(unsigned)Status::PosInternalOff;
			stat |= (unsigned)Status::PosOff;
			setStatus(stat);
			return true;
		}
		if(method == METH_GET_LOG) {
			const shv::chainpack::RpcValue::Map &m = params.toMap();
			cp::RpcValue::DateTime from = m.value("from").toDateTime();
			cp::RpcValue::DateTime to = m.value("to").toDateTime();
			return getLog(from, to);
		}
		if(method == METH_BATTERY_VOLTAGE) {
			return m_batteryVoltage;
		}
		if(method == METH_SIM_SET_BATTERY_VOLTAGE) {
			unsigned d = params.toUInt();
			m_batteryVoltage = d;
			checkBatteryTresholds();
			return true;
		}
		if(method == METH_BATTERY_LOW_TRESHOLD) {
			return m_batteryLowTreshold;
		}
		if(method == METH_SET_BATTERY_LOW_TRESHOLD) {
			unsigned d = params.toUInt();
			m_batteryLowTreshold = d;
			checkBatteryTresholds();
			return true;
		}
		if(method == METH_BATTERY_HIGH_TRESHOLD) {
			return m_batteryHighTreshold;
		}
		if(method == METH_SET_BATTERY_HIGH_TRESHOLD) {
			unsigned d = params.toUInt();
			m_batteryHighTreshold = d;
			checkBatteryTresholds();
			return true;
		}
	}
	else if(shv_path[0] == "status") {
		if(method == cp::Rpc::METH_GET) {
			return status();
		}
	}
	return Super::callMethod(shv_path, method, params);
}

shv::chainpack::RpcValue Lublicator::getLog(const shv::chainpack::RpcValue::DateTime &from, const shv::chainpack::RpcValue::DateTime &to)
{
	shv::chainpack::RpcValue::List lines;
	QString qs = "SELECT ts, path, val FROM shvlog";
	QString where;
	auto format_sql = [](const cp::RpcValue::DateTime &dt) -> QString  {
		using DT = cp::RpcValue::DateTime;
		return QString::fromStdString(dt.toIsoString(DT::MsecPolicy::Always, DT::IncludeTimeZone));
	};
	if(from.isValid()) {
		where += "ts >= '" + format_sql(from) + "'";
	}
	if(to.isValid()) {
		if(!where.isEmpty())
			where += " AND ";
		where += "ts <= '" + format_sql(to) + "'";
	}
	if(!where.isEmpty())
		qs += " WHERE " + where;
	QSqlQuery q = logQuery();
	if(!q.exec(qs))
		SHV_EXCEPTION(("SQL error: " + qs + q.lastError().driverText() + q.lastError().databaseText()).toStdString());

	while(q.next()) {
		shv::chainpack::RpcValue::List line;
		line.push_back(q.value(0).toDateTime().toString(Qt::ISODateWithMs).toStdString());
		line.push_back(q.value(1).toString().toStdString());
		line.push_back(shv::chainpack::RpcValue::fromCpon(q.value(2).toString().toStdString()));
		lines.push_back(line);
	}
	shv::chainpack::RpcValue ret = lines;
	cp::RpcValue::Map device;
	device["id"] = "DS:" + nodeId();
	cp::RpcValue::MetaData md;
	md.setValue("device", device); // required
	md.setValue("logVersion", 1); // required
	md.setValue("dateTime", cp::RpcValue::DateTime::now());
	md.setValue("tsFrom", from);
	md.setValue("tsTo", to);
	md.setValue("fields", cp::RpcValue::List{
					cp::RpcValue::Map{{"name", "timestamp"}},
					cp::RpcValue::Map{{"name", "path"}},
					cp::RpcValue::Map{{"name", "value"}},
				});
	md.setValue("types", cp::RpcValue::Map{
					{"Status", cp::RpcValue::Map{
						 {"type", "BitField"},
						 {"fields", cp::RpcValue::List{
							  cp::RpcValue::Map{{"name", "PosOff"}, {"value", 0}},
							  cp::RpcValue::Map{{"name", "PosOn"}, {"value", 1}},
							  cp::RpcValue::Map{{"name", "PosMiddle"}, {"value", 2}},
							  cp::RpcValue::Map{{"name", "PosError"}, {"value", 3}},
							  cp::RpcValue::Map{{"name", "BatteryLow"}, {"value", 4}},
							  cp::RpcValue::Map{{"name", "BatteryHigh"}, {"value", 5}},
							  cp::RpcValue::Map{{"name", "DoorOpenCabinet"}, {"value", 6}},
							  cp::RpcValue::Map{{"name", "DoorOpenMotor"}, {"value", 7}},
							  cp::RpcValue::Map{{"name", "ModeAuto"}, {"value", 8}},
							  cp::RpcValue::Map{{"name", "ModeRemote"}, {"value", 9}},
							  cp::RpcValue::Map{{"name", "ModeService"}, {"value", 10}},
							  cp::RpcValue::Map{{"name", "MainSwitch"}, {"value", 11}},
						  }
						 },
						 {"description", "PosOff = 0, PosOn = 1, PosMiddle = 2, PosError= 3, BatteryLow = 4, BatteryHigh = 5, DoorOpenCabinet = 6, DoorOpenMotor = 7, ModeAuto= 8, ModeRemote = 9, ModeService = 10, MainSwitch = 11, ErrorRtc = 12"},
					 }
					},
				});
	md.setValue("pathInfo", cp::RpcValue::Map{
					{"status", cp::RpcValue::Map{ {"type", "Status"} }
					},
				});
	ret.setMetaData(std::move(md));
	return ret;
}

void Lublicator::addLogEntry(const std::string &key, const shv::chainpack::RpcValue &value)
{
	QSqlQuery q = logQuery();
	if(q.prepare("INSERT INTO shvlog (ts, path, val) VALUES (:ts, :path, :val)")) {
		q.bindValue(":ts", QVariant::fromValue(QDateTime::currentDateTimeUtc()));
		q.bindValue(":path", QVariant::fromValue(QString::fromStdString(key)));
		q.bindValue(":val", QVariant::fromValue(QString::fromStdString(value.toCpon())));
		if(!q.exec()) {
			shvError() << "Error exec append log";
		}
	}
	else {
		shvError() << "Error prepare append log query" << q.lastError().driverText() << q.lastError().databaseText();
	}
}

QSqlQuery Lublicator::logQuery()
{
	QSqlDatabase db = QSqlDatabase::database(sqlConnectionName());
	return QSqlQuery(db);
}

QString Lublicator::sqlConnectionName()
{
	return QStringLiteral("lublicator-%1").arg(QString::fromStdString(nodeId()));
}

void Lublicator::checkBatteryTresholds()
{
	unsigned s = status();
	if(m_batteryVoltage >= m_batteryHighTreshold)
		s |= (unsigned)Status::BatteryHigh;
	else
		s &= ~((unsigned)Status::BatteryHigh);
	if(m_batteryVoltage <= m_batteryLowTreshold)
		s |= (unsigned)Status::BatteryLow;
	else
		s &= ~((unsigned)Status::BatteryLow);
	setStatus(s);
}

