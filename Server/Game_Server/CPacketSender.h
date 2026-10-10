#pragma once
#include "TCPSocket.h"
#include "CHealthMana.h"

namespace wod_server {
	class CPacketSender
	{
	public:
		CPacketSender();
		~CPacketSender() {}

		void Initailize(const SOCKET& _socket);
		void Reset();

		std::shared_ptr<Session> GetSession() const { return m_session; }
		void SetTimeDifference(const long long& _time) { m_clientTimeDifference = _time; }

	public:
		void SendMovePacket(const int _matchID, const vec3& _pos, char _dir) const;
		void SendMatchEndPacket(bool _win);
		void SendAddPlayerPacket(int _id, const vec3& _look, const vec3& _right, const ModelCustomize& _model) const;
		void SendAddNpcPacket(int _id, const vec3& _pos, const vec3& _look, const vec3& _right, NPC_TYPE _npcType = NPC_TYPE::MINION) const;
		void SendRotatePacket(int _id, const vec3& _look, const vec3& _right) const;
		void SendSelectSkillPacket(int _id, int _storage, int _skill) const;
		void SendLoginPacket() const;
		void SendGameTimePacket(int _time, char _type) const;
		void SendReadyPacket(int _id, bool _ready) const;
		void SendJobSelectPacket(int _id, short _job) const;
		void SendGameStartPacket() const;
		void SendChatPacket(char _name[NAME_SIZE], WCHAR _chat[CHAT_SIZE]) const;
		void SendModelCustomizePacket(int _id, const ModelCustomize& _model) const;
		void SendMoveNpcPacket(int _id, const vec3& _pos, const vec3& _look, const vec3& _right, NPC_TYPE _npcType = NPC_TYPE::MINION, bool _idle = false) const;
		void SendTeleportPacket(int _id, bool _finish) const;
		void SendSkillPacket(int _id, char _type, int _skillNum, bool _onOff = false);
		void SendCoolTimePacket(int _cooltime1, int _cooltime2, int _cooltime3, int _cooltime4, int _cooltime5) const;
		void SendTowerAttackPacket(int _id, const vec3& _pos, const vec3& _look) const;
		void SendTowerAttackRemovePacket(int _id) const;
		void SendTowerAttackAddPacket(int _id, const vec3& _pos) const;
		void SendRemoveNpcPacket(int _id, NPC_TYPE _npcType = NPC_TYPE::MINION) const;
		void SendNpcStatChangePacket(int _id, int _maxHp, int _curHp) const;
		void SendNpcAttackPacket(int _id) const;
		void SendGoldPacket(int _matchID, int _gold);
		void SendPlayerStatChangePacket(int _id, const CStat& _statValue) const;
		void SendPlayerStatusChangePakcet(int _id, char _statusType, short _statusNum);
		void SendPlayerHealthManaPacket(int _id, CHealthMana& _healthMana) const;
		void SendSkillFinishPacket(int _id) const;
		void SendPlayerRespawnPacket(int _id, bool _respawn) const;
		void SendStructureStatChangePacket(int _id, int _maxHp, int _curHp) const;
		void SendStructureStatusChangePacket(int _id, bool _broken = false) const;
		void SendMagicEyePacket(bool _show) const;
		void SendTeleportActivePacket(bool _active) const;
		void SendGameOverPacket(bool _nexusDestroy) const;
		void SendMonsterKillBuffPacket(char _monsterType, int _id) const;
		void SendJumpFinishPacket() const;
		void SendRTTPacket() const;

		void SendAddSkillObjectPacket(int _id, SKILL_TYPE _type, const vec3& _pos, const vec3& _look);
		void SendUpdateSkillObjectPacket(int _id, SKILL_TYPE _type, const vec3& _pos);
		void SendRemoveSkillObjectPacket(int _id, SKILL_TYPE _type);

		void SendDummyLoginInfoPacket(int _id, const vec3& _pos);

	private:
		std::shared_ptr<Session> m_session;

		long long m_clientTimeDifference = 0;
	};
}