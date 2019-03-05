#ifndef BRCLABPARSER_H
#define BRCLABPARSER_H

#include "shv/chainpack/rpcvalue.h"
#include <QString>

class BrclabParser
{
public:
	BrclabParser();
	static shv::chainpack::RpcValue parse(const QString &fn);
private:
	static shv::chainpack::RpcValue readBrclabFile(const QString &fn);
	static shv::chainpack::RpcValue deviceData(const shv::chainpack::RpcValue &device);
	static std::string deviceTypeToString(int device_type);
	static std::string tcTypeToString(int tc_type);
};

#endif // BRCLABPARSER_H
