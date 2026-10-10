#include "pch.h"
#include "CPacketSender.h"
#include "CNetworkMgr.h"

namespace wod_server {
	CPacketSender::CPacketSender()
	{
		m_session = std::make_shared<Session>();
	}

	void CPacketSender::Initailize(const SOCKET& _socket)
	{
	}

	void CPacketSender::SendMovePacket(const int _matchID, const vec3& _pos, char _dir) const
	{
		SC_MOVE_PLAYER_PACKET p;
		p.size = sizeof(p);
		p.type = SC_MOVE_PLAYER;
		p.x = _pos.m_x;
		p.y = _pos.m_y;
		p.z = _pos.m_z;
		p.id = _matchID;
		p.direction = _dir;
		p.move_time = std::chrono::high_resolution_clock::now().time_since_epoch().count() + m_clientTimeDifference;
		m_session->Send(&p);
	}

	void CPacketSender::SendMatchEndPacket(bool _win)
	{
		SC_MATCH_END_PACKET p;
		p.size = sizeof(p);
		p.type = SC_MATCH_END;
		memset(p.lobbyip, 0, INET_ADDRSTRLEN);
		memcpy_s(p.lobbyip, INET_ADDRSTRLEN, network::GetInstance()->m_lobbyIP.c_str(), INET_ADDRSTRLEN);
		p.lobbyport = LOBBY_PORT;
		p.win = _win;

		m_session->Send(&p);
	}

	void CPacketSender::SendAddPlayerPacket(int _id, const vec3& _look, const vec3& _right, const ModelCustomize& _model) const
	{
		SC_ADD_PLAYER_PACKET p;
		p.size = sizeof(SC_ADD_PLAYER_PACKET);
		p.type = SC_ADD_PLAYER;
		p.id = _id;
		p.lookX = _look.m_x; p.lookY = _look.m_y; p.lookZ = _look.m_z;
		p.rightX = _right.m_x; p.rightY = _right.m_y; p.rightZ = _right.m_z;
		p.model = _model;

		m_session->Send(&p);
	}

	void CPacketSender::SendAddNpcPacket(int _id, const vec3& _pos, const vec3& _look, const vec3& _right, NPC_TYPE _npcType) const
	{
		SC_ADD_NPC_PACKET p;
		p.size = sizeof(SC_ADD_NPC_PACKET);
		p.type = SC_ADD_NPC;
		p.npcType = static_cast<int>(_npcType);
		p.id = _id;
		p.x = _pos.m_x;
		p.y = _pos.m_y;
		p.z = _pos.m_z;
		p.lookX = _look.m_x; p.lookY = _look.m_y; p.lookZ = _look.m_z;
		p.rightX = _right.m_x; p.rightY = _right.m_y; p.rightZ = _right.m_z;

		m_session->Send(&p);
	}

	void CPacketSender::SendRotatePacket(int _id, const vec3& _look, const vec3& _right) const
	{
		SC_ROTATE_PLAYER_PACKET p;
		p.size = sizeof(SC_ROTATE_PLAYER_PACKET);
		p.type = SC_ROTATE_PLAYER;
		p.id = _id;
		p.lookX = _look.m_x; p.lookY = _look.m_y; p.lookZ = _look.m_z;
		p.rightX = _right.m_x; p.rightY = _right.m_y; p.rightZ = _right.m_z;
		m_session->Send(&p);
	}

	void CPacketSender::SendSelectSkillPacket(int _id, int _storage, int _skill) const
	{
		SC_SKILL_SELECT_PACKET p;
		p.size = sizeof(SC_SKILL_SELECT_PACKET);
		p.type = SC_SKILL_SELECT;
		p.id = _id;
		p.storage = _storage;
		p.skill = _skill;
		m_session->Send(&p);
	}

	void CPacketSender::SendLoginPacket() const
	{
		SC_LOGIN_INFO_PACKET p;
		p.size = sizeof(SC_LOGIN_INFO_PACKET);
		p.type = SC_LOGIN_INFO;
		p.x = 0;
		p.y = 0;
		p.z = 0;
		p.id = 0;
		m_session->Send(&p);
	}

	void CPacketSender::SendGameTimePacket(int _time, char _type) const
	{
		SC_GAME_TIME_PACKET p;
		p.size = sizeof(SC_GAME_TIME_PACKET);
		p.type = SC_GAME_TIME;
		p.time = _time;
		p.timeType = _type;
		m_session->Send(&p);
	}

