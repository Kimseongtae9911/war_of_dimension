#include "stdafx.h"
#include "SoundManager.h"

unique_ptr<SoundManager> SoundManager::m_instance;

void SoundManager::Initialize()
{
	Ready_SoundManager();
	//m_animSound.insert();

	//Projectile Skills Name
	m_projectileSoundName.insert({ SKILL_TYPE::WIZARD_ATTACK, pair{L"Wizzard_MagicMissile.wav", 1.0f}});
	m_projectileSoundName.insert({ SKILL_TYPE::WIZARD_MAGIC_MISSILE, pair{L"Wizzard_Magic_Missile.wav", 1.0f}});
	m_projectileSoundName.insert({ SKILL_TYPE::WIZARD_ENERGY_BALL, pair{L"Wizzard_EnergyBall01.wav", 1.0f}});
	m_projectileSoundName.insert({ SKILL_TYPE::WIZARD_BIGBANG, pair{L"Wizzard_BigBang01.wav", 1.0f}});
	m_projectileSoundName.insert({ SKILL_TYPE::WIZARD_BIGBANG_CONTINUE, pair{L"Wizzard_BigBang02.wav", 1.0f}});
	m_projectileSoundName.insert({ SKILL_TYPE::WIZARD_DARKNESS_RAY, pair{L"Wizzard_DarknessRay.wav", 1.0f}});
	m_projectileSoundName.insert({ SKILL_TYPE::SWORDMAN_AURA_BLADE, pair{L"Swordman_AuraBlade.wav", 1.0f}});
	m_projectileSoundName.insert({ SKILL_TYPE::SWORDMAN_JUDGEMENT_SWORD, pair{L"Swordman_JudgementSword02.wav", 1.0f}});
	m_projectileSoundName.insert({ SKILL_TYPE::SWORDMAN_PROTECTED_AREA, pair{L"Swordman_ProtectedArea.wav", 1.0f}});
	m_projectileSoundName.insert({ SKILL_TYPE::ARCHER_ATTACK, pair{L"Archer_Attack.wav", 1.0f}});
	m_projectileSoundName.insert({ SKILL_TYPE::ARCHER_PHOENIX_ARROW, pair{L"Archer_PhoenixArrow1.wav",1.0f}});
	m_projectileSoundName.insert({ SKILL_TYPE::ARCHER_PENETRAITING_SHOT, pair{L"Archer_PenetraitingShot.wav", 1.0f}});
	m_projectileSoundName.insert({ SKILL_TYPE::ARCHER_STICKY_ARROW, pair{L"Archer_StickyArrow.wav", 1.0f}});
	m_projectileSoundName.insert({ SKILL_TYPE::ARCHER_STROM_ARROW, pair{L"Archer_StormArrow.wav", 1.0f}});
	m_projectileSoundName.insert({ SKILL_TYPE::ARCHER_ARROW_RAIN, pair{L"Archer_ArrowRain.wav", 1.0f}});
	m_projectileSoundName.insert({ SKILL_TYPE::FIGHTER_FIREBALL, pair{L"Fighter_FireBall.wav", 1.0f}});
	m_projectileSoundName.insert({ SKILL_TYPE::PRO_RETURN_ZERO, pair{L"Programmer_Return0.wav", 1.0f}});
	m_projectileSoundName.insert({ SKILL_TYPE::PRO_SCL, pair{L"Programmer_SwitchCaseLaser.wav", 1.0f}});
	m_projectileSoundName.insert({ SKILL_TYPE::PRO_ATTACK, pair{L"Programmer_Return0.wav", 1.0f}});
	m_projectileSoundName.insert({ SKILL_TYPE::PRO_HELLO_WORLD, pair{L"Programmer_HelloWorld.wav", 1.0f}});
	m_projectileSoundName.insert({ SKILL_TYPE::OGRE_ROCK_THROW, pair{L"Ogre_RockThrow.wav", 1.0f}});
	m_projectileSoundName.insert({ SKILL_TYPE::OGRE_DIMENSION_CRUSH, pair{L"Orge_DimensionCrush.wav", 1.0f}});
	m_projectileSoundName.insert({ SKILL_TYPE::OGRE_DIMENSION_PUNCH, pair{L"Orge_DimensionPunch.wav", 1.0f}});
	m_projectileSoundName.insert({ SKILL_TYPE::ARCHER_MULTIPLE_SHOT, pair{L"Archer_MultipleShot.wav", 1.0f}});

	//Projectile Remove Skill Name
	m_projectileRemoveSoundName.insert({ SKILL_TYPE::OGRE_ROCK_THROW, pair{L"Orge_RockThrowArrive.wav", 0.8f} });

	//Normal Attack
	m_animSoundName.insert({ 1, pair{L"Fighter_Attack.wav", 1.0f}});
	m_animSoundName.insert({ 2, pair{L"Swordman_Attack.wav", 1.0f}});
	m_animSoundName.insert({ 0, pair{L"Orge_Attack.wav", 0.7f}});

	//Boss Skill Anim Name
	m_animSoundName.insert({ 21, pair{L"Ogre_HeavySwing.wav", 1.0f}});
	m_animSoundName.insert({ 22, pair{L"Orge_Crunch.wav", 0.7f}});
	m_animSoundName.insert({ 23, pair{L"Orge_Charging.wav", 0.6f}});
	m_animSoundName.insert({ 24, pair{L"Orge_Roar.wav", 0.7f}});
	m_animSoundName.insert({ 25, pair{L"Orge_Gluttony.wav", 1.0f}});
	m_animSoundName.insert({ 26, pair{L"Orge_Endure.wav", 1.0f}});
	m_animSoundName.insert({ 29, pair{L"Orge_Butting.wav", 1.0f}});

	m_animSoundName.insert({ 31, pair{L"Programmer_SwitchCaseMove.wav", 1.0f}});
	m_animSoundName.insert({ 32, pair{L"Programmer_Pointer.wav", 1.0f}});
	m_animSoundName.insert({ 33, pair{L"Programmer_PlusStat.mp3", 1.0f}});
	m_animSoundName.insert({ 34, pair{L"Programmer_MinusStats.wav", 1.0f}});
	m_animSoundName.insert({ 35, pair{L"Programmer_Release.wav", 1.0f}});
	m_animSoundName.insert({ 36, pair{L"Programmer_Delete.wav", 1.0f}});
	m_animSoundName.insert({ 39, pair{L"Programmer_WhileTrue.wav", 1.0f}});

	//Hero Skill Anim Name
	m_animSoundName.insert({ 48, pair{L"Archer_BackStep.wav", 1.0f}});
	m_animSoundName.insert({ 49, pair{L"Archer_Dodge.wav", 1.0f}});
	m_animSoundName.insert({ 51, pair{L"Archer_Vault.ogg", 1.0f}});
	m_animSoundName.insert({ 52, pair{L"Archer_VitalPoint.wav", 1.0f}});
	m_animSoundName.insert({ 55, pair{L"Archer_HunterEyes.flac", 1.0f}});
	m_animSoundName.insert({ 56, pair{L"Archer_WindStep.wav", 1.0f}});

	m_animSoundName.insert({ 60, pair{L"Fighter_Dash.wav", 1.0f}});
	m_animSoundName.insert({ 61, pair{L"Fighter_Dodge.wav", 1.0f}});
	m_animSoundName.insert({ 62, pair{L"Fighter_SpinKick.wav", 1.0f}});
	m_animSoundName.insert({ 63, pair{L"Fighter_WildAttack.wav", 1.0f}});
	m_animSoundName.insert({ 64, pair{L"Fighter_DragonFist.wav", 1.0f}});
	m_animSoundName.insert({ 65, pair{L"Fighter_Meditation.wav", 1.0f}});
	m_animSoundName.insert({ 66, pair{L"Fighter_PointBlood.wav", 1.0f}});
	m_animSoundName.insert({ 67, pair{L"Fighter_indestructible.wav", 1.0f}});
	m_animSoundName.insert({ 68, pair{L"Fighter_WindKick.wav", 1.0f}});
	m_animSoundName.insert({ 69, pair{L"Fight_Counter.wav", 1.0f}});
	m_animSoundName.insert({ 71, pair{L"Fighter_Rising Dragon.wav", 1.0f}});

	m_animSoundName.insert({ 72, pair{L"Swordman_Dodge.wav", 1.0f}});
	m_animSoundName.insert({ 73, pair{L"Swordman_Run.wav", 1.0f}});
	m_animSoundName.insert({ 74, pair{L"Swordman_HeavySlash.wav", 1.0f}});
	m_animSoundName.insert({ 75, pair{L"Swordman_ShieldBash.wav", 1.0f}});
	m_animSoundName.insert({ 76, pair{L"Swordman_WarCry.wav", 1.0f}});
	m_animSoundName.insert({ 77, pair{L"Swordman_DefensiveStance.wav", 1.0f}});
	m_animSoundName.insert({ 78, pair{L"Swordman_Berserk.wav", 1.0f}});
	m_animSoundName.insert({ 80, pair{L"Swordman_HellBlade.wav", 1.0f}});
	m_animSoundName.insert({ 81, pair{L"Swordman_JudgementSword01.wav", 1.0f}});
	m_animSoundName.insert({ 82, pair{L"Swordman_ProtectedArea.wav", 1.0f}});
	m_animSoundName.insert({ 83, pair{L"Swordman_AnkleCut.wav", 1.0f}});

	m_animSoundName.insert({ 84, pair{L"Wizzard_Teleport.wav", 1.0f}});
	m_animSoundName.insert({ 85, pair{L"Wizzard_Blink.wav", 1.0f}});
	m_animSoundName.insert({ 86, pair{L"Wizzard_BodyStrength.wav", 1.0f}});
	m_animSoundName.insert({ 88, pair{L"Wizzard_EarthImpact.wav", 1.0f}});
	m_animSoundName.insert({ 91, pair{L"Wizzard_MagicEye.wav", 1.0f}});
	m_animSoundName.insert({ 93, pair{L"Wizzard_Reflect.wav", 1.0f}});
	m_animSoundName.insert({ 95, pair{L"Wizzard_Overload.wav", 1.0f}});
}

void SoundManager::Release()
{
	Stop_All();

	for(auto & pair : m_mapSound)
		pair.second->release();

	m_mapSound.clear();

	m_pSystem->close();
	m_pSystem->release();
}


void SoundManager::Ready_SoundManager()
{
	System_Create(&m_pSystem);
	m_pSystem->init(static_cast<int>(CHANNELID::END) * MAX_SOUNDCHANNEL, FMOD_INIT_NORMAL, nullptr);
}

void SoundManager::Update_SoundManager()
{
	if (nullptr != m_pSystem)
		m_pSystem->update();
}

void SoundManager::Play_Sound(const wstring& strSoundKey, CHANNELID eID, const float volume)
{
	auto iter_find = m_mapSound.find(strSoundKey);

	if (m_mapSound.end() == iter_find)
		return;

	FMOD::Channel* pChannel = nullptr;
	m_pSystem->playSound(iter_find->second, 0, false, &pChannel);
	pChannel->setVolume(volume + m_masterVolume);
	m_pChannel[static_cast<int>(eID)].push_back(pChannel);

	if (MAX_SOUNDCHANNEL < m_pChannel[static_cast<int>(eID)].size())
	{
		pChannel = m_pChannel[static_cast<int>(eID)].front();

		if (nullptr != pChannel)
			pChannel->stop();

		m_pChannel[static_cast<int>(eID)].pop_front();
	}
}

void SoundManager::Play_Sound(int skillNum, CHANNELID eID, const float volume)
{
	if (!m_animSoundName.contains(skillNum))
		return;

	pair<wstring, float> soundInfo  = m_animSoundName.find(skillNum)->second;

	Play_Sound(soundInfo.first, eID, soundInfo.second);
}

void SoundManager::Play_Sound(SKILL_TYPE projectileSkill, CHANNELID eID, const float volume)
{
	if (!m_projectileSoundName.contains(projectileSkill))
		return;

	pair<wstring, float> soundInfo = m_projectileSoundName.find(projectileSkill)->second;

	Play_Sound(soundInfo.first, eID, soundInfo.second);
}

void SoundManager::Play_RemoveSound(SKILL_TYPE projectileSkill, CHANNELID eID)
{
	if (!m_projectileRemoveSoundName.contains(projectileSkill))
		return;

	pair<wstring, float> soundInfo = m_projectileRemoveSoundName.find(projectileSkill)->second;

	Play_Sound(soundInfo.first, eID, soundInfo.second);
}

void SoundManager::Play_BGM(const wstring& strSoundKey, const float volume)
{
	auto iter_find = m_mapSound.find(strSoundKey);

	if (m_mapSound.end() == iter_find)
		return;

	FMOD::Channel* pChannel = nullptr;

	m_pSystem->playSound(iter_find->second, 0, false, &pChannel);
	pChannel->setMode(FMOD_LOOP_NORMAL);
	pChannel->setVolume(volume + m_masterVolume);

	m_pChannel[static_cast<int>(CHANNELID::BGM)].push_back(pChannel);

	if (MAX_SOUNDCHANNEL < m_pChannel[static_cast<int>(CHANNELID::BGM)].size())
	{
		pChannel = m_pChannel[static_cast<int>(CHANNELID::BGM)].front();

		if (nullptr != pChannel)
			pChannel->stop();

		m_pChannel[static_cast<int>(CHANNELID::BGM)].pop_front();
	}
}

