#pragma once

#include <QVariant>

namespace timeline {

struct Sample
{
	using timemsec_t = int64_t;

	timemsec_t time = 0;
	QVariant value;
	//timemsec_t origtime = 0; // sometimes is needed to show samples in transformed time scale (hide empty areas without samples)

	Sample() {}
	Sample(timemsec_t t, const QVariant &v) : time(t), value(v) {}
	Sample(timemsec_t t, QVariant &&v) : time(t), value(std::move(v)) {}

	//inline timemsec_t displayTime() const {return origtime != 0? origtime: time;}
};

} // namespace timeline