	void CPacketSender::SendReadyPacket(int _id, bool _ready) const
	{
		SC_READY_PACKET p;
		p.size = sizeof(SC_READY_PACKET);
		p.type = SC_READY;
		p.id = _id;
		p.ready = _ready;
		m_session->Send(&p);
	}

	void CPacketSender::SendJobSelectPacket(int _id, short _job) const
	{
		SC_JOB_SELECT_PACKET p;
		p.size = sizeof(SC_JOB_SELECT_PACKET);
		p.type = SC_JOB_SELECT;
		p.id = _id;
		p.job = _job;
		m_session->Send(&p);
	}

	void CPacketSender::SendGameStartPacket() const
	{
		SC_GAME_START_PACKET p;
		p.size = sizeof(SC_GAME_START_PACKET);
		p.type = SC_GAME_START;
		m_session->Send(&p);
	}

	void CPacketSender::SendChatPacket(char _name[NAME_SIZE], WCHAR _chat[CHAT_SIZE]) const
	{
		SC_CHAT_PACKET p;
		p.size = sizeof(SC_CHAT_PACKET);
		p.type = SC_CHAT;
		p.chatType = 3;
		wcscpy_s(p.chat, _chat);
		memcpy_s(p.name, NAME_SIZE, _name, NAME_SIZE);
		m_session->Send(&p);
	}

	void CPacketSender::SendModelCustomizePacket(int _id, const ModelCustomize& _model) const
	{
		SC_MODEL_CUSTOMIZE_PACKET p;
		p.size = sizeof(SC_MODEL_CUSTOMIZE_PACKET);
		p.type = SC_MODEL_CUSTOMIZE;
		p.id = _id;
		p.model = _model;
		m_session->Send(&p);
	}

	void CPacketSender::SendMoveNpcPacket(int _id, const vec3& _pos, const vec3& _look, const vec3& _right, NPC_TYPE _npcType, bool _idle) const
	{
		SC_MOVE_NPC_PACKET p;
		p.size = sizeof(SC_MOVE_NPC_PACKET);
		p.type = SC_MOVE_NPC;
		p.npcType = static_cast<int>(_npcType);
		p.id = _id;
		p.x = _pos.m_x; p.y = _pos.m_y; p.z = _pos.m_z;
		p.lookX = _look.m_x; p.lookY = _look.m_y; p.lookZ = _look.m_z;
		p.rightX = _right.m_x; p.rightY = _right.m_y; p.rightZ = _right.m_z;
		p.idle = _idle;
		m_session->Send(&p);
	}

	void CPacketSender::SendTeleportPacket(int _id, bool _finish) const
	{
		SC_TELEPORT_PACKET p;
		p.size = sizeof(SC_TELEPORT_PACKET);
		p.type = SC_TELEPORT;
		p.id = _id;
		p.finish = _finish;
		m_session->Send(&p);
	}

	void CPacketSender::SendSkillPacket(int _id, char _type, int _skillNum, bool _onOff)
	{
		SC_SKILL_PACKET p;
		p.size = sizeof(SC_SKILL_PACKET);
		p.type = SC_SKILL;
		p.skillType = _type;
		p.id = _id;
		p.skillNum = static_cast<short>(_skillNum);
		p.onOff = _onOff;
		m_session->Send(&p);
	}

	void CPacketSender::SendCoolTimePacket(int _cooltime1, int _cooltime2, int _cooltime3, int _cooltime4, int _cooltime5) const
	{
		SC_COOLTIME_PACKET p;
		p.size = sizeof(SC_COOLTIME_PACKET);
		p.type = SC_COOLTIME;

		p.skill1 = _cooltime1;
		p.skill2 = _cooltime2;
		p.skill3 = _cooltime3;
		p.skill4 = _cooltime4;
		p.skill5 = _cooltime5;
		m_session->Send(&p);
	}

	void CPacketSender::SendTowerAttackPacket(int _id, const vec3& _pos, const vec3& _look) const
	{
		SC_TOWER_ATTACK_PACKET p;
		p.size = sizeof(SC_TOWER_ATTACK_PACKET);
		p.type = SC_TOWER_ATTACK;
		p.id = _id;
		p.x = _pos.m_x; p.y = _pos.m_y; p.z = _pos.m_z;
		p.lookX = _look.m_x; p.lookY = _look.m_y; p.lookZ = _look.m_z;
		m_session->Send(&p);
	}

	void CPacketSender::SendTowerAttackRemovePacket(int _id) const
	{
		SC_TOWER_ATTACK_REMOVE_PACKET p;
		p.size = sizeof(SC_TOWER_ATTACK_REMOVE_PACKET);
		p.type = SC_TOWER_ATTACK_REMOVE;
		p.id = _id;
		m_session->Send(&p);
	}

