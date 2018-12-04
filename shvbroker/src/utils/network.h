#pragma once

#include <vector>
#include <string>

namespace utils {

class Network
{
public:
	enum class IPVer {Invalid, V4, V6};
	struct IPInfo
	{
		std::string interface;
		std::string host;
		IPVer version = IPVer::Invalid;

		IPInfo() {}
		IPInfo(std::string i, std::string h, IPVer v)
			: interface(std::move(i))
			, host(std::move(h))
			, version(v)
		{}
	};

	static std::vector<IPInfo> scanIP();
};

} // namespace utils

