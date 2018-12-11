#pragma once

#include <QString>

class QHostAddress;

namespace utils {

class Network
{
public:
	static uint32_t toIntIPv4Address(const std::string &addr);
	static bool isGlobalIPv4Address(uint32_t addr);
	static bool isPublicIPv4Address(uint32_t addr);
	static QHostAddress primaryPublicIPv4Address();
	static QHostAddress primaryIPv4Address();
};

} // namespace utils

