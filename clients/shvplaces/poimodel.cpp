#include "poimodel.h"

QHash<int, QByteArray> PoiModel::roleNames() const
{
	QHash<int, QByteArray> roles;
	roles[ShvPathRole] = "shvPath";
	roles[NameRole] = "name";
	roles[GpsRole] = "gps";
	return roles;
}
