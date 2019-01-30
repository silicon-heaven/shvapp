#include "brclabparser.h"

#include <shv/chainpack/chainpackreader.h>
#include <shv/core/exception.h>

#include <fstream>

namespace cp = shv::chainpack;

BrclabParser::BrclabParser()
{

}

cp::RpcValue BrclabParser::parse(const QString &fn)
{
	shv::chainpack::RpcValue data = readBrclabFile(fn);
	cp::RpcValue::MetaData md = data.metaData();
	const shv::chainpack::RpcValue::String &s = data.toString();
	QByteArray ba = qUncompress((const uchar*)s.data(), s.size());
	shv::chainpack::RpcValue body = cp::RpcValue::fromChainPack(ba.toStdString());

	if (!body.isMap()) {
		SHV_EXCEPTION("Invalid chainpack format, document must be a map");
	}

	cp::RpcValue::Map ret;
	ret["sha1"] = md.value("documentHash");
	ret["dcsMode"] = md.value("deviceType");
	ret["trafficType"] = md.value("trafficType");

	const cp::RpcValue::List &devices = body.toMap().value("devices").toList();

	if (devices.size() > 2){
		SHV_EXCEPTION("Invalid chainpack format, document contains more than 2 devices.");
	}

	for (size_t i = 0; i < devices.size(); i++){
		std::string key = (i == 0) ? "BRC_A" : "BRC_B";
		ret[key] = deviceData(devices.at(i));
	}

	return ret;
}

shv::chainpack::RpcValue BrclabParser::readBrclabFile(const QString &fn)
{
	try{
		std::ifstream ifs;
		ifs.exceptions( std::ifstream::failbit | std::ifstream::badbit);
		ifs.open(fn.toStdString(), std::ios::binary);

		shv::chainpack::ChainPackReader rd(ifs);
		cp::RpcValue data = rd.read();
		ifs.close();
		return data;
	}
	catch (...) {
		SHV_EXCEPTION("Cannot open file " + fn.toStdString());
	}
}

shv::chainpack::RpcValue BrclabParser::deviceData(const shv::chainpack::RpcValue &device)
{
	shv::chainpack::RpcValue::Map ret;
	if (!device.isMap()) {
		SHV_EXCEPTION("Invalid chainpack format, device must be map");
	}

	const cp::RpcValue::Map &tc = device.toMap().value("tc_parameters").toMap();

	ret["tcName"] = tc.value("name");
	ret["detectorCapacity"] = tc.value("capacity");
	ret["detectorResistor"] = tc.value("resistor");
	ret["jumperResistor"] = tc.value("jumper_r");
	ret["feedingVoltage"] = tc.value("feeding_voltage");
	ret["detectorLength"] = tc.value("length").toUInt();
	ret["detectorShape "] = tc.value("type").toInt();

	const cp::RpcValue::Map &tuning = device.toMap().value("tunning").toMap();
	const cp::RpcValue::Map &tuning_results = tuning.value("results").toMap();
	const cp::RpcValue::List &tunning_series = tuning.value("series").toList();


	ret["resonantFrequency"] = tuning_results.value("RSNT_FRQ_DAC");
	ret["Q"] = tuning_results.value("Q");
	ret["Umax"] = tuning_results.value("AMPL_MAX");
	ret["operationlGain"] = tuning_results.value("OPER_GAIN_SCANNED");
	ret["tuningCurve"] = tunning_series;

	return ret;
}
