#include "network.h"

#include <shv/coreqt/log.h>

#include <QAbstractSocket>
#include <QNetworkInterface>

namespace utils {

QHostAddress Network::primaryPublicIPv4Address()
{
	QList<QHostAddress> addrs = QNetworkInterface::allAddresses();
	for(const QHostAddress &addr : addrs) {
		shvDebug() << addr.toString() << "is global:" << addr.isGlobal();
		if(addr.protocol() == QAbstractSocket::IPv4Protocol) {
			if(addr.isGlobal()) {
				quint32 a = addr.toIPv4Address();
				// 10.0.0.0/8
				if((a & 0xFF000000) == 0x0A000000)
					continue;
				// 172.16.0.0/12
				if((a & 0xFFF00000) == 0xAC100000)
					continue;
				// 192.168.0.0/16
				if((a & 0xFFFF0000) == 0xC0A80000)
					continue;
				return addr;
			}
		}
	}
	return QHostAddress();
}

QHostAddress Network::primaryIPv4Address()
{
	QList<QHostAddress> addrs = QNetworkInterface::allAddresses();
	for(const QHostAddress &addr : addrs) {
		shvDebug() << addr.toString() << "is global:" << addr.isGlobal();
		if(addr.protocol() == QAbstractSocket::IPv4Protocol) {
			if(addr.isGlobal()) {
				return addr;
			}
		}
	}
	return QHostAddress(QHostAddress::LocalHost);
}

} // namespace utils
