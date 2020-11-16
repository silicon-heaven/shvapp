#include "diskcleaner.h"

#include "application.h"
#include "appclioptions.h"

#include <bits/std_mutex.h>
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
    for (const QFileInfo &info : entries) {
        if (info.isDir()) {
            QDir child_dir(info.absoluteFilePath());
            scanDir(child_dir, ctx);
        }
        else if (info.isFile()) {
			if (info.fileName().length() == 27 && info.suffix() == "chp") {
				QString filename = info.fileName();
				try {
					int64_t ts = shv::core::utils::ShvFileJournal::JournalContext::fileNameToFileMsec(info.fileName().toStdString());
					if (ts < ctx.oldest_file_ts) {
						ctx.oldest_file_ts = ts;
						ctx.oldest_file_info = info;
					}
				}
				catch(...) {}
			}
            ctx.total_size += info.size();
        }
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
		CheckDiskContext ctx;
		QDir cache_dir(QString::fromStdString(Application::instance()->cliOptions()->logCacheDir()));
		scanDir(cache_dir, ctx);
		if (ctx.total_size < m_cacheSizeLimit) {
			break;
		}
//		shvInfo() << "removing " << ctx.oldest_file_info.absoluteFilePath().toStdString() << "due to big cache size";

		QDir dir(ctx.oldest_file_info.absolutePath(), "*.chp", QDir::SortFlag::Name, QDir::Filter::Files);
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
		QFile(ctx.oldest_file_info.absoluteFilePath()).remove();
	}
}