	void CPacketSender::SendTowerAttackAddPacket(int _id, const vec3& _pos) const
	{
		SC_TOWER_ATTACK_ADD_PACKET p;
		p.size = sizeof(SC_TOWER_ATTACK_ADD_PACKET);
		p.type = SC_TOWER_ATTACK_ADD;
		p.id = _id;
		p.x = _pos.m_x; p.y = _pos.m_y; p.z = _pos.m_z;
		m_session->Send(&p);
	}

	void CPacketSender::SendRemoveNpcPacket(int _id, NPC_TYPE _npcType) const
	{
		SC_REMOVE_NPC_PACKET p;
		p.size = sizeof(SC_REMOVE_NPC_PACKET);
		p.type = SC_REMOVE_NPC;
		p.npcType = static_cast<int>(_npcType);
		p.id = _id;
		m_session->Send(&p);
	}

	void CPacketSender::SendNpcStatChangePacket(int _id, int _maxHp, int _curHp) const
	{
		SC_NPC_STAT_CHANGE_PACKET p;
		p.size = sizeof(SC_NPC_STAT_CHANGE_PACKET);
		p.type = SC_NPC_STAT_CHANGE;
		p.id = _id;
		p.maxHp = _maxHp;
		p.curHp = _curHp;
		m_session->Send(&p);
	}

	void CPacketSender::SendNpcAttackPacket(int _id) const
	{
		SC_NPC_ATTACK_PACKET p;
		p.size = sizeof(SC_NPC_ATTACK_PACKET);
		p.type = SC_NPC_ATTACK;
		p.id = static_cast<short>(_id);
		m_session->Send(&p);
	}

	void CPacketSender::SendGoldPacket(int _matchID, int _gold)
	{
		SC_GIVE_GOLD_PACKET p;
		p.size = sizeof(SC_GIVE_GOLD_PACKET);
		p.type = SC_GIVE_GOLD;
		p.id = static_cast<short>(_matchID);
		p.gold = _gold;
		m_session->Send(&p);
	}

	void CPacketSender::SendPlayerStatChangePacket(int _id, const CStat& _statValue) const
	{
		SC_PLAYER_STAT_CHANGE_PACKET p;
		p.size = sizeof(SC_PLAYER_STAT_CHANGE_PACKET);
		p.type = SC_PLAYER_STAT_CHANGE;
		p.id = _id;
		p.speed = _statValue.m_speed;
		m_session->Send(&p);
	}

	void CPacketSender::SendPlayerStatusChangePakcet(int _id, char _statusType, short _statusNum)
	{
		SC_PLAYER_STATUS_CHANGE_PACKET p;
		p.size = sizeof(SC_PLAYER_STATUS_CHANGE_PACKET);
		p.id = _id;
		p.type = SC_PLAYER_STATUS_CHANGE;
		p.statusType = _statusType;
		p.statusNum = _statusNum;
		m_session->Send(&p);
	}

	void CPacketSender::SendPlayerHealthManaPacket(int _id, CHealthMana& _healthMana) const
	{
		SC_HEALTH_MANA_PACKET p;
		p.size = sizeof(SC_HEALTH_MANA_PACKET);
		p.type = SC_HEALTH_MANA;
		p.id = _id;
		p.maxHp = _healthMana.GetMaxHp();
		p.curHp = _healthMana.GetCurHp();
		p.maxMp = _healthMana.GetMaxMp();
		p.curMp = _healthMana.GetCurMp();
		m_session->Send(&p);
	}

	void CPacketSender::SendSkillFinishPacket(int _id) const
	{
		SC_SKILL_FINISH_PACKET p;
		p.size = sizeof(SC_SKILL_FINISH_PACKET);
		p.type = SC_SKILL_FINISH;
		p.id = static_cast<short>(_id);
		m_session->Send(&p);
	}

	void CPacketSender::SendPlayerRespawnPacket(int _id, bool _respawn) const
	{
		SC_PLAYER_RESPAWN_PACKET p;
		p.size = sizeof(SC_PLAYER_RESPAWN_PACKET);
		p.type = SC_PLAYER_RESPWAN;
		p.id = _id;
		p.respawn = _respawn;
		m_session->Send(&p);
	}

	void CPacketSender::SendStructureStatChangePacket(int _id, int _maxHp, int _curHp) const
	{
		SC_STRUCTURE_STAT_CHANGE_PACKET p;
		p.size = sizeof(SC_STRUCTURE_STAT_CHANGE_PACKET);
		p.type = SC_STRUCTURE_STAT_CHANGE;
		p.id = _id;
		p.maxHp = _maxHp;
		p.curHp = _curHp;
		m_session->Send(&p);
	}

