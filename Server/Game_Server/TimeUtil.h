#pragma once

namespace wod_server {

class TimeUtil
{
public:
	static constexpr uint8_t Sec = 1;
	static constexpr uint8_t Min = Sec * 60;
	static constexpr uint8_t Hour = Min * 60;
	static constexpr uint8_t Day = Hour * 24;

	static const std::chrono::time_point<std::chrono::system_clock> NextFrameTime() { return std::chrono::system_clock::now() + std::chrono::milliseconds(15); }
	static const std::chrono::time_point<std::chrono::system_clock> PassedTimeMSec(int millisecond) { return std::chrono::system_clock::now() + std::chrono::milliseconds(millisecond); }
	static const std::chrono::time_point<std::chrono::system_clock> CurTime() { return std::chrono::system_clock::now(); }
	static float CalElapsedTime(std::chrono::system_clock::time_point lastTime) { return (std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now() - lastTime).count() * 0.000001f); }
};

}