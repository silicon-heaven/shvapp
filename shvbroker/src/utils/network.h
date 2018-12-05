#pragma once

#include <QString>

class QHostAddress;

namespace utils {

class Network
{
public:
	static QHostAddress primaryPublicIPv4Address();
	static QHostAddress primaryIPv4Address();
};

} // namespace utils