void SoundManager::Stop_Sound(CHANNELID eID)
{
	for (auto& pChannel : m_pChannel[static_cast<int>(eID)])
	{
		if (nullptr != pChannel)
			pChannel->stop();
	}

	m_pChannel[static_cast<int>(eID)].clear();
}

void SoundManager::Stop_All()
{
	for (int i = 0; i < static_cast<int>(CHANNELID::END); ++i)
	{
		for (auto& pChannel : m_pChannel[i])
		{
			if (nullptr != pChannel)
				pChannel->stop();
		}

		m_pChannel[i].clear();
	}
}

void SoundManager::ChangeVolume(float volume)
{
	float temp = m_masterVolume;
	m_masterVolume = volume;

	for (int i = 0; i < static_cast<int>(CHANNELID::END); ++i)
	{
		for (auto& pChannel : m_pChannel[i])
		{
			if (nullptr != pChannel) {
				float curVolume;
				pChannel->getVolume(&curVolume);
				pChannel->setVolume(curVolume - temp + m_masterVolume);
			}
		}
	}
}

HRESULT SoundManager::Load_SoundFile(const char* pFilePath)
{
	_finddata_t fd;

	char pFindFirstPath[128];
	ZeroMemory(pFindFirstPath, sizeof(char) * 128);
	strcpy_s(pFindFirstPath, pFilePath);
	strcat_s(pFindFirstPath, "*.*");
	intptr_t Handle = _findfirst(pFindFirstPath, &fd);

	if (0 == Handle)
		return E_FAIL;

	int iResult = 0;

	char szCurPath[128];
	ZeroMemory(&szCurPath, sizeof(char) * 128);
	strcpy_s(szCurPath, pFilePath);
	char szFullPath[128] = "";

	while (iResult != -1)
	{
		strcpy_s(szFullPath, szCurPath);
		strcat_s(szFullPath, fd.name);

		FMOD::Sound* pSound = nullptr;

		FMOD_RESULT eRes = m_pSystem->createSound(szFullPath, FMOD_DEFAULT, nullptr, &pSound);
		if (eRes == FMOD_OK)
		{
			int iLen = (int)strlen(fd.name) + 1;

			TCHAR* pSoundKey = new TCHAR[iLen];
			ZeroMemory(pSoundKey, iLen);

			MultiByteToWideChar(CP_ACP, 0, fd.name, iLen, pSoundKey, iLen);

			m_mapSound.emplace(pSoundKey, pSound);

			if (pSoundKey)
			{
				delete[] pSoundKey;
				pSoundKey = nullptr;
			}
		}
		iResult = _findnext(Handle, &fd);
	}
	_findclose(Handle);
	m_pSystem->update();

	return NOERROR;
}