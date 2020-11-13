#ifndef DISKCLEANER_H
#define DISKCLEANER_H

#include <QDateTime>
#include <QDir>
#include <QObject>
#include <QTimer>

class DiskCleaner : public QObject
{
	Q_OBJECT

public:
	explicit DiskCleaner(int64_t cache_size_limit, QObject *parent = nullptr);

private:
	struct CheckDiskContext {
		int64_t total_size = 0LL;
		QFileInfo oldest_file_info;
		int64_t oldest_file_ts = std::numeric_limits<int64_t>::max();
	};

	class ScopeGuard
	{
	public:
		bool try_lock();
		void unlock();
	private:
		bool m_lock = false;
	};

	void checkDiskOccupation();
	void scanDir(const QDir &dir, CheckDiskContext &ctx);

	QTimer m_cleanTimer;
	ScopeGuard m_checkingScopeGuard;
	int64_t m_cacheSizeLimit;
};

#endif // DISKCLEANER_H
