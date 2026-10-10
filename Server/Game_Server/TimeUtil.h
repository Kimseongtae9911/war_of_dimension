#pragma once

namespace wod_server {

class TimeUtil
{
public:
	static constexpr uint8_t m_Sec = 1;
	static constexpr uint8_t m_Min = m_Sec * 60;
	static constexpr uint8_t m_Hour = m_Min * 60;
	static constexpr uint8_t m_Day = m_Hour * 24;

	static const std::chrono::time_point<std::chrono::system_clock> NextFrameTime() { return std::chrono::system_clock::now() + std::chrono::milliseconds(15); }
	static const std::chrono::time_point<std::chrono::system_clock> PassedTimeMSec(int _millisecond) { return std::chrono::system_clock::now() + std::chrono::milliseconds(_millisecond); }
	static const std::chrono::time_point<std::chrono::system_clock> CurTime() { return std::chrono::system_clock::now(); }
	static float CalElapsedTime(std::chrono::system_clock::time_point _lastTime) { return (std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now() - _lastTime).count() * 0.000001f); }
};

}