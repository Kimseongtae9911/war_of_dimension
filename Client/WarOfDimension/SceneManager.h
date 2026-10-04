#pragma once


struct ChattingInfo
{
	WCHAR ChatBuf[256] = L"";
	WCHAR TempChatBuf[2] = L"";
	bool bOnChat = false;
	CHAT eChatOption = CHAT::CHANNEL;
};

struct TitleInfo
{
	ChattingInfo Chat;
	WCHAR passBuf[BUF_SIZE] = L"";
	bool passShow[BUF_SIZE] = { true };
	bool pw = false;
};

struct LobbyInfo
{
	ChattingInfo Chat;
};

struct ReadyInfo
{
	ChattingInfo Chat;	
};

struct ParticleInfo
{
	XMFLOAT3 pos = XMFLOAT3(0,0,0);
	XMFLOAT3 Dir = XMFLOAT3(1,0,0);
	bool	show = false;
};

class SceneManager
{
	SINGLETON(SceneManager);
public:
	void Reset();

	char m_Name[NAME_SIZE] = {};
	POINT ptCursorPos = {}; //Screen Point
	SCENEKIND m_nCurScene = SCENEKIND::TITLE;

	TitleInfo m_TitleInfo;
	LobbyInfo m_LobbyInfo;
	ReadyInfo m_ReadyInfo;

	array<vector<int>, 48> m_MatchingAniList;
	unordered_map<PARTICLE_SITUATION, array<vector<ParticleInfo*>, 5>> m_ParticleInfo;
	
	UINT m_nDrawOption = HABLE_MACCANN;
	bool m_shadow = true;
	bool m_outline = true;
	float m_fExposure = 1.1f, m_fSaturation = 1.2f, m_fContrast = 1.2f, m_fVibrance = 1.6f;

	//Loading
	bool	m_bWorkingThread = true;
	float   m_fLoadingProgressPercent = 0.f;
	float ToLobbyPercent[LOADING_TEXT_COUNT] = { 0.3f, 0.8f, 1.0f };
	float ToReadyPercent[LOADING_TEXT_COUNT] = { 0.25f, 0.85f, 1.0f };
	float ToIngamePercent[LOADING_TEXT_COUNT] = { 0.35f, 0.95f, 1.0f };
public:
	ORDER GetOrder() { return myClientOrder; }
	void SetOrder(ORDER ord) { myClientOrder = ord; }
	void ReadFile();
	

	JOB GetJob() { return myClinetJob; }
	void SetJob(JOB ord) { myClinetJob = ord; }

private:
	ORDER myClientOrder = ORDER::PLAYER1;
	JOB myClinetJob = JOB::ARCHER;
};

