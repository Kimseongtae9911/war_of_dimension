#pragma once

class SoundManager
{
	SINGLETON(SoundManager)


public:
	void Initialize();
	void Release();

	void Update_SoundManager();

private:
	void Ready_SoundManager();

public:
	void Play_Sound(const wstring& strSoundKey, CHANNELID eID, const float volume = 1.0f);
	void Play_Sound(int skillNum, CHANNELID eID, const float volume = 1.0f);
	void Play_Sound(SKILL_TYPE projectileSkill, CHANNELID eID, const float volume = 1.0f);
	void Play_RemoveSound(SKILL_TYPE projectileSkill, CHANNELID eID);
	void Play_BGM(const wstring& strSoundKey, const float volume = 1.0f);

	void Stop_Sound(CHANNELID eID);
	void Stop_All();

	void ChangeVolume(float volume);

public:
	HRESULT	Load_SoundFile(const char* pFilePath);

private:
	map<wstring, FMOD::Sound*> m_mapSound;
	list<FMOD::Channel*> m_pChannel[static_cast<int>(CHANNELID::END)];
	FMOD::System* m_pSystem;

	unordered_map<int, pair<wstring, float>> m_animSoundName;
	unordered_map<SKILL_TYPE, pair<wstring, float>> m_projectileSoundName;
	unordered_map<SKILL_TYPE, pair<wstring, float>> m_projectileRemoveSoundName;

	float m_masterVolume = INIT_VOLUME_FLOAT;
};

