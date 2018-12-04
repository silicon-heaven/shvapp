#include "network.h"

#include <shv/core/log.h>

#ifdef __unix__
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#endif

namespace utils {

#ifdef __unix__
std::vector<Network::IPInfo> Network::scanIP()
{
	std::vector<IPInfo> ret;
	struct ifaddrs *ifaddr, *ifa;

	if (getifaddrs(&ifaddr) == -1) {
		shvError() << "getifaddrs:" << strerror(errno);
		return ret;
	}

	for (ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
		if (ifa->ifa_addr == nullptr)
			continue;

		int family = ifa->ifa_addr->sa_family;
		/*
		printf("%s  address family: %d%s\n",
			   ifa->ifa_name, family,
			   (family == AF_PACKET) ? " (AF_PACKET)" :
									   (family == AF_INET) ?   " (AF_INET)" :
															   (family == AF_INET6) ?  " (AF_INET6)" : "");
		*/
		/* For an AF_INET* interface address, display the address */

		if (family == AF_INET || family == AF_INET6) {
			char host[NI_MAXHOST];
			int s = getnameinfo(ifa->ifa_addr,
							(family == AF_INET) ? sizeof(struct sockaddr_in) :
												  sizeof(struct sockaddr_in6),
							host, NI_MAXHOST, nullptr, 0, NI_NUMERICHOST);
			if (s == 0)
				ret.emplace_back(std::string(ifa->ifa_name), std::string(host), (family == AF_INET)? IPVer::V4: IPVer::V6);
			else
				shvError() << "getnameinfo() failed::" << gai_strerror(s);
		}
	}

	freeifaddrs(ifaddr);
	return ret;
}
#else
std::vector<Network::IPInfo> Network::scanIP()
{
	shvError() << "Network::scanIP() - Not implemented on this platform";
	return Network::IPInfo();
}
#endif

} // namespace utils
