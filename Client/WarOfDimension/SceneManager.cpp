#include "stdafx.h"
#include "SceneManager.h"

std::unique_ptr<SceneManager> SceneManager::m_instance;


void SceneManager::Reset()
{
	myClientOrder = ORDER::PLAYER1;
	myClinetJob = JOB::ARCHER;
	m_bWorkingThread = true;
	m_fLoadingProgressPercent = 0.f;
}

void SceneManager::ReadFile()
{
	std::fstream in("MatchingSkillAni.txt");
	if (in.fail())
		cout << "Failed to read file" << endl;

	int ArrayNum = 0;
	int AniCount = 0;
	int AniNum = 0;

	while (!in.eof())
	{
		in >> ArrayNum;
		in >> AniCount;
		if (AniCount > 0)
		{
			vector<int> vecTemp;
			for (int i = 0; i < AniCount; ++i)
			{
				in >> AniNum;
				vecTemp.push_back(AniNum);
			}		
			m_MatchingAniList[ArrayNum - 1] = vecTemp;
		}
	}
}
