#pragma once

namespace wod_server {
class CMatch
{
	enum class ESceneType { ReadyScene = 0, LoadingScene = 1, GameScene = 2 };	

public:
	CMatch() {}
	~CMatch() {}

	//Connecting
	void ConnectUpdate(int matchNum);
	void ClientConnect(const CS_LOGIN_PACKET* packet, std::shared_ptr<CClient> client);

	//ReadyScene
	void RegisterClient(int matchID, int id) { m_clientid[matchID] = id; }
	bool IsAllReady() { return std::ranges::all_of(m_clientReady, [](bool ready) {return ready; }); }
	bool IsLoadComplete() { return std::ranges::all_of(m_clientLoading, [](bool ready) {return ready; }); }
	void ReadyUpdate(int matchNum);
	void SetReady(const CS_READY_PACKET* packet);
	void SelectSkill(const CS_SKILL_SELECT_PACKET* packet, std::shared_ptr<CClient> client);
	void SelectJob(const CS_JOB_SELECT_PACKET* packet, std::shared_ptr<CClient> client);
	void SelectStat(const CS_STAT_SELECT_PACKET* packet, std::shared_ptr<CClient> client);

	//Loading
	void LoadComplete(const CS_LOAD_COMPLETE_PACKET* packet, int id) { m_clientLoading[id] = true; }
	void LoadingUpdate(int matchNum);

	//InGame
	void TeleportStart(std::shared_ptr<CClient> client);
	void UseItem(const CS_USE_ITEM_PACKET* packet, std::shared_ptr<CClient> client);
	void InGameUpdate(int matchNum);

	void Update();
	void Reset();

	template<typename Func>
	void PushJob(Func&& f) { m_jobQueue.PushJob(std::forward<Func>(f)); }

	const std::array<int, MAX_PLAYER>& GetClientIds() const { return m_clientid; }

private:
	std::array<int, MAX_PLAYER> m_clientid = { -1, -1, -1, -1 };
	std::array<bool, MAX_PLAYER> m_clientReady = { false, false, false, false };
	std::array<bool, MAX_PLAYER> m_clientLoading = { false, false, false, false };
	ESceneType m_sceneType = ESceneType::ReadyScene;
	JobQueue m_jobQueue;

	float m_readyTime = 120.f;
	TimePoint m_updateTime = {};
	TimePoint m_lastGoldUpdateTime = {};
	TimePoint m_lastPlayerHeal = {};
	TimePoint m_lastMinionRespawn = {};
};

}