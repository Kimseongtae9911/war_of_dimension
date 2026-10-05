#pragma once
#include "Object.h"
#include "CSkillModel.h"
#include "WizardAttack.h"
#include "MagicMissile.h"
#include "EnergyBall.h"
#include "BigBang.h"
#include "AuraBlade.h"
#include "JudgementSword.h"
#include "ArcherAttack.h"
#include "ArcherObject.h"
#include "DarknessRay.h"
#include "ProtectedArea.h"
#include "FireBall.h"
#include "PenetraitingShot.h"
#include "PhoenixArrow.h"
#include "StickyArrow.h"
#include "ReturnZero.h"
#include "SCL.h"
#include "ProAttack.h"
#include "HelloWorld.h"
#include "RockThrow.h"
#include "DimensionCrush.h"
#include "MultipleShot.h"
#include <mutex>
#include <optional>
#include "ParticleSelection.h"

class OverlapEx;
class CGameObject;
class CPlayer;
class CTowerAttack;
class CMinion;
class CMonster;
class CSkillObject;

struct ObjectInfo
{
	float prevX, prevY, prevZ;
	float x, y, z;
	float lookX, lookY, lookZ;
	float rightX, rightY, rightZ;
	int animation; //0:Idle, 1:Forward, 2:Back, 3:Left, 4:Right
	long long lastPacketTime;
	long long recvPacketTime;
	array<chrono::system_clock::time_point, 5> lastSkillTime = {};

	SKILLKIND playerSkill = SKILLKIND::NONE;
	atomic_bool skillUsed = false;

	volatile bool show;
	bool dissolve;

	void Initialize() {
		prevX = 0.f, prevY = FLOOR_HEIGHT, prevZ = 0.f;
		x = 0.f; y = FLOOR_HEIGHT; z = 0.f;
		lookX = 1.f; lookY = 0.f; lookZ = 0.f;
		rightX = 0.f; rightY = 0.f; rightZ = 1.f;
		animation = 0;
		show = false;
		dissolve = false;
		skillUsed = false;
		playerSkill = SKILLKIND::NONE;
		lastPacketTime = chrono::system_clock::now().time_since_epoch().count();
	}	
};

struct NpcInfo
{
	float x, y, z;
	float lookX, lookY, lookZ;
	float rightX, rightY, rightZ;
	int animation;

	volatile bool show;
	bool dissolve;

	void Initialize() {
		x = 0.f; y = FLOOR_HEIGHT; z = 0.f;
		lookX = 0.f; lookY = 0.f; lookZ = 1.f;
		rightX = 1.f; rightY = 0.f; rightZ = 0.f;
		animation = 0;
		show = false;
		dissolve = false;
	}
};

struct StructureInfo 
{
	volatile bool active;
	bool broken = false;
	int maxHp;
	int curHp;
};

struct LobbySceneInfo {
	int shopType = 0;
	int partNum = 0;
	std::array<vector<pair<int, int>>, 27> customizeDatas;
	vector<AuctionInfo> auctionPageInfos;
	int auctionPartNum = 0;
};

struct ReadySceneInfo
{
	array<array<int, 4>, INGAME_PLAYER> selectSkills;	//[First Index: Player ID][Second Index: 스킬인덱스] = {value = skillNum}
	//Player First SkillNum = 48, Boss First SkillNum = 21 / Need to subract max skill num(48, 21) and add skill number(.txtfile)
	array<int, INGAME_PLAYER> playerJobs;
	array<bool, INGAME_PLAYER> playerReadys;
	int time = 60;

	void Initialize() {
		for (int i = 0; i < selectSkills.size(); ++i) {
			for (int j = 0; j < selectSkills[i].size(); ++j) {
				if (i == 3) {
					//selectSkills[i][j] = BOSS_SKILL / 2;
					selectSkills[i][j] = 0;
				}
				else {
					//selectSkills[i][j] = PLAYER_SKILL / 4;
					selectSkills[i][j] = 0;
				}
			}
		}
		for (int i = 0; i < INGAME_PLAYER; ++i) {
			playerJobs[i] = 0;
			if (i == 3) {
				playerJobs[3] = 4;
			}
			playerReadys[i] = false;
		}
		time = 60;
	}
};

struct GameSceneInfo
{
	int time;

	void Initialize() {
		time = 0;
	}
};

struct SkillObjectInfo
{
	SkillObjectInfo() {
		pos = { 0.f, 0.f, 0.f };
		look = { 1.f, 0.f, 0.f };
		show = false;
	}
	SkillObjectInfo(const SkillObjectInfo& obj) {
		pos = obj.pos;
		look = obj.look;
		show = false;
	}

	atomic_bool show = false;
	XMFLOAT3 pos = { 0.f, 0.f, 0.f };
	XMFLOAT3 look = { 1.f, 0.f, 0.f };
};

// 좌표, 방향벡터, 애니메이션 트랙번호(int)
// 옷정보, 상태이상

class NetworkManager
{
	SINGLETON(NetworkManager);
	friend struct HeroSelectionTestAccess;

public:
	void Initialize(string ip);
	void Release();
	void Disconnect();
	void Reset();

