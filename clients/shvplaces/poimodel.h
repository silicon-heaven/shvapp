#ifndef POIMODEL_H
#define POIMODEL_H

#include <QAbstractListModel>
#include <QPair>

using GPS = QPair<double, double>;

class Predator
{
public:
	Predator() : gps(0, 0) { }
	Predator(const QString &shv_path, const QString &name, GPS gps)
		: shvPath(shv_path)
		, name(name)
		, gps(gps)
	{ }

	QString shvPath;
	QString name;
	GPS gps;
};

class PoiModel : public QAbstractListModel
{
	Q_OBJECT

	using Super = QAbstractListModel;
public:
	enum PredatorRoles {
		ShvPathRole = Qt::UserRole + 1,
		NameRole,
		GpsRole,
	};

	PoiModel(QObject *parent = nullptr) : Super(parent) {}
	~PoiModel() override {}

	QModelIndex parent(const QModelIndex &child) const override {Q_UNUSED(child) return QModelIndex();}
	QModelIndex index(int row, int column = 0, const QModelIndex &parent = QModelIndex()) const override {return Super::index(row, column, parent);}
	int columnCount(const QModelIndex &parent = QModelIndex()) const override {return parent.isValid() ? 0 : 1;}

	QHash<int, QByteArray> roleNames() const override;
	int rowCount(const QModelIndex &parent) const override {Q_UNUSED(parent) return m_predators.count();}
	QVariant data(const QModelIndex &index, int role) const override
	{
		if(role == ShvPathRole) {
			return m_predators[index.row()].shvPath;
		}
		if(role == NameRole || role == Qt::DisplayRole) {
			return m_predators[index.row()].name;
		}
		if(role == GpsRole) {
			GPS gps = m_predators[index.row()].gps;
			return QVariantList{gps.first, gps.second};
		}
		return QVariant();
	}
	void addPredator(const QString &shv_path, const QString &name, GPS gps)
	{
		m_predators.append(Predator(shv_path, name, gps));
	}
private:
	QList<Predator> m_predators;
};




#endif // POIMODEL_H
