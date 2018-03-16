#ifndef ATTRIBUTESMODEL_H
#define ATTRIBUTESMODEL_H

//#include <qfopcua/client.h>
//#include <qfopcua/nodeid.h>

#include <QStandardItemModel>
#include <QPointer>

class ShvNodeItem;

namespace shv { namespace chainpack { class RpcMessage; }}

class AttributesModel : public QStandardItemModel
{
	Q_OBJECT
private:
	typedef QStandardItemModel Super;
public:
	AttributesModel(QObject *parent = nullptr);
	~AttributesModel() Q_DECL_OVERRIDE;
public:
	Qt::ItemFlags flags(const QModelIndex &ix) const Q_DECL_OVERRIDE;
	QVariant data ( const QModelIndex & index, int role = Qt::DisplayRole ) const Q_DECL_OVERRIDE;
	bool setData(const QModelIndex &ix, const QVariant &val, int role = Qt::EditRole) Q_DECL_OVERRIDE;
	QVariant headerData ( int section, Qt::Orientation o, int role = Qt::DisplayRole ) const Q_DECL_OVERRIDE;

	void load(ShvNodeItem *nd);
private:
	void onRpcMessageReceived(const shv::chainpack::RpcMessage &msg);
private:
	//QPointer<qfopcua::Client> m_client;
	ShvNodeItem *m_nodeItem = nullptr;
	unsigned m_rpcRqId = 0;
	//shv::iotqt::rpc::ClientConnection *m_rpcConnection = nullptr;
};

#endif // ATTRIBUTESMODEL_H
