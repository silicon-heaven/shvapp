#ifndef POIMODEL_H
#define POIMODEL_H

#include <QAbstractListModel>
#include <QPair>

using GPS = QPair<double, double>;

class ShvSite
{
public:
	ShvSite() : gps(0, 0) { }
	ShvSite(const QString &shv_path, const QString &name, GPS gps)
		: shvPath(shv_path)
		, name(name)
		, gps(gps)
	{ }

	QString shvPath;
	QString name;
	GPS gps;
	bool brokerConnected = false;
};

class SitesModel : public QAbstractListModel
{
	Q_OBJECT

	using Super = QAbstractListModel;
public:
	enum SiteRoles {
		ShvPathRole = Qt::UserRole + 1,
		NameRole,
		GpsRole,
		BrokerConnectedRole,
		COUNT,
	};
	Q_ENUMS(SiteRoles)

	SitesModel(QObject *parent = nullptr) : Super(parent) {}
	~SitesModel() override {}

	//QModelIndex parent(const QModelIndex &child) const override {Q_UNUSED(child) return QModelIndex();}
	//QModelIndex index(int row, int column = 0, const QModelIndex &parent = QModelIndex()) const override {return Super::index(row, column, parent);}
	//int columnCount(const QModelIndex &parent = QModelIndex()) const override {return parent.isValid() ? 0 : 1;}

	Q_INVOKABLE QVariant value(int row, const QString &role_name) const;
	Q_INVOKABLE QVariant value(int row, int role) const;

	void clear();
	QHash<int, QByteArray> roleNames() const override;
	int rowCount(const QModelIndex &parent = QModelIndex()) const override {Q_UNUSED(parent) return m_sites.count();}
	QVariant data(const QModelIndex &index, int role) const override;
	ShvSite* addSite(const QString &shv_path, const QString &name, GPS gps);
	void setSiteBrokerConnected(const std::string &shv_path, bool is_connected);
private:
	QList<ShvSite> m_sites;
};




#endif // POIMODEL_H
