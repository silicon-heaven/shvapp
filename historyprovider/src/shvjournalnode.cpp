#include "shvjournalnode.h"
#include "historyapp.h"

#include <shv/coreqt/log.h>
#include <shv/iotqt/rpc/deviceconnection.h>
#include <shv/iotqt/rpc/rpccall.h>

#include <QDir>
#include <QTimer>

#define journalDebug() shvCDebug("historyjournal")
#define journalInfo() shvCInfo("historyjournal")
#define journalError() shvCError("historyjournal")

namespace cp = shv::chainpack;
namespace {
std::vector<cp::MetaMethod> methods {
	{cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_READ},
	{cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_READ},
	{"syncLog", cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_WRITE},
};
}

ShvJournalNode::ShvJournalNode(const QString& site_path)
	: Super("shvjournal")
	, m_logsPath(QString::fromStdString(shv::core::Utils::joinPath(site_path.toStdString(), std::string{".app/shvjournal"})))
	, m_repoPath(QString::fromStdString(shv::core::Utils::joinPath(std::string("/tmp/historyprovider"), site_path.toStdString())))
{
	QDir(m_repoPath).mkpath(".");
}

size_t ShvJournalNode::methodCount(const StringViewList&)
{
	return methods.size();
}

const cp::MetaMethod* ShvJournalNode::metaMethod(const StringViewList&, size_t index)
{
	return &methods.at(index);
}

class FileSyncer : public QObject {
	Q_OBJECT;
public:
	FileSyncer(const cp::RpcValue& files, const QString& logs_path, const QString& repo_path, const cp::RpcRequest& request)
		: m_files(files)
		, m_filesList(m_files.asList())
		, m_currentFile(m_filesList.begin())
		, m_logsPath(logs_path)
		, m_repoPath(repo_path)
		, m_request(request)
	{
		connect(this, &FileSyncer::fileDone, [this] {
			++m_currentFile;
			syncCurrentFile();
		});

		syncCurrentFile();
	}

	Q_SLOT void syncCurrentFile()
	{
		if (m_currentFile == m_filesList.end()) {
			auto response = m_request.makeResponse();
			response.setResult("All files have been synced");
			HistoryApp::instance()->rpcConnection()->sendMessage(response);
			deleteLater();
			return;
		}

		auto file_name = QString::fromStdString(m_currentFile->asList().at(0).asString());
		auto full_file_name = QDir(m_repoPath).filePath(file_name);
		QFile file(full_file_name);
		auto local_size = file.size();
		auto remote_size = m_currentFile->asList().at(2).toInt();

		journalInfo() << "Syncing file" << full_file_name << "remote size:" << remote_size << "local size:" << (file.exists() ? QString::number(local_size) : "<doesn't exist>");
		if (file.exists()) {
			if (local_size == remote_size) {
				emit fileDone();
				return;
			}
		}

		auto sites_log_file = m_logsPath + "/" + file_name;
		journalDebug() << "Retrieving" << sites_log_file << "offset:" << local_size;
		auto call = shv::iotqt::rpc::RpcCall::create(HistoryApp::instance()->rpcConnection())
			->setShvPath(sites_log_file)
			->setMethod("read")
			->setParams(cp::RpcValue::Map{{"offset", cp::RpcValue::Int(local_size)}});
		connect(call, &shv::iotqt::rpc::RpcCall::error, [sites_log_file] (const QString& error) {
			journalError() << "Couldn't retrieve" << sites_log_file << ":" << error;
		});

		connect(call, &shv::iotqt::rpc::RpcCall::result, [this, full_file_name, sites_log_file] (const cp::RpcValue& result) {
			journalDebug() << "Got" << sites_log_file;
			QFile log_file(full_file_name);
			if (log_file.open(QFile::WriteOnly | QFile::Append)) {
				auto blob = result.asBlob();
				journalDebug() << "Writing" << full_file_name;
				log_file.write(reinterpret_cast<const char*>(blob.data()), blob.size());
			} else {
				journalError() << "Couldn't open" << full_file_name << "for writing";
			}
			emit fileDone();
		});
		call->start();
	}

    Q_SIGNAL void fileDone();

private:
	cp::RpcValue m_files;
	cp::RpcValue::List m_filesList;
	cp::RpcValue::List::const_iterator m_currentFile;

	QString m_logsPath;
	QString m_repoPath;

	cp::RpcRequest m_request;

};

cp::RpcValue ShvJournalNode::callMethodRq(const cp::RpcRequest &rq)
{
	auto method = rq.method().asString();
	if (method == "syncLog") {
		journalInfo() << "Syncing shvjournal" << m_logsPath;
		auto call = shv::iotqt::rpc::RpcCall::create(HistoryApp::instance()->rpcConnection())
			->setShvPath(m_logsPath)
			->setMethod("ls")
			->setParams(cp::RpcValue::Map{{"size", true}});

		connect(call, &shv::iotqt::rpc::RpcCall::error, [rq] (const QString& error) {
			journalError() << error;
			auto response = rq.makeResponse();
			response.setError(cp::RpcResponse::Error::create(
						cp::RpcResponse::Error::MethodCallException, "Couldn't retrieve filelist from the device"));
			HistoryApp::instance()->rpcConnection()->sendMessage(response);
		});

		connect(call, &shv::iotqt::rpc::RpcCall::result, [this, rq] (const cp::RpcValue& result) {
			journalDebug() << "Got filelist from" << m_logsPath;
			new FileSyncer(result, m_logsPath, m_repoPath, rq);
		});
		call->start();

		return {};
	}

	return Super::callMethodRq(rq);
}

#include "shvjournalnode.moc"
