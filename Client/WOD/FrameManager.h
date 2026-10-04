#pragma once
#include "stdafx.h"
class CFrameManager
{
private:
	CFrameManager();
	virtual ~CFrameManager();

public:
	const float& GetDelta() const;

public:
	void UpdateTime();
	bool FrameLimit(float fLimit);

public:
	void Release();

private:
	// Delta Time
	LARGE_INTEGER	m_CurTime;
	LARGE_INTEGER	m_OldTime;
	LARGE_INTEGER	m_CpuTick;
	float			m_fDeltaTime = 0.f;

private:
	// Frame Limit
	LARGE_INTEGER	m_FrameCurTime;
	LARGE_INTEGER	m_FrameOldTime;
	LARGE_INTEGER	m_FrameCpuTick;
	float			m_fTimeCount = 0.f;
};

