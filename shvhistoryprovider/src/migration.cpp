#include "application.h"
#include "appclioptions.h"
#include "devicemonitor.h"
#include "logdir.h"
#include "migration.h"

#include <shv/coreqt/exception.h>
#include <shv/iotqt/rpc/rpc.h>
#include <shv/coreqt/log.h>

#include <QDir>

Migration::Migration()
	: m_root(QString::fromStdString(Application::instance()->cliOptions()->logCacheDir()))
{
}

bool Migration::isNeeded() const
{
	return m_root.exists("maintenance.sqlite");
}

void Migration::exec()
{
	shvInfo() << "migration started";
	migrateDir(m_root.path());
	m_root.remove("maintenance.sqlite");
	shvInfo() << "migration finished";
}

void Migration::migrateDir(const QString &path)
{
	QDir dir(path);
	QString shv_path = path.mid((int)Application::instance()->cliOptions()->logCacheDir().length());
	if (shv_path.startsWith('/')) {
		shv_path.remove(0, 1);
	}
	DeviceMonitor *monitor = Application::instance()->deviceMonitor();
	QStringList monitored_devices = monitor->monitoredDevices();
	bool is_monitored = monitored_devices.contains(shv_path);
	if (monitor->isElesysDevice(shv_path)) {
		shvInfo() << "removing elesys dir" << dir.absolutePath().mid(m_root.absolutePath().length());
		dir.removeRecursively();
		return;
	}

	for (const QFileInfo &entry : dir.entryInfoList(QDir::AllEntries | QDir::Filter::NoDotAndDotDot)) {
		if (entry.fileName().contains("dirty")) {
			shvInfo() << "removing dirty log" << entry.absoluteFilePath().mid(m_root.absolutePath().length());
			dir.remove(entry.fileName());
		}
		else if (entry.isDir()) {
			migrateDir(entry.absoluteFilePath());
		}
		else {
			if (is_monitored) {
				migrateFile(entry);
			}
			else if (dir.absolutePath() != m_root.absolutePath()) {
				shvInfo() << "removing dir" << dir.absolutePath().mid(m_root.absolutePath().length()) << "because it is no longer monitored";
				dir.removeRecursively();
				return;
			}
		}
	}
	if (dir.entryList(QDir::AllEntries | QDir::Filter::NoDotAndDotDot).count() == 0) {
		shvInfo() << "removing empty dir" << dir.absolutePath().mid(m_root.absolutePath().length());
		dir.removeRecursively();
	}
}

void Migration::migrateFile(const QFileInfo &fileinfo)
{
	if (fileinfo.suffix() == "chp" || fileinfo.suffix() == "log2") {
		return;
	}
	QFile file(fileinfo.absoluteFilePath());
	QDate file_date = QDate::fromString(fileinfo.fileName().mid(0, 8), "yyyyMMdd");
	if (!file_date.isValid()) {
		file.remove();
		return;
	}

	if (!file.open(QFile::ReadOnly)) {
		SHV_QT_EXCEPTION("Cannot open file " + file.fileName());
	}
	std::string err;
	shv::chainpack::RpcValue val = shv::chainpack::RpcValue::fromChainPack(file.readAll().toStdString(), &err);
	if (!err.empty()) {
		SHV_QT_EXCEPTION("Error parsing file " + file.fileName() + ": " + QString::fromStdString(err));
	}
	file.close();
	shv::chainpack::RpcValue::Map metadata = val.metaData().sValues();
	if (metadata.hasKey("compression")) {
		if (metadata["compression"] == "qCompress") {
			if (!val.isString()) {
				SHV_EXCEPTION("Compressed value must be string");
			}
			QByteArray uncompressed = qUncompress(QByteArray::fromStdString(val.toString()));
			val = shv::chainpack::RpcValue::fromChainPack(uncompressed.toStdString(), &err);
			if (!err.empty()) {
				SHV_QT_EXCEPTION("Error parsing uncompressed content of " + file.fileName());
			}
			metadata.erase("compression");
		}
		else {
			SHV_EXCEPTION("unknown compression " + metadata["compression"].toString());
		}
	}
	QDateTime since(file_date, QTime(0, 0, 0, 0), Qt::TimeSpec::UTC);
	QDateTime until(file_date, QTime(23, 59, 59, 999), Qt::TimeSpec::UTC);
	metadata["since"] = shv::chainpack::RpcValue::fromValue<QDateTime>(since);
	metadata["until"] = shv::chainpack::RpcValue::fromValue<QDateTime>(until);
	val.setMetaData(std::move(metadata));

	QFile new_file(fileinfo.absolutePath() + QDir::separator() + LogDir::fileName(since));
	if (!new_file.open(QFile::Truncate | QFile::WriteOnly)) {
		SHV_QT_EXCEPTION("Cannot open file " + new_file.fileName() + " for write");
	}
	new_file.write(QByteArray::fromStdString(val.toChainPack()));
	new_file.close();
	file.remove();
}
