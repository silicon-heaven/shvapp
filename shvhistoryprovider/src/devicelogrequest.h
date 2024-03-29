#ifndef DEVICELOGREQUEST_H
#define DEVICELOGREQUEST_H

#include "logdir.h"

#include <shv/core/utils/shvgetlogparams.h>
#include <shv/core/utils/shvmemoryjournal.h>
#include <shv/chainpack/rpcmessage.h>

#include <QDateTime>

class DeviceLogRequest : public QObject
{
	Q_OBJECT
	using Super = QObject;

public:
	explicit DeviceLogRequest(const QString &site_path, const QDateTime &since, const QDateTime &until, QObject *parent = nullptr);

	void exec();

	const QDateTime &since() const { return m_since; }
	const QDateTime &until() const { return m_until; }

	Q_SIGNAL void finished(bool);

private:
	void getChunk();
	void onChunkReceived(const shv::chainpack::RpcResponse &response);
	shv::core::utils::ShvGetLogParams logParams(bool with_snapshot = true) const;
	void appendToPreviousFile(shv::core::utils::ShvMemoryJournal &log, const QDateTime &until);
	void prependPreviousFile(shv::core::utils::ShvMemoryJournal &log);
	void saveToNewFile(shv::core::utils::ShvMemoryJournal &log, const QDateTime &until);
	void trimDirtyLog(const QDateTime &until);
	void fixFirstLogFile();

	const QString m_sitePath;
	QDateTime m_since;
	QDateTime m_until;
	LogDir m_logDir;
	bool m_askElesys;
	bool m_appendLog;
};

#endif // DEVICELOGREQUEST_H
