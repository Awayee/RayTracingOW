#pragma once
#include <chrono>

typedef std::chrono::steady_clock::time_point TimePoint;
inline TimePoint NowTimePoint() {
	return std::chrono::steady_clock::now();
}
template<typename T> T NowTimeMs() {
	return static_cast<T>(NowTimePoint().time_since_epoch().count());
}
template<typename T> T GetDurationMill(const TimePoint& start, const TimePoint& end) {
	return std::chrono::duration_cast<std::chrono::duration<T, std::milli>>(end - start).count();
}