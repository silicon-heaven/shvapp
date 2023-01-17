#include "diskcleaner.h"

#include "application.h"
#include "appclioptions.h"
#include "devicemonitor.h"

#include <shv/core/log.h>
#include <shv/core/utils/shvfilejournal.h>

namespace cp = shv::chainpack;

DiskCleaner::DiskCleaner(int64_t cache_size_limit, QObject *parent)
	: QObject(parent)
	, m_cleanTimer(this)
	, m_isChecking(false)
	, m_cacheSizeLimit(cache_size_limit)
{
	connect(&m_cleanTimer, &QTimer::timeout, this, &DiskCleaner::checkDiskOccupation);
	m_cleanTimer.start(15 * 60 * 1000);
}

void DiskCleaner::scanDir(const QDir &dir, DiskCleaner::CheckDiskContext &ctx)
{
	QFileInfoList entries = dir.entryInfoList(QDir::Filter::AllEntries | QDir::Filter::NoDotAndDotDot);
	int entry_count = static_cast<int>(entries.count());
    for (const QFileInfo &info : entries) {
        if (info.isDir()) {
            QDir child_dir(info.absoluteFilePath());
            scanDir(child_dir, ctx);
        }
        else if (info.isFile()) {
			int cache_dir_path_length = static_cast<int>(Application::instance()->cliOptions()->logCacheDir().size());
			QString site_path = dir.absolutePath().mid(cache_dir_path_length + 1);
			if (ctx.deviceOccupationInfo.contains(site_path)) {
				if (info.fileName().length() == 27 && info.suffix() == "chp") {
					DeviceOccupationInfo &occupation_info = ctx.deviceOccupationInfo[site_path];
					QString filename = info.fileName();
					try {
						int64_t ts = shv::core::utils::ShvFileJournal::JournalContext::fileNameToFileMsec(info.fileName().toStdString());
						if (ts < occupation_info.oldestFileTs) {
							occupation_info.oldestFileTs = ts;
							occupation_info.oldestFileInfo = info;
						}
						occupation_info.occupiedSize += info.size();
					}
					catch(...) {}
				}
				ctx.totalSize += info.size();
			}
			else {
//				shvInfo() << "removing file for not existing device" << site_path.toStdString() << info.fileName().toStdString();
//				QFile::remove(info.absoluteFilePath());
//				--entry_count;
			}
        }
    }
	if (entry_count == 0) {
		QDir copy_dir(dir);
		copy_dir.removeRecursively();
	}
	QCoreApplication::processEvents();
}

void DiskCleaner::checkDiskOccupation()
{
	if (m_isChecking) {
		return;
	}
	ScopeGuard sg(m_isChecking);

	while (true) {
		if (!Application::instance()->deviceConnection()->isBrokerConnected()) {
			return;
		}
		CheckDiskContext ctx;
		for (const QString &device_path : Application::instance()->deviceMonitor()->monitoredDevices()) {
			ctx.deviceOccupationInfo[device_path] = DeviceOccupationInfo();
		}
		QDir cache_dir(QString::fromStdString(Application::instance()->cliOptions()->logCacheDir()));
		scanDir(cache_dir, ctx);
		if (ctx.totalSize < m_cacheSizeLimit) {
			break;
		}

		int limit_per_device = static_cast<int>(m_cacheSizeLimit / ctx.deviceOccupationInfo.count());
		for (const auto &occupation_info : ctx.deviceOccupationInfo) {
			if (occupation_info.occupiedSize > limit_per_device) {
				QDir dir(occupation_info.oldestFileInfo.absolutePath(), "*.chp", QDir::SortFlag::Name, QDir::Filter::Files);
				QStringList file_names = dir.entryList();
				if (file_names.count() > 1) {
					QFile second_oldest_file(dir.absoluteFilePath(file_names[1]));
					if (second_oldest_file.open(QFile::ReadOnly)) {
						cp::RpcValue log = cp::RpcValue::fromChainPack(second_oldest_file.readAll().toStdString());
						log.setMetaValue("HP", cp::RpcValue::Map{{ "firstLog", true }});
						second_oldest_file.close();
						if (second_oldest_file.open(QFile::WriteOnly | QFile::Truncate)) {
							second_oldest_file.write(QByteArray::fromStdString(log.toChainPack()));
						}
					}
				}
				shvInfo() << "removing " << occupation_info.oldestFileInfo.absoluteFilePath().toStdString() << "due to big cache size";
				QFile(occupation_info.oldestFileInfo.absoluteFilePath()).remove();
			}
		}
	}
}
