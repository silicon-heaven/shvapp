#include "brclabparser.h"

#include <shv/chainpack/chainpackreader.h>
#include <shv/core/exception.h>

#include <fstream>

namespace cp = shv::chainpack;
static constexpr double TO_VOLTS_RATIO = 2.5 / 4095;

BrclabParser::BrclabParser()
{

}

cp::RpcValue BrclabParser::parse(const QString &fn)
{
	shv::chainpack::RpcValue data = readBrclabFile(fn);
	cp::RpcValue::MetaData md = data.metaData();

	const shv::chainpack::RpcValue::String &s = data.asString();
	QByteArray ba = qUncompress(reinterpret_cast<const uchar*>(s.data()), static_cast<int>(s.size()));
	shv::chainpack::RpcValue body = cp::RpcValue::fromChainPack(ba.toStdString());

	if (!body.isMap()) {
		SHV_EXCEPTION("Invalid chainpack format, document must be a map");
	}

	cp::RpcValue::Map ret;
	ret["sha1"] = md.value("documentHash");
	ret["dcsMode"] = deviceTypeToString(md.value("deviceType").toInt());

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

	ret["dipSwitchesOn"] = device.toMap().value("dipSwitchesOn").toString();
	ret["tcName"] = tc.value("name");
	ret["detectorCapacity"] = tc.value("capacity");
	ret["detectorResistor"] = tc.value("resistor");
	ret["jumperResistor"] = tc.value("jumper_r");
	ret["feedingVoltage"] = tc.value("feeding_voltage");
	ret["detectorLength"] = tc.value("length").toUInt();
	ret["detectorShape "] = tc.value("type").toInt();
	ret["trafficType"] = ((tc.value("only_rail_vehicles").toBool()) ? "tramsOnly" : "mixed");

	const cp::RpcValue::Map &tuning = device.toMap().value("tunning").toMap();
	const cp::RpcValue::Map &tuning_results = tuning.value("results").toMap();
	const cp::RpcValue::List &tunning_series = tuning.value("series").toList();

	ret["resonantFrequency"] = tuning_results.value("RSNT_FRQ_DAC");
	ret["Q"] = tuning_results.value("Q").toInt() / 100.0;
	ret["Umax"] = tuning_results.value("AMPL_MAX").toInt() * TO_VOLTS_RATIO;
	ret["operationlGain"] = tuning_results.value("OPER_GAIN_SCANNED");
	ret["tuningCurve"] = tunning_series;

	return ret;
}

std::string BrclabParser::deviceTypeToString(int device_type)
{
	enum DeviceType {Unknown = 0, Single, DCS, SIL3};

	switch (device_type) {
	case DeviceType::Single : return "single";
	case DeviceType::DCS: return "double";
	case DeviceType::SIL3: return "sil3";
	default: return "unknown";
	}
}

std::string BrclabParser::tcTypeToString(int tc_type)
{
	enum TcType {NotSelected = 0, ComplexRectangular, Rectangular, Triangular, Twincap, Double, MetalDetector, MetalDetectorPrefabricated,
			InductiveCoupled, TypeEnumLast};

	switch (tc_type) {
	case TcType::NotSelected: return "notSelected";
	case TcType::ComplexRectangular: return "crtc";
	case TcType::Rectangular: return "rtc";
	case TcType::Triangular: return "ttc";
	case TcType::Twincap: return "twctc";
	case TcType::Double: return "dtc";
	case TcType::MetalDetector: return "md";
	case TcType::MetalDetectorPrefabricated: return "mdp";
	case TcType::InductiveCoupled: return "ictc";
	default: return "unknown";
	}
}