	void CPacketSender::SendStructureStatusChangePacket(int _id, bool _broken) const
	{
		SC_STRUCTURE_STATUS_CHANGE_PACKET p;
		p.size = sizeof(SC_STRUCTURE_STATUS_CHANGE_PACKET);
		p.type = SC_STRUCTURE_STATUS_CHANGE;
		p.id = _id;
		p.broken = _broken;
		m_session->Send(&p);
	}

	void CPacketSender::SendMagicEyePacket(bool _show) const
	{
		SC_MAGIC_EYE_POS_PACKET p;
		p.size = sizeof(p);
		p.type = SC_MAGIC_EYE_POS;
		p.show = _show;
		m_session->Send(&p);
	}

	void CPacketSender::SendTeleportActivePacket(bool _active) const
	{
		SC_TELEPORT_ACTIVE_PACKET p;
		p.size = sizeof(p);
		p.type = SC_TELEPORT_ACTIVE;
		p.active = _active;
		m_session->Send(&p);
	}

	void CPacketSender::SendGameOverPacket(bool _nexusDestroy) const
	{
		SC_GAME_OVER_PACKET p;
		p.size = sizeof(p);
		p.type = SC_GAME_OVER;
		p.nexusDestroy = _nexusDestroy;
		m_session->Send(&p);
	}

	void CPacketSender::SendMonsterKillBuffPacket(char _monsterType, int _id) const
	{
		SC_MONSTER_KILL_BUFF_PACKET p;
		p.size = sizeof(p);
		p.type = SC_MONSTER_KILL_BUFF;
		p.monsterType = _monsterType;
		p.id = _id;
		m_session->Send(&p);
	}

	void CPacketSender::SendJumpFinishPacket() const
	{
		SC_JUMP_FINISH_PACKET p;
		p.size = sizeof(p);
		p.type = SC_JUMP_FINISH;
		m_session->Send(&p);
	}

	void CPacketSender::SendRTTPacket() const
	{
		SC_RTT_PACKET p;
		p.size = sizeof(p);
		p.type = SC_RTT;
		p.time = std::chrono::high_resolution_clock::now().time_since_epoch().count();
		m_session->Send(&p);
	}

	void CPacketSender::SendAddSkillObjectPacket(int _id, SKILL_TYPE _type, const vec3& _pos, const vec3& _look)
	{
		SC_ADD_SKILL_OBJECT_PACKET p;
		p.size = sizeof(SC_ADD_SKILL_OBJECT_PACKET);
		p.type = SC_ADD_SKILL_OBJECT;
		p.id = _id;
		p.objectType = static_cast<short>(_type);
		p.x = _pos.m_x; p.y = _pos.m_y; p.z = _pos.m_z;
		p.lookX = _look.m_x; p.lookY = _look.m_y; p.lookZ = _look.m_z;
		m_session->Send(&p);
	}

	void CPacketSender::SendUpdateSkillObjectPacket(int _id, SKILL_TYPE _objectType, const vec3& _pos)
	{
		SC_UPDATE_SKILL_OBJECT_PACKET p;
		p.size = sizeof(SC_UPDATE_SKILL_OBJECT_PACKET);
		p.type = SC_UPDATE_SKILL_OBJECT;
		p.objectType = static_cast<short>(_objectType);
		p.id = _id;
		p.x = _pos.m_x; p.y = _pos.m_y; p.z = _pos.m_z;
		m_session->Send(&p);
	}

	void CPacketSender::SendRemoveSkillObjectPacket(int _id, SKILL_TYPE _objectType)
	{
		SC_REMOVE_SKILL_OBJECT_PACKET p;
		p.size = sizeof(SC_REMOVE_SKILL_OBJECT_PACKET);
		p.type = SC_REMOVE_SKILL_OBJECT;
		p.id = _id;
		p.objectType = static_cast<short>(_objectType);
		m_session->Send(&p);
	}

	void CPacketSender::SendDummyLoginInfoPacket(int _id, const vec3& _pos)
	{
		SC_DUMMY_LOGIN_INFO_PACKET p;
		p.size = sizeof(p);
		p.type = SC_DUMMY_LOGIN_INFO;
		p.id = _id;
		p.x = _pos.m_x;
		p.y = _pos.m_y;
		p.z = _pos.m_z;
		m_session->Send(&p);
	}
}