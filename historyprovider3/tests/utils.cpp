#include "tests/utils.h"

#include "src/historyapp.h"
#include "src/appclioptions.h"

QDir get_site_cache_dir(const std::string& site_path)
{
	return QDir{QString::fromStdString(shv::core::utils::joinPath(HistoryApp::instance()->cliOptions()->journalCacheRoot(), site_path))};
}

void remove_cache_contents(const std::string& site_path)
{
	auto cache_dir = get_site_cache_dir(site_path);
	auto it = QDirIterator(cache_dir, QDirIterator::Subdirectories);
	while (it.hasNext()) {
		auto entry = it.next();
		if (it.fileInfo().isFile()) {
			QFile(entry).remove();
		}
	}
}

RpcValue::List get_cache_contents(const std::string& site_path)
{
	RpcValue::List res;
	auto cache_dir = get_site_cache_dir(site_path);
	cache_dir.setFilter(QDir::Files | QDir::NoDotAndDotDot);
	auto it = QDirIterator(cache_dir, QDirIterator::Subdirectories);
	while (it.hasNext()) {
		auto entry = it.next();
		RpcValue::List name_and_size;
		name_and_size.push_back(entry.mid(cache_dir.path().size() + 1).toStdString());
		QFile file(entry);
		name_and_size.push_back(RpcValue::UInt(QFile(cache_dir.filePath(entry)).size()));
		res.push_back(name_and_size);
	}

	std::sort(res.begin(), res.end(), [] (const RpcValue& a, const RpcValue& b) {
		return a.asList().front().asString() < b.asList().front().asString();
	});

	return res;
}

void create_dummy_cache_files(const std::string& site_path, const std::vector<DummyFileInfo>& files)
{
	remove_cache_contents(site_path);
	auto cache_dir = get_site_cache_dir(site_path);

	for (const auto& file : files) {
		// I can't think of a better way to create all parent
		// directories of file name without first creating the filename
		// as a directory and then removing it.
		QDir(cache_dir.filePath(file.fileName)).mkpath(".");
		QDir(cache_dir.filePath(file.fileName)).removeRecursively();
		QFile qfile(cache_dir.filePath(file.fileName));
		qfile.open(QFile::WriteOnly);
		qfile.write(file.content.c_str());
	}
}

std::string join(const std::string& a, const std::string& b)
{
	return shv::core::utils::joinPath(a, b);
}

RpcValue operator""_cpon(const char* data, size_t size)
{
  return RpcValue::fromCpon(std::string{data, size});
}
