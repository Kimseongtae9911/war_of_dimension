#ifndef GAMETIMER_H
#define GAMETIMER_H

class CGameTimer
{
public:
	CGameTimer();
 
	float TotalTime()const; // 초단위
	float DeltaTime()const; // 초단위

	void Reset(); // 메시지 루프 이전에 호출
	void Start(); // 타이머 시작 시 호출
	void Stop();  // 타이머 정지 시 호출
	void Tick();  // 프레임 단위로 호출

private:
	double mSecondsPerCount;
	double mDeltaTime;

	__int64 mBaseTime;
	__int64 mPausedTime;
	__int64 mStopTime;
	__int64 mPrevTime;
	__int64 mCurrTime;

	bool mStopped;
};

#endif // GAMETIMER_H