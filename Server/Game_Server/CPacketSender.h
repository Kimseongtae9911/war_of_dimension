#pragma once
#include "TCPSocket.h"
#include "CHealthMana.h"

namespace wod_server {
	class CPacketSender
	{
	public:
		CPacketSender();
		~CPacketSender() {}

		void Initailize(const SOCKET& socket);
		void Reset();

		std::shared_ptr<Session> GetSession() const { return m_session; }
		void SetTimeDifference(const long long& time) { m_clientTimeDifference = time; }

	public:
		void SendMovePacket(const int matchID, const vec3& pos, char dir) const;
		void SendMatchEndPacket(bool win);
		void SendAddPlayerPacket(int id, const vec3& look, const vec3& right, const ModelCustomize& model) const;
		void SendAddNpcPacket(int id, const vec3& pos, const vec3& look, const vec3& right, NPC_TYPE npcType = NPC_TYPE::MINION) const;
		void SendRotatePacket(int id, const vec3& look, const vec3& right) const;
		void SendSelectSkillPacket(int id, int storage, int skill) const;
		void SendLoginPacket() const;
		void SendGameTimePacket(int time, char type) const;
		void SendReadyPacket(int id, bool ready) const;
		void SendJobSelectPacket(int id, short job) const;
		void SendGameStartPacket() const;
		void SendChatPacket(char name[NAME_SIZE], WCHAR chat[CHAT_SIZE]) const;
		void SendModelCustomizePacket(int id, const ModelCustomize& model) const;
		void SendMoveNpcPacket(int id, const vec3& pos, const vec3& look, const vec3& right, NPC_TYPE npcType = NPC_TYPE::MINION, bool idle = false) const;
		void SendTeleportPacket(int id, bool finish) const;
		void SendSkillPacket(int id, char type, int skillNum, bool onOff = false);
		void SendCoolTimePacket(int cooltime1, int cooltime2, int cooltime3, int cooltime4, int cooltime5) const;
		void SendTowerAttackPacket(int id, const vec3& pos, const vec3& look) const;
		void SendTowerAttackRemovePacket(int id) const;
		void SendTowerAttackAddPacket(int id, const vec3& pos) const;
		void SendRemoveNpcPacket(int id, NPC_TYPE npcType = NPC_TYPE::MINION) const;
		void SendNpcStatChangePacket(int id, int maxHp, int curHp) const;
		void SendNpcAttackPacket(int id) const;
		void SendGoldPacket(int matchID, int gold);
		void SendPlayerStatChangePacket(int id, const CStat& stat) const;
		void SendPlayerStatusChangePakcet(int id, char statusType, short statusNum);
		void SendPlayerHealthManaPacket(int id, CHealthMana& healthMana) const;
		void SendSkillFinishPacket(int id) const;
		void SendPlayerRespawnPacket(int id, bool respawn) const;
		void SendStructureStatChangePacket(int id, int maxHp, int curHp) const;
		void SendStructureStatusChangePacket(int id, bool broken = false) const;
		void SendMagicEyePacket(bool show) const;
		void SendTeleportActivePacket(bool active) const;
		void SendGameOverPacket(bool nexusDestroy) const;
		void SendMonsterKillBuffPacket(char monsterType, int id) const;
		void SendJumpFinishPacket() const;
		void SendRTTPacket() const;

		void SendAddSkillObjectPacket(int id, SKILL_TYPE type, const vec3& pos, const vec3& look);
		void SendUpdateSkillObjectPacket(int id, SKILL_TYPE type, const vec3& pos);
		void SendRemoveSkillObjectPacket(int id, SKILL_TYPE type);

		void SendDummyLoginInfoPacket(int id, const vec3& pos);

	private:
		std::shared_ptr<Session> m_session;

		long long m_clientTimeDifference = 0;
	};
}