	void SendPacket(BASE_PACKET* packet) const;
	void SendSkillSelectPacket(int storage, int skill) const;
	void SendMatchPacket(char role, bool match) const;
	void SendReadyPacket(bool ready) const;
	void SendJobSelectPacket(int job) const;
	void SendChatPacket(int id, WCHAR chatBuf[256], char name[NAME_SIZE], CHAT option) const;
	void SendLoginPacket(char name[NAME_SIZE], char password[NAME_SIZE] = NULL);
	void SendRotatePacket(const XMFLOAT3& look, const XMFLOAT3& right) const;
	void SendSkillFinishPacket() const;
	void SendModelCustomizePacket(const ModelCustomize& model) const;
	void SendSkillPacket(SKILLKIND skill, bool onOff = false) const;
	void SendLoadCompletePacket() const;
	void SendJumpPacket() const;
	void SendTeleportPacket() const;
	void SendTowerActivatePacket(int num) const;
	void SendMinionPathPacket(int path) const;
	void SendNpcAttackFinishPacket(int id) const;
	void SendShopPacket() const;
	void SendSignUpPacket(char name[NAME_SIZE], char password[NAME_SIZE]) const;
	void SendRegisterAuctionPacket(char name[NAME_SIZE], SHOP_TYPE type, short customizeNum, int buyTokenNum) const;
	void SendChangeChannelPacket(int channel);
	void SendPortNumPacket();
	void SendStakeTokenPacket(int tokenNum, unsigned short stakeDays);
	void SendChangeNodePacket(bool fullNode);
	void SendStatSelectPacket(const Additional_Stats& stats);
	void SendBuyItemPacket(ITEMKIND type);
	void SendBuyStatPacket(ITEMKIND type);
	void SendUseItemPacket(int num);
	void SendCreateTransactionPacket() const;
	void SendOpenAuctionPacket() const;
	void SendOpenBlockChainPacket() const;
	void SendOpenCustomizePacket() const;
	void SendGetAuctionInfoPacket(int pageNum) const;
	void SendBuyAuctionPacket(char sellerName[NAME_SIZE], short customizeType, short customizeNum, unsigned short buyPrice) const;
	void SendDebugGoldPacket();
	void SendLoginCompletePacket();

	const void SendTestIngamePacket(char name[NAME_SIZE]) const;
	const void SendTestIngamePacket() const;

	void RecvPacket();

	void WorkerThread();

	void SetId(int id) { m_id = id; }
	int GetId() const { return m_id; }
	int GetChannel() const { return m_channel; }
	int GetTokenNum() const { return m_tokenNum; }

	void TestReady(bool type = false);
	void StoreIngameAppearance(int slot, const ModelCustomize& appearance);
	void FreezeIngameAppearances();
	std::optional<std::array<ModelCustomize, 3>> GetFrozenIngameAppearances() const;
	std::optional<ModelCustomize> GetFrozenIngameAppearance(int slot) const;
	void SeedTestIngameAppearances();
	void StoreReadySkill(int player, int slot, int skill);
	void StoreReadyJob(int player, int job);
	void FreezeIngameSkills();
	std::optional<IngameSkillLoadout> GetFrozenIngameSkills() const;
	IngameSkillLoadout GetIngameSkillLoadout() const;

public:
	SCENEKIND playerScene = SCENEKIND::NONE;
	float lerpPercentage = 0.16f;

	//Clients
	std::array<CGameObject*, LOBBY_MAX_CLIENT> OtherClients = {};
	CPlayer* myClient = NULL;
	std::array<ModelCustomize, LOBBY_MAX_CLIENT> m_ArrayOtherClientCustom;
	std::array<ModelCustomize, INGAME_PLAYER> m_ArrayInGameClientsCustom;
	ObjectInfo* myInfo;
	ObjectInfo* otherClientsInfo;
	char clientPassword[NAME_SIZE];

	//Lobby
	LobbySceneInfo* lobbySceneInfo;
	int channelNum = -1;

	//Ready
	ReadySceneInfo* readySceneInfo;

	//InGame
	GameSceneInfo* gameSceneInfo;
	NpcInfo* npcInfo;
	NpcInfo* monsterInfo;

	unordered_map<SKILL_TYPE, std::vector<SkillObjectInfo>> skillObjectInfos;
	atomic_bool skillUsed = false;
	array<int, 5> skillCoolTime = {};
	SKILLKIND playerSkill = SKILLKIND::NONE;

	StructureInfo* structureInfo;
	bool magneticFenceActive = true;
	bool teleportActive = false;	

	list<SERVERCHAT> ListChating;

	std::array<CTowerAttack*, PATH_NUM> towerAttacks = {};
	std::array<CMinion*, MAX_MINION> minions = {};
	std::array<CMonster*, MONSTER_NUM> monsters = {};

private:
	void Recv(int id, int bytes, OverlapEx* over_ex);
	void Send(int id, int bytes, OverlapEx* over_ex);
	void Disconnect(int id, int bytes, OverlapEx* over_ex);
	void Connect(int id, int bytes, OverlapEx* over_ex);

	void LoginInfoPacket(int id, BASE_PACKET* packet);
	void LoginFailPacket(int id, BASE_PACKET* packet);
	void AddPlayerPacket(int id, BASE_PACKET* packet);
	void RemovePlayerPacket(int id, BASE_PACKET* packet);
	void MovePacket(int id, BASE_PACKET* packet);
	void MatchPacket(int id, BASE_PACKET* packet);
	void MatchEndPacket(int id, BASE_PACKET* packet);
	void ChatPacket(int id, BASE_PACKET* packet);
	void RotatePacket(int id, BASE_PACKET* packet);
	void SkillPacket(int id, BASE_PACKET* packet);
	void CoolTimePacket(int id, BASE_PACKET* packet);
	void AddNpcPacket(int id, BASE_PACKET* packet);
	void SkillSelectPacket(int id, BASE_PACKET* packet);
	void GameTimePacket(int id, BASE_PACKET* packet);
	void ReadyPacket(int id, BASE_PACKET* packet);
	void JobSelectPacket(int id, BASE_PACKET* packet);
	void GameStartPacket(int id, BASE_PACKET* packet);
	void CustomizePacket(int id, BASE_PACKET* packet);	
	void ChangeServerPacket(int id, BASE_PACKET* packet);
	void MoveNpcPacket(int id, BASE_PACKET* packet);
	void TeleportPacket(int id, BASE_PACKET* packet);
	void TowerAttackPacket(int id, BASE_PACKET* packet);
	void TowerAttackRemovePacket(int id, BASE_PACKET* packet);
	void TowerAttackAddPacket(int id, BASE_PACKET* packet);
	void RemoveNpcPacket(int id, BASE_PACKET* packet);
	void NpcStatChangePacket(int id, BASE_PACKET* packet);
	void AddSkillObjectPacket(int id, BASE_PACKET* packet);
	void UpdateSkillObjectPacket(int id, BASE_PACKET* packet);
	void RemoveSkillObjectPacket(int id, BASE_PACKET* packet);
	void NpcAttackPacket(int id, BASE_PACKET* packet);
	void GiveGoldPacket(int id, BASE_PACKET* packet);
	void PlayerStatChangePacket(int id, BASE_PACKET* packet);
	void PlayerStatusChangePacket(int id, BASE_PACKET* packet);
	void HealthManaPacket(int id, BASE_PACKET* packet);
	void SkillFinishPacket(int id, BASE_PACKET* packet);
	void ShopPacket(int id, BASE_PACKET* packet);
	void AuctionInfoPacket(int id, BASE_PACKET* packet);
	void PlayerRespawnPacket(int id, BASE_PACKET* packet);
	void StructureStatChangePacket(int id, BASE_PACKET* packet);
	void StructureStatusChangePacket(int id, BASE_PACKET* packet);
	void ChangeChannelPacket(int id, BASE_PACKET* packet);
	void PeerInfoPacket(int id, BASE_PACKET* packet);
	void TokenNumPacket(int id, BASE_PACKET* packet);
	void ChangeNodePacket(int id, BASE_PACKET* packet);
	void StakeTokenPacket(int id, BASE_PACKET* packet);
	void TransactionPacket(int id, BASE_PACKET* packet);
	void MagicEyePacket(int id, BASE_PACKET* packet);
	void BlockHeaderPacket(int id, BASE_PACKET* packet);
	void BlockBodyPacket(int id, BASE_PACKET* packet);
	void FullNodePacket(int id, BASE_PACKET* packet);
	void TeleportActivePacket(int id, BASE_PACKET* packet);
	void GameOverPacket(int id, BASE_PACKET* packet);
	void MonsterKillBuffPacket(int id, BASE_PACKET* packet);
	void JumpFinishPacket(int id, BASE_PACKET* packet);
	void CustomizePartsPacket(int id, BASE_PACKET* packet);
	void AuctionPartsNumPacket(int id, BASE_PACKET* packet);
	void StakedTokenInfoPacket(int id, BASE_PACKET* packet);
	void RTTPacket(int id, BASE_PACKET* packet);

private:
	SOCKET m_socket;
	mutable std::mutex m_appearanceMutex;
	mutable std::mutex m_skillSelectionMutex;
	std::optional<IngameSkillLoadout> m_frozenSkills;
	std::array<bool, 3> m_appearanceReceived{};
	std::optional<std::array<ModelCustomize, 3>> m_frozenAppearances;
	HANDLE m_iocp;
	OverlapEx* m_over;
	int m_remainData = 0;
	int m_id = -1;
	int m_channel = 0;
	int m_tokenNum = 0;

	std::unordered_map<OP_TYPE, std::function<void(int, int, OverlapEx*)>> m_iocpfunc;
	std::unordered_map<char, std::function<void(BASE_PACKET*, int)>> m_packetfunc;

	//P2P
	int m_portNum;
	string m_blockHeader;
	string m_blockBody;
};

