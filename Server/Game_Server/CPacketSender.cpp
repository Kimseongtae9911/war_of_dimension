#include "pch.h"
#include "CPacketSender.h"
#include "CNetworkMgr.h"

namespace wod_server {
	CPacketSender::CPacketSender()
	{
		m_session = std::make_shared<Session>();
	}

	void CPacketSender::Initailize(const SOCKET& socket)
	{
	}

	void CPacketSender::SendMovePacket(const int matchID, const vec3& pos, char dir) const
	{
		SC_MOVE_PLAYER_PACKET p;
		p.size = sizeof(p);
		p.type = SC_MOVE_PLAYER;
		p.x = pos.x;
		p.y = pos.y;
		p.z = pos.z;
		p.id = matchID;
		p.direction = dir;
		p.move_time = std::chrono::high_resolution_clock::now().time_since_epoch().count() + m_clientTimeDifference;
		m_session->Send(&p);
	}

	void CPacketSender::SendMatchEndPacket(bool win)
	{
		SC_MATCH_END_PACKET p;
		p.size = sizeof(p);
		p.type = SC_MATCH_END;
		memset(p.lobbyip, 0, INET_ADDRSTRLEN);
		memcpy_s(p.lobbyip, INET_ADDRSTRLEN, network::GetInstance()->lobbyIP.c_str(), INET_ADDRSTRLEN);
		p.lobbyport = LOBBY_PORT;
		p.win = win;

		m_session->Send(&p);
	}

	void CPacketSender::SendAddPlayerPacket(int id, const vec3& look, const vec3& right, const ModelCustomize& model) const
	{
		SC_ADD_PLAYER_PACKET p;
		p.size = sizeof(SC_ADD_PLAYER_PACKET);
		p.type = SC_ADD_PLAYER;
		p.id = id;
		p.lookX = look.x; p.lookY = look.y; p.lookZ = look.z;
		p.rightX = right.x; p.rightY = right.y; p.rightZ = right.z;
		p.model = model;

		m_session->Send(&p);
	}

	void CPacketSender::SendAddNpcPacket(int id, const vec3& pos, const vec3& look, const vec3& right, NPC_TYPE npcType) const
	{
		SC_ADD_NPC_PACKET p;
		p.size = sizeof(SC_ADD_NPC_PACKET);
		p.type = SC_ADD_NPC;
		p.npcType = static_cast<int>(npcType);
		p.id = id;
		p.x = pos.x;
		p.y = pos.y;
		p.z = pos.z;
		p.lookX = look.x; p.lookY = look.y; p.lookZ = look.z;
		p.rightX = right.x; p.rightY = right.y; p.rightZ = right.z;

		m_session->Send(&p);
	}

	void CPacketSender::SendRotatePacket(int id, const vec3& look, const vec3& right) const
	{
		SC_ROTATE_PLAYER_PACKET p;
		p.size = sizeof(SC_ROTATE_PLAYER_PACKET);
		p.type = SC_ROTATE_PLAYER;
		p.id = id;
		p.lookX = look.x; p.lookY = look.y; p.lookZ = look.z;
		p.rightX = right.x; p.rightY = right.y; p.rightZ = right.z;
		m_session->Send(&p);
	}

