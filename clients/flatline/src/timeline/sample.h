#pragma once

#include <QVariant>

#include <limits>

namespace timeline {

using timemsec_t = int64_t;

struct Sample
{

	timemsec_t time = 0;
	QVariant value;
	//timemsec_t origtime = 0; // sometimes is needed to show samples in transformed time scale (hide empty areas without samples)

	Sample() {}
	Sample(timemsec_t t, const QVariant &v) : time(t), value(v) {}
	Sample(timemsec_t t, QVariant &&v) : time(t), value(std::move(v)) {}

	bool isValid() const {return value.isValid() && time > 0;}
};

template<typename T>
struct Range
{
	T min = std::numeric_limits<T>::max();
	T max = std::numeric_limits<T>::min();

	Range() {}
	Range(T mn, T mx) : min(mn), max(mx) {}
	//XRange(const QPair<timemsec_t, timemsec_t> r) : min(r.first), max(r.second) {}

	//bool operator==(const T &o) const { return min == o.min && max == o.max; }
	bool isValid() const { return min <= max; }
	bool isEmpty() const { return min >= max; }
	T interval() const {return max - min;}
};

using XRange = Range<timemsec_t>;
using YRange = Range<double>;

} // namespace timeline

