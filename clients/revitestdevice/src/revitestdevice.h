#ifndef REVITESTDEVICE_H
#define REVITESTDEVICE_H

#include <QObject>

namespace shv { namespace chainpack { class RpcMessage; class RpcValue; }}
namespace shv { namespace iotqt { namespace node { class ShvNodeTree; }}}
namespace shv { namespace core { namespace utils { class ShvFileJournal; struct ShvJournalEntry; }}}

class RevitestDevice : public QObject
{
	Q_OBJECT
public:
	explicit RevitestDevice(QObject *parent = nullptr);

	void onRpcMessageReceived(const shv::chainpack::RpcMessage &msg);
	Q_SIGNAL void sendRpcMessage(const shv::chainpack::RpcMessage &msg);

	void getSnapshot(std::vector<shv::core::utils::ShvJournalEntry> &snapshot);
	shv::iotqt::node::ShvNodeTree * shvTree() {return m_shvTree;}
private:
	void onLublicatorPropertyValueChanged(const std::string &property_name, const shv::chainpack::RpcValue &new_val);
	void createDevices();
private:
	shv::iotqt::node::ShvNodeTree *m_shvTree = nullptr;
};

#endif // REVITESTDEVICE_H