	void CPacketSender::SendSelectSkillPacket(int id, int storage, int skill) const
	{
		SC_SKILL_SELECT_PACKET p;
		p.size = sizeof(SC_SKILL_SELECT_PACKET);
		p.type = SC_SKILL_SELECT;
		p.id = id;
		p.storage = storage;
		p.skill = skill;
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

	void CPacketSender::SendGameTimePacket(int time, char type) const
	{
		SC_GAME_TIME_PACKET p;
		p.size = sizeof(SC_GAME_TIME_PACKET);
		p.type = SC_GAME_TIME;
		p.time = time;
		p.timeType = type;
		m_session->Send(&p);
	}

	void CPacketSender::SendReadyPacket(int id, bool ready) const
	{
		SC_READY_PACKET p;
		p.size = sizeof(SC_READY_PACKET);
		p.type = SC_READY;
		p.id = id;
		p.ready = ready;
		m_session->Send(&p);
	}

	void CPacketSender::SendJobSelectPacket(int id, short job) const
	{
		SC_JOB_SELECT_PACKET p;
		p.size = sizeof(SC_JOB_SELECT_PACKET);
		p.type = SC_JOB_SELECT;
		p.id = id;
		p.job = job;
		m_session->Send(&p);
	}

	void CPacketSender::SendGameStartPacket() const
	{
		SC_GAME_START_PACKET p;
		p.size = sizeof(SC_GAME_START_PACKET);
		p.type = SC_GAME_START;
		m_session->Send(&p);
	}

	void CPacketSender::SendChatPacket(char name[NAME_SIZE], WCHAR chat[CHAT_SIZE]) const
	{
		SC_CHAT_PACKET p;
		p.size = sizeof(SC_CHAT_PACKET);
		p.type = SC_CHAT;
		p.chatType = 3;
		wcscpy_s(p.chat, chat);
		memcpy_s(p.name, NAME_SIZE, name, NAME_SIZE);
		m_session->Send(&p);
	}

	void CPacketSender::SendModelCustomizePacket(int id, const ModelCustomize& model) const
	{
		SC_MODEL_CUSTOMIZE_PACKET p;
		p.size = sizeof(SC_MODEL_CUSTOMIZE_PACKET);
		p.type = SC_MODEL_CUSTOMIZE;
		p.id = id;
		p.model = model;
		m_session->Send(&p);
	}

	void CPacketSender::SendMoveNpcPacket(int id, const vec3& pos, const vec3& look, const vec3& right, NPC_TYPE npcType, bool idle) const
	{
		SC_MOVE_NPC_PACKET p;
		p.size = sizeof(SC_MOVE_NPC_PACKET);
		p.type = SC_MOVE_NPC;
		p.npcType = static_cast<int>(npcType);
		p.id = id;
		p.x = pos.x; p.y = pos.y; p.z = pos.z;
		p.lookX = look.x; p.lookY = look.y; p.lookZ = look.z;
		p.rightX = right.x; p.rightY = right.y; p.rightZ = right.z;
		p.idle = idle;
		m_session->Send(&p);
	}

	void CPacketSender::SendTeleportPacket(int id, bool finish) const
	{
		SC_TELEPORT_PACKET p;
		p.size = sizeof(SC_TELEPORT_PACKET);
		p.type = SC_TELEPORT;
		p.id = id;
		p.finish = finish;
		m_session->Send(&p);
	}

	void CPacketSender::SendSkillPacket(int id, char type, int skillNum, bool onOff)
	{
		SC_SKILL_PACKET p;
		p.size = sizeof(SC_SKILL_PACKET);
		p.type = SC_SKILL;
		p.skillType = type;
		p.id = id;
		p.skillNum = static_cast<short>(skillNum);
		p.onOff = onOff;
		m_session->Send(&p);
	}

	void CPacketSender::SendCoolTimePacket(int cooltime1, int cooltime2, int cooltime3, int cooltime4, int cooltime5) const
	{
		SC_COOLTIME_PACKET p;
		p.size = sizeof(SC_COOLTIME_PACKET);
		p.type = SC_COOLTIME;

		p.skill1 = cooltime1;
		p.skill2 = cooltime2;
		p.skill3 = cooltime3;
		p.skill4 = cooltime4;
		p.skill5 = cooltime5;
		m_session->Send(&p);
	}

	void CPacketSender::SendTowerAttackPacket(int id, const vec3& pos, const vec3& look) const
	{
		SC_TOWER_ATTACK_PACKET p;
		p.size = sizeof(SC_TOWER_ATTACK_PACKET);
		p.type = SC_TOWER_ATTACK;
		p.id = id;
		p.x = pos.x; p.y = pos.y; p.z = pos.z;
		p.lookX = look.x; p.lookY = look.y; p.lookZ = look.z;
		m_session->Send(&p);
	}

	void CPacketSender::SendTowerAttackRemovePacket(int id) const
	{
		SC_TOWER_ATTACK_REMOVE_PACKET p;
		p.size = sizeof(SC_TOWER_ATTACK_REMOVE_PACKET);
		p.type = SC_TOWER_ATTACK_REMOVE;
		p.id = id;
		m_session->Send(&p);
	}

	void CPacketSender::SendTowerAttackAddPacket(int id, const vec3& pos) const
	{
		SC_TOWER_ATTACK_ADD_PACKET p;
		p.size = sizeof(SC_TOWER_ATTACK_ADD_PACKET);
		p.type = SC_TOWER_ATTACK_ADD;
		p.id = id;
		p.x = pos.x; p.y = pos.y; p.z = pos.z;
		m_session->Send(&p);
	}

	void CPacketSender::SendRemoveNpcPacket(int id, NPC_TYPE npcType) const
	{
		SC_REMOVE_NPC_PACKET p;
		p.size = sizeof(SC_REMOVE_NPC_PACKET);
		p.type = SC_REMOVE_NPC;
		p.npcType = static_cast<int>(npcType);
		p.id = id;
		m_session->Send(&p);
	}

	void CPacketSender::SendNpcStatChangePacket(int id, int maxHp, int curHp) const
	{
		SC_NPC_STAT_CHANGE_PACKET p;
		p.size = sizeof(SC_NPC_STAT_CHANGE_PACKET);
		p.type = SC_NPC_STAT_CHANGE;
		p.id = id;
		p.maxHp = maxHp;
		p.curHp = curHp;
		m_session->Send(&p);
	}

	void CPacketSender::SendNpcAttackPacket(int id) const
	{
		SC_NPC_ATTACK_PACKET p;
		p.size = sizeof(SC_NPC_ATTACK_PACKET);
		p.type = SC_NPC_ATTACK;
		p.id = static_cast<short>(id);
		m_session->Send(&p);
	}

	void CPacketSender::SendGoldPacket(int matchID, int gold)
	{
		SC_GIVE_GOLD_PACKET p;
		p.size = sizeof(SC_GIVE_GOLD_PACKET);
		p.type = SC_GIVE_GOLD;
		p.id = static_cast<short>(matchID);
		p.gold = gold;		
		m_session->Send(&p);
	}

	void CPacketSender::SendPlayerStatChangePacket(int id, const CStat& stat) const
	{
		SC_PLAYER_STAT_CHANGE_PACKET p;
		p.size = sizeof(SC_PLAYER_STAT_CHANGE_PACKET);
		p.type = SC_PLAYER_STAT_CHANGE;
		p.id = id;
		p.speed = stat.speed;
		m_session->Send(&p);
	}

	void CPacketSender::SendPlayerStatusChangePakcet(int id, char statusType, short statusNum)
	{
		SC_PLAYER_STATUS_CHANGE_PACKET p;
		p.size = sizeof(SC_PLAYER_STATUS_CHANGE_PACKET);
		p.id = id;
		p.type = SC_PLAYER_STATUS_CHANGE;
		p.statusType = statusType;
		p.statusNum = statusNum;
		m_session->Send(&p);
	}

	void CPacketSender::SendPlayerHealthManaPacket(int id, CHealthMana& healthMana) const
	{
		SC_HEALTH_MANA_PACKET p;
		p.size = sizeof(SC_HEALTH_MANA_PACKET);
		p.type = SC_HEALTH_MANA;
		p.id = id;
		p.maxHp = healthMana.GetMaxHp();
		p.curHp = healthMana.GetCurHp();
		p.maxMp = healthMana.GetMaxMp();
		p.curMp = healthMana.GetCurMp();
		m_session->Send(&p);
	}

	void CPacketSender::SendSkillFinishPacket(int id) const
	{
		SC_SKILL_FINISH_PACKET p;
		p.size = sizeof(SC_SKILL_FINISH_PACKET);
		p.type = SC_SKILL_FINISH;
		p.id = static_cast<short>(id);
		m_session->Send(&p);
	}

	void CPacketSender::SendPlayerRespawnPacket(int id, bool respawn) const
	{
		SC_PLAYER_RESPAWN_PACKET p;
		p.size = sizeof(SC_PLAYER_RESPAWN_PACKET);
		p.type = SC_PLAYER_RESPWAN;
		p.id = id;
		p.respawn = respawn;
		m_session->Send(&p);
	}

	void CPacketSender::SendStructureStatChangePacket(int id, int maxHp, int curHp) const
	{
		SC_STRUCTURE_STAT_CHANGE_PACKET p;
		p.size = sizeof(SC_STRUCTURE_STAT_CHANGE_PACKET);
		p.type = SC_STRUCTURE_STAT_CHANGE;
		p.id = id;
		p.maxHp = maxHp;
		p.curHp = curHp;
		m_session->Send(&p);
	}

	void CPacketSender::SendStructureStatusChangePacket(int id, bool broken) const
	{
		SC_STRUCTURE_STATUS_CHANGE_PACKET p;
		p.size = sizeof(SC_STRUCTURE_STATUS_CHANGE_PACKET);
		p.type = SC_STRUCTURE_STATUS_CHANGE;
		p.id = id;
		p.broken = broken;
		m_session->Send(&p);
	}

	void CPacketSender::SendMagicEyePacket(bool show) const
	{
		SC_MAGIC_EYE_POS_PACKET p;
		p.size = sizeof(p);
		p.type = SC_MAGIC_EYE_POS;
		p.show = show;
		m_session->Send(&p);
	}

	void CPacketSender::SendTeleportActivePacket(bool active) const
	{
		SC_TELEPORT_ACTIVE_PACKET p;
		p.size = sizeof(p);
		p.type = SC_TELEPORT_ACTIVE;
		p.active = active;
		m_session->Send(&p);
	}

	void CPacketSender::SendGameOverPacket(bool nexusDestroy) const
	{
		SC_GAME_OVER_PACKET p;
		p.size = sizeof(p);
		p.type = SC_GAME_OVER;
		p.nexusDestroy = nexusDestroy;
		m_session->Send(&p);
	}

	void CPacketSender::SendMonsterKillBuffPacket(char monsterType, int id) const
	{
		SC_MONSTER_KILL_BUFF_PACKET p;
		p.size = sizeof(p);
		p.type = SC_MONSTER_KILL_BUFF;
		p.monsterType = monsterType;
		p.id = id;
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

	void CPacketSender::SendAddSkillObjectPacket(int id, SKILL_TYPE type, const vec3& pos, const vec3& look)
	{
		SC_ADD_SKILL_OBJECT_PACKET p;
		p.size = sizeof(SC_ADD_SKILL_OBJECT_PACKET);
		p.type = SC_ADD_SKILL_OBJECT;
		p.id = id;
		p.objectType = static_cast<short>(type);
		p.x = pos.x; p.y = pos.y; p.z = pos.z;
		p.lookX = look.x; p.lookY = look.y; p.lookZ = look.z;
		m_session->Send(&p);
	}

	void CPacketSender::SendUpdateSkillObjectPacket(int id, SKILL_TYPE objectType, const vec3& pos)
	{
		SC_UPDATE_SKILL_OBJECT_PACKET p;
		p.size = sizeof(SC_UPDATE_SKILL_OBJECT_PACKET);
		p.type = SC_UPDATE_SKILL_OBJECT;
		p.objectType = static_cast<short>(objectType);
		p.id = id;
		p.x = pos.x; p.y = pos.y; p.z = pos.z;
		m_session->Send(&p);
	}

	void CPacketSender::SendRemoveSkillObjectPacket(int id, SKILL_TYPE objectType)
	{
		SC_REMOVE_SKILL_OBJECT_PACKET p;
		p.size = sizeof(SC_REMOVE_SKILL_OBJECT_PACKET);
		p.type = SC_REMOVE_SKILL_OBJECT;
		p.id = id;
		p.objectType = static_cast<short>(objectType);
		m_session->Send(&p);
	}

	void CPacketSender::SendDummyLoginInfoPacket(int id, const vec3& pos)
	{
		SC_DUMMY_LOGIN_INFO_PACKET p;
		p.size = sizeof(p);
		p.type = SC_DUMMY_LOGIN_INFO;
		p.id = id;
		p.x = pos.x;
		p.y = pos.y;
		p.z = pos.z;
		m_session->Send(&p);
	}
}