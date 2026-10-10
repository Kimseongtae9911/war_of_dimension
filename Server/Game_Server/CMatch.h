#pragma once

namespace wod_server {
class CMatch
{
	enum class ESceneType { ReadyScene = 0, LoadingScene = 1, GameScene = 2 };

public:
	CMatch() {}
	~CMatch() {}

	//Connecting
	void ConnectUpdate(int _matchNum);
	void ClientConnect(const CS_LOGIN_PACKET* _packet, std::shared_ptr<CClient> _client);

	//ReadyScene
	void RegisterClient(int _matchID, int _id) { m_clientid[_matchID] = _id; }
	bool IsAllReady() { return std::ranges::all_of(m_clientReady, [](bool _ready) {return _ready; }); }
	bool IsLoadComplete() { return std::ranges::all_of(m_clientLoading, [](bool _ready) {return _ready; }); }
	void ReadyUpdate(int _matchNum);
	void SetReady(const CS_READY_PACKET* _packet);
	void SelectSkill(const CS_SKILL_SELECT_PACKET* _packet, std::shared_ptr<CClient> _client);
	void SelectJob(const CS_JOB_SELECT_PACKET* _packet, std::shared_ptr<CClient> _client);
	void SelectStat(const CS_STAT_SELECT_PACKET* _packet, std::shared_ptr<CClient> _client);

	//Loading
	void LoadComplete(const CS_LOAD_COMPLETE_PACKET* _packet, int _id) { m_clientLoading[_id] = true; }
	void LoadingUpdate(int _matchNum);

	//InGame
	void TeleportStart(std::shared_ptr<CClient> _client);
	void UseItem(const CS_USE_ITEM_PACKET* _packet, std::shared_ptr<CClient> _client);
	void InGameUpdate(int _matchNum);

	void Update();
	void Reset();

	template<typename Func>
	void PushJob(Func&& _f) { m_jobQueue.PushJob(std::forward<Func>(_f)); }

	const std::array<int, MAX_PLAYER>& GetClientIds() const { return m_clientid; }

private:
	std::array<int, MAX_PLAYER> m_clientid = { -1, -1, -1, -1 };
	std::array<bool, MAX_PLAYER> m_clientReady = { false, false, false, false };
	std::array<bool, MAX_PLAYER> m_clientLoading = { false, false, false, false };
	ESceneType m_sceneType = ESceneType::ReadyScene;
	JobQueue m_jobQueue{wod::core::JobBudget::Snapshot};

	float m_readyTime = 120.f;
	TimePoint m_updateTime = {};
	TimePoint m_lastGoldUpdateTime = {};
	TimePoint m_lastPlayerHeal = {};
	TimePoint m_lastMinionRespawn = {};
};

}