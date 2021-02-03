#ifndef DISKCLEANER_H
#define DISKCLEANER_H

#include <QDateTime>
#include <QDir>
#include <QMap>
#include <QObject>
#include <QTimer>

class DiskCleaner : public QObject
{
	Q_OBJECT

public:
	explicit DiskCleaner(int64_t cache_size_limit, QObject *parent = nullptr);

private:
	struct DeviceOccupationInfo
	{
		int64_t occupiedSize = 0LL;
		QFileInfo oldestFileInfo;
		int64_t oldestFileTs = std::numeric_limits<int64_t>::max();
	};
	struct CheckDiskContext
	{
		int64_t totalSize = 0LL;
		QMap<QString, DeviceOccupationInfo> deviceOccupationInfo;
	};

	class ScopeGuard
	{
	public:
		ScopeGuard(bool &val) : m_value(val) { val = true; }
		~ScopeGuard() { m_value = false; }
	private:
		bool &m_value;
	};

	void checkDiskOccupation();
	void scanDir(QDir &dir, CheckDiskContext &ctx);

	QTimer m_cleanTimer;
	bool m_isChecking;
	int64_t m_cacheSizeLimit;
};

#endif // DISKCLEANER_H
