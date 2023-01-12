#include "application.h"
#include "appclioptions.h"
#include "logdir.h"

#include <QDateTime>
#include <QVector>

#include <shv/core/utils/shvfilejournal.h>
#include <shv/chainpack/chainpackreader.h>
#include <shv/coreqt/rpc.h>

#include <iostream>
#include <fstream>

LogDir::LogDir(const QString &site_path)
	: m_dir(dirPath(site_path), "*", QDir::SortFlag::Name, QDir::Filter::Files)
{
	if (!m_dir.exists()) {
		QDir(QString::fromStdString(Application::instance()->cliOptions()->logCacheDir())).mkpath(site_path);
		m_dir.refresh();
	}
}

QString LogDir::dirPath(const QString &site_path)
{
	return QString::fromStdString(Application::instance()->cliOptions()->logCacheDir()) + QDir::separator() + site_path;
}

QStringList LogDir::findFiles(const QDateTime &since, const QDateTime &until)
{
	struct File {
		QString filename;
		int64_t date;
	};
	QVector<File> files;
	QFileInfoList file_infos = m_dir.entryInfoList();
	for (int i = 0; i < file_infos.count(); ++i) {
		const QFileInfo &file_info = file_infos[i];
		QString filename = file_info.fileName();
		if (file_info.suffix() != "chp") {
			if (filename != dirtyLogName()) {
				Application::instance()->registerAlienFile(file_info.absoluteFilePath());
			}
		}
		else {
			try {
				int64_t datetime = shv::core::utils::ShvFileJournal::JournalContext::fileNameToFileMsec(filename.toStdString());
				files << File{ filename, datetime };
			}
			catch (...) {
				Application::instance()->registerAlienFile(file_info.absoluteFilePath());
			}
		}
	}

	if (files.count() == 0) {
		return QStringList();
	}

	int since_pos = 0;
	if (since.isValid()) {
		auto it = std::upper_bound(files.begin(), files.end(), File { QString(), since.toMSecsSinceEpoch() }, [](const File &f1, const File &f2) {
			return f1.date < f2.date;
		});
		if (it != files.begin()) {
			--it;
		}
		since_pos = static_cast<int>((it - files.begin()));
	}
	qsizetype until_pos = files.count();
	if (until.isValid()) {
		auto it = std::upper_bound(files.begin(), files.end(), File { QString(), until.toMSecsSinceEpoch() }, [](const File &f1, const File &f2) {
			return f1.date < f2.date;
		});
		until_pos = static_cast<int>((it - files.begin()));
	}
	if (until_pos - since_pos == 1 && until_pos == files.count()) { //we are on a last file, we must check until
		std::ifstream in_file;
		in_file.open(m_dir.absoluteFilePath(files[since_pos].filename).toStdString(), std::ios::in | std::ios::binary);
		shv::chainpack::ChainPackReader log_reader(in_file);
		shv::chainpack::RpcValue::MetaData meta_data;
		log_reader.read(meta_data);
		QDateTime file_until = rpcvalue_cast<QDateTime>(meta_data.value("until"));
		if (file_until < since) {
			return QStringList();
		}
	}
	QStringList result;
	for (int i = since_pos; i < until_pos; ++i) {
		result << m_dir.absoluteFilePath(files[i].filename);
	}
	return result;
}

QString LogDir::fileName(const QDateTime &datetime)
{
	return QString::fromStdString(shv::core::utils::ShvFileJournal::JournalContext::msecToBaseFileName(datetime.toMSecsSinceEpoch())) + ".chp";
}

QString LogDir::filePath(const QDateTime &datetime)
{
	return m_dir.absoluteFilePath(fileName(datetime));
}

QString LogDir::dirtyLogName() const
{
	return QStringLiteral("dirty.log2");
}

QString LogDir::dirtyLogPath() const
{
	return m_dir.absoluteFilePath(dirtyLogName());
}
