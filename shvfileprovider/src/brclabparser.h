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
};

#endif // BRCLABPARSER_H
