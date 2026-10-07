#include "pch.h"
#include <Protocol/Validation.h>
#include "CClient.h"
#include "SocketUtil.h"
#include "CPacketMgr.h"
#include "Resource.h"
#include "Stats.h"
#include "ClientInfos.h"

namespace wod_server {

	CClient::CClient()
	{
		m_pos = { 0, 4.0f, 0 };

		m_maxVelXZ = PLAYER_MAX_VELXZ;
		m_maxVelY = PLAYER_MAX_VELY;
		m_friction = WORLD_FRICTION;

		m_look = { 0.f, 0.f, 1.f };
		m_right = { 1.f, 0.f, 0.f };
		m_up = { 0.f, 1.f, 0.f };		

		m_matchID = -1;

		m_jump = false;
		m_jumpNum = 0;
		m_jumpTime = 0.f;

		m_teleport = false;
		m_teleportNum = 0;
		m_teleportLastUsedTime = TimeUtil::CurTime();

		m_status = std::make_shared<CStatus>();

		m_packetSender = new CPacketSender;
		for (int i = 0; i < m_skills.size(); ++i) {
			m_skills[i] = std::make_shared<CSkill>();
		}
		for (int i = 0; i < m_itemInfos.size(); ++i) {
			m_itemInfos[i] = new CItemInfo;
		}

		m_defensiveFunction.insert({ DEFENSIVE_BUFF::NONE, [this](int damage, int id)->bool {return NoDefensive(damage, id); } });
		m_defensiveFunction.insert({DEFENSIVE_BUFF::REFLECT, [this](int damage, int id)->bool {return Reflect(damage, id); } });
		m_defensiveFunction.insert({ DEFENSIVE_BUFF::DEFENSIVE_STANCE, [this](int damage, int id)->bool {return DefensiveStance(damage, id); } });
		m_defensiveFunction.insert({ DEFENSIVE_BUFF::INDESTRUCTIBLE, [this](int damage, int id)->bool {return Indestructible(damage, id); } });
		m_defensiveFunction.insert({ DEFENSIVE_BUFF::COUNTER, [this](int damage, int id)->bool {return Counter(damage, id); } });
		m_defensiveFunction.insert({ DEFENSIVE_BUFF::ENDURE, [this](int damage, int id)->bool {return Endure(damage, id); } });
	}

	CClient::~CClient()
	{
		for (int i = 0; i < m_itemInfos.size(); ++i) {
			delete m_itemInfos[i];
		}
		delete m_packetSender;
	}

	void CClient::Initialize()
	{
		stateLock.lock();
		m_state = CL_STATE::ST_READY;
		stateLock.unlock();

		m_pos = { 0.0f, 4.0f, 0.0f };
	}

	void CClient::InitializeBoundingBox(const vec3& boxCenter, const vec3& boxExtent, float scaleValue)
	{
		m_boundingBox.Center = { boxCenter.x, boxCenter.y, boxCenter.z };
		m_boundingBox.Extents = { boxExtent.x, boxExtent.y, boxExtent.z };

		m_initBoundingBox.Center = { boxCenter.x, boxCenter.y, boxCenter.z };
		m_initBoundingBox.Extents = { boxExtent.x, boxExtent.y, boxExtent.z };

		DirectX::XMStoreFloat4x4(&m_worldMatrix, DirectX::XMMatrixIdentity());
		DirectX::XMMATRIX mtxScale = DirectX::XMMatrixScaling(scaleValue, scaleValue, scaleValue);
		XMStoreFloat4x4(&m_worldMatrix, mtxScale * DirectX::XMLoadFloat4x4(&m_worldMatrix));
		m_worldMatrix._21 = 0.f; m_worldMatrix._22 = 1.f; m_worldMatrix._23 = 0.f;
	}

	void CClient::SetInitializeStat(int hp, int mp, int attack, int magic, int defense, int regist, int speed, int tentacity, int critical)
	{
		CStat tempStat;
		if (m_matchID == MAX_PLAYER - 1) {
			//boss
			m_status->healthMana.SetMaxHp(BossStats::INIT_HP + BossStats::HP_STAT_SELECT_INCREASE * hp);
			m_status->healthMana.SetCurHp(BossStats::INIT_HP + BossStats::HP_STAT_SELECT_INCREASE * hp);
			m_status->healthMana.SetCurHp(BossStats::INIT_HP + BossStats::HP_STAT_SELECT_INCREASE * hp);
			m_status->healthMana.SetMaxMp(BossStats::INIT_MP + BossStats::MP_STAT_SELECT_INCREASE * mp);
			m_status->healthMana.SetCurMp(BossStats::INIT_MP + BossStats::MP_STAT_SELECT_INCREASE * mp);
			tempStat.strength = BossStats::INIT_ATTACK + BossStats::ATTACK_STAT_SELECT_INCREASE * attack;
			tempStat.magic = BossStats::INIT_MAGIC + BossStats::MAGIC_STAT_SELECT_INCREASE * magic;
			tempStat.armor = BossStats::INIT_DEFENSE + BossStats::DEFENSE_STAT_SELECT_INCREASE * defense;
			tempStat.regist = BossStats::INIT_REGIST + BossStats::REGIST_STAT_SELECT_INCREASE * regist;
			tempStat.speed = BossStats::INIT_SPEED + BossStats::SPEED_STAT_SELECT_INCREASE * static_cast<float>(speed);
			tempStat.endure = BossStats::INIT_TENTACITY + BossStats::TENTACITY_STAT_SELECT_INCREASE * tentacity;
			tempStat.critical = BossStats::INIT_CRITICAL + BossStats::CRITICAL_STAT_SELECT_INCREASE * critical;
		}
		else {
			//hero
			m_status->healthMana.SetMaxHp(HeroStats::INIT_HP + HeroStats::HP_STAT_SELECT_INCREASE * hp);
			m_status->healthMana.SetCurHp(HeroStats::INIT_HP + HeroStats::HP_STAT_SELECT_INCREASE * hp);
			m_status->healthMana.SetMaxMp(HeroStats::INIT_MP + HeroStats::MP_STAT_SELECT_INCREASE * mp);
			m_status->healthMana.SetCurMp(HeroStats::INIT_MP + HeroStats::MP_STAT_SELECT_INCREASE * mp);
			tempStat.strength = HeroStats::INIT_ATTACK + HeroStats::ATTACK_STAT_SELECT_INCREASE * attack;
			tempStat.magic = HeroStats::INIT_MAGIC + HeroStats::MAGIC_STAT_SELECT_INCREASE * magic;
			tempStat.armor = HeroStats::INIT_DEFENSE + HeroStats::DEFENSE_STAT_SELECT_INCREASE * defense;
			tempStat.regist = HeroStats::INIT_REGIST + HeroStats::REGIST_STAT_SELECT_INCREASE * regist;
			tempStat.speed = HeroStats::INIT_SPEED + HeroStats::SPEED_STAT_SELECT_INCREASE * static_cast<float>(speed);
			tempStat.endure = HeroStats::INIT_TENTACITY + HeroStats::TENTACITY_STAT_SELECT_INCREASE * tentacity;
			tempStat.critical = HeroStats::INIT_CRITICAL + HeroStats::CRITICAL_STAT_SELECT_INCREASE * critical;
		}
		m_status->SetStat(tempStat);
	}

	bool CClient::CheckTeleportCoolTime()
	{
		return (TimeUtil::CurTime() - m_teleportLastUsedTime > std::chrono::milliseconds(TELEPORT_COOLTIME));
	}

	void CClient::Move(float elapsedTime)
	{
		if (m_usingSkill || (DEFENSIVE_BUFF::INDESTRUCTIBLE == m_status->defensiveBuff) || SKILL_BUFF::STUN == m_status->skillBuff || m_isDead) {
			m_vel = { 0.f, 0.f, 0.f };
			m_dir = 0;
			SetUpdateTime();
			return;
		}

		if (m_jump) {
			m_vel = { 0.f, 0.f, 0.f };
			m_dir = 0;
			Jump(elapsedTime);
			return;
		}

		if (m_teleport) {
			m_vel = { 0.f, 0.f, 0.f };
			m_dir = 0;
			Teleport(elapsedTime);
			return;
		}

		vec3 shift = { 0, 0, 0 };
		if (m_dir & DIR_FORWARD) {
			shift = vec3::Add(shift, m_look, PLAYER_SPEED);
		}
		if (m_dir & DIR_BACKWARD) {
			shift = vec3::Add(shift, m_look, -PLAYER_SPEED);
		}
		if (m_dir & DIR_RIGHT) {
			shift = vec3::Add(shift, m_right, PLAYER_SPEED);
		}
		if (m_dir & DIR_LEFT) {
			shift = vec3::Add(shift, m_right, -PLAYER_SPEED);
		}
		m_vel += shift;

		float velocity = sqrtf(m_vel.x * m_vel.x + m_vel.z * m_vel.z);

		//Check the velocity is below the maximum velocity
		if (velocity > m_maxVelXZ) {
			m_vel.x *= (m_maxVelXZ / velocity);
			m_vel.z *= (m_maxVelXZ / velocity);
		}

		//Check collision and moves if it doesn't collide
		shift = m_vel * elapsedTime * m_status->GetStat().speed;

		float height;
		
		if (3 == (m_matchID % MAX_PLAYER) && CGameMgr::GetInstance()->GetFence(m_matchNum) && GameUtil::FenceCollision(m_boundingBox, shift)) {
			m_vel = vec3(0.f, 0.f, 0.f);
			return;
		}

		if (GameUtil::ClientCollisionCheck(m_id, shift)) {
			if (GameUtil::MapCollision(m_pos, shift, height, m_curNode)) {
				m_pos += shift;
				m_pos.y = height;
			}
		}

		if (m_dir != 0)
			UpdateBoundingBox();
		

		//Deceleration calculation
		velocity = m_vel.Length();
		float deceleration = m_friction * elapsedTime;
		if (deceleration > velocity)
			deceleration = velocity;
		m_vel = vec3::Add(m_vel, vec3::Normalize(m_vel * -deceleration));	
	}

	void CClient::Jump(float elapsedTime)
	{
		m_pos = GameUtil::GetJumpPos(m_jumpNum, m_jumpTime);		

		m_jumpTime += elapsedTime;
		if (m_jumpTime >= JUMP_TIME) {
			m_jump = false;
			m_jumpTime = 0.f;
			m_pos.y = 2.0f;
			m_dir = 0;
			m_packetSender->SendJumpFinishPacket();
		}
		m_packetSender->SendMovePacket(m_matchID, m_pos, m_dir);
	}

	void CClient::Teleport(float elapsedTime)
	{
		vec2 dir = vec2::Normalize(TELEPORT_TARGET_POS[m_teleportNum] - TELEPORT_POS[m_teleportNum]);

		m_pos.x += dir.x * TELEPORT_SPEED * elapsedTime;
		m_pos.z += dir.z * TELEPORT_SPEED * elapsedTime;

		if(::sqrt(::pow(TELEPORT_TARGET_POS[m_teleportNum].x - m_pos.x, 2) + ::pow(TELEPORT_TARGET_POS[m_teleportNum].z - m_pos.z, 2)) < 1.f) {
			m_pos.x = TELEPORT_TARGET_POS[m_teleportNum].x;
			m_pos.z = TELEPORT_TARGET_POS[m_teleportNum].z;
			
			for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(m_matchNum)) {
				if (-1 == id)
					continue;
				PACKET_SENDER(id)->SendTeleportPacket(m_matchID, true);
			}
			m_teleport = false;
		}
		m_packetSender->SendMovePacket(m_matchID, m_pos, m_dir);
	}

	void CClient::Damage(int power, int critical, DAMAGE_TYPE type, int objectID)
	{
		if (m_defensiveFunction[m_status->defensiveBuff](power, objectID))
			return;

		CStat stat = m_status->GetStat();
		if (m_status->healthMana.Damage(power, critical, stat.armor, stat.regist, type)) {
			ProcessDeath(objectID);
		}
		else {
			for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(m_matchNum)) {
				if (id == -1)
					continue;
				PACKET_SENDER(id)->SendPlayerHealthManaPacket(m_matchID, m_status->healthMana);
			}
		}

		if (objectID < NPC_ID) {
			if (CObjectMgr::GetInstance()->GetClient(objectID)->GetStatus()->damageBuff == DAMAGE_BUFF::GLUTTONY) {
				CObjectMgr::GetInstance()->GetClient(objectID)->GetStatus()->healthMana.HealHp(static_cast<int>(power * 0.3f));
				for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(m_matchNum)) {
					if (id == -1)
						continue;
					PACKET_SENDER(id)->SendPlayerHealthManaPacket(3, CObjectMgr::GetInstance()->GetClient(objectID)->GetStatus()->healthMana);
				}
			}
		}
	}

	void CClient::RecvProcess(const DWORD& bytes, OverlapEx* overEx)
	{

        auto session = m_packetSender->GetSession();
        const auto generation = session->Generation();
        std::vector<wod::core::FrameDecoder::Frame> frames;
        if (!session->Decode(bytes, *overEx, frames)) { Disconnect(); return; }
        for (auto& frame : frames) {
            if (!wod::protocol::Validate(frame, wod::protocol::Endpoint::GameClient)) { Disconnect(); return; }
            auto* packet = reinterpret_cast<BASE_PACKET*>(frame.data());
            if (packet->type == CS_LOGIN || packet->type == CS_RTT || packet->type == CS_TEST_INGAME || packet->type == CS_TEST_INGAME2)
                session->WithGeneration(generation, [&] { CPacketMgr::GetInstance()->Packet_Exec(packet, shared_from_this()); });
            else {
                if (m_matchNum < 0 || m_matchNum >= MAX_MATCH) { Disconnect(); return; }
                auto& match = CMatchMgr::GetInstance()->GetMatch(m_matchNum);
                match.PushJob([client = shared_from_this(), session, generation, frame = std::move(frame)]() mutable {
                    session->WithGeneration(generation, [&] { CPacketMgr::GetInstance()->Packet_Exec(reinterpret_cast<BASE_PACKET*>(frame.data()), client); });
                });
            }
        }
        session->Recv();

	}

	void CClient::Disconnect()
	{
        auto session = m_packetSender->GetSession();
        session->Invalidate();
        auto* over = Resource::GetOverObjectFromPool();
        over->SetOP(OP_TYPE::OP_DISCONNECT);
        if (!SocketUtil::Runtime().Disconnect(session->GetSocket(), *over)) Resource::overExPool.push(over);
        std::unique_lock lock(stateLock); m_state = CL_STATE::ST_FREE;

	}

	void CClient::ProcessRespawn(TimePoint now)
	{
		if (now - m_deathTime < std::chrono::milliseconds(CGameMgr::GetInstance()->GetHeroRespawnTime(m_matchNum)))
			return;

		m_isDead = false;
		m_usingSkill = false;
		m_status->healthMana.SetCurHp(m_status->healthMana.GetMaxHp());
		m_status->healthMana.SetCurMp(GetStatus()->healthMana.GetMaxMp());
		m_status->skillBuff = SKILL_BUFF::NONE;

		for (int playerID : CMatchMgr::GetInstance()->GetMatchPlayers(m_matchNum)) {
			if (-1 == playerID)
				continue;

			PACKET_SENDER(playerID)->SendPlayerRespawnPacket(m_matchID, false);
			PACKET_SENDER(playerID)->SendMovePacket(m_matchID, GetPos(), 0);
			PACKET_SENDER(playerID)->SendPlayerHealthManaPacket(m_matchID, GetStatus()->healthMana);
			PACKET_SENDER(playerID)->SendSkillFinishPacket(m_matchID);
			PACKET_SENDER(playerID)->SendPlayerStatusChangePakcet(m_matchID, 0, static_cast<short>(SKILL_BUFF::NONE));
		}
	}

	void CClient::ProcessBaseHeal(TimePoint now)
	{
		if (now - m_lastBaseHeal < std::chrono::milliseconds(ClientInfos::BASE_HEAL_COOLTIME))
			return;

		const auto SendHealthManaPkt = [this, now](int healAmount) {
			m_status->healthMana.HealHp(healAmount);
			for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(m_matchNum))
			{
				if (-1 == id)
					continue;
				PACKET_SENDER(id)->SendPlayerHealthManaPacket(m_matchID, m_status->healthMana);
			}

			m_lastBaseHeal = now;
			};

		if (m_matchID == 3) {
			if (DistanceXZ(m_pos, vec3(BOSS_SHOP_POS_X, 0.0f, BOSS_SHOP_POS_Z)) > SHOP_DISTANCE)
				return;

			SendHealthManaPkt(ClientInfos::BOSS_BASE_HEAL_AMOUNT);
		}
		else {
			if (DistanceXZ(m_pos, vec3(SHOP_POS_X, 0.0f, SHOP_POS_Z)) > SHOP_DISTANCE)
				return;
			
			SendHealthManaPkt(ClientInfos::HERO_BASE_HEAL_AMOUNT);
		}
	}

	void CClient::ProcessTeleportCoolTime(TimePoint now)
	{
		if (!m_isTeleportCool)
			return;

		if (now - m_teleportLastUsedTime < std::chrono::milliseconds(TELEPORT_COOLTIME))
			return;

		m_teleportLastUsedTime = now;
		m_isTeleportCool = false;
		m_packetSender->SendTeleportActivePacket(true);
	}

	bool CClient::Update(float elapsedTime)
	{
		auto now = CGameMgr::GetInstance()->GetLastTime(m_matchNum);
		if (m_isDead)
		{
			ProcessRespawn(now);
			return true;
		}

		ProcessBaseHeal(now);
		ProcessTeleportCoolTime(now);

		Move(elapsedTime);


		return true;
	}

	bool CClient::NoDefensive(int damage, int id)
	{
		return false;
	}

	bool CClient::Reflect(int damage, int id)
	{
		if (id >= NPC_ID) {
			CObjectMgr::GetInstance()->GetNpc(m_matchNum, id - NPC_ID)->Damaged(m_id, damage, DAMAGE_TYPE::MAGIC, false);
		}
		else {
			CObjectMgr::GetInstance()->GetClient(id)->Damage(damage, 0, DAMAGE_TYPE::MAGIC, m_id);
		}

		return true;
	}

	bool CClient::DefensiveStance(int damage, int id)
	{
		CStat stat = m_status->GetStat();
		if (m_status->healthMana.Damage(static_cast<int>(damage * 0.4f), 0, stat.armor, stat.regist, DAMAGE_TYPE::STRENGTH)) {
			//Player Respawn
			ProcessDeath(id);
			m_pos = ClientInfos::HERO_START_POS[m_matchID];
			UpdateBoundingBox();

			for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(m_matchNum)) {
				if (id == -1)
					continue;
				CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendPlayerRespawnPacket(m_matchID, true);
			}
		}
		else {
			for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(m_matchNum)) {
				if (id == -1)
					continue;
				CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendPlayerHealthManaPacket(m_matchID, m_status->healthMana);
			}
		}
		return true;
	}

	bool CClient::Indestructible(int damage, int id)
	{
		CStat stat = m_status->GetStat();
		if (m_status->healthMana.Damage(static_cast<int>(damage * 0.1f), 0, stat.armor, stat.regist, DAMAGE_TYPE::STRENGTH)) {
			//Player Respawn
			ProcessDeath(id);
			m_pos = ClientInfos::HERO_START_POS[m_matchID];
			UpdateBoundingBox();

			for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(m_matchNum)) {
				if (id == -1)
					continue;
				CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendPlayerRespawnPacket(m_matchID, true);
			}
		}
		else {
			for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(m_matchNum)) {
				if (id == -1)
					continue;
				CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendPlayerHealthManaPacket(m_matchID, m_status->healthMana);
			}
		}
		return true;
	}

	bool CClient::Counter(int damage, int id)
	{
		m_accumulatedDamage += damage;

		return true;
	}

	bool CClient::Endure(int damage, int id)
	{
		CStat stat = m_status->GetStat();
		if (m_status->healthMana.Damage(static_cast<int>(damage * 0.5f), 0, stat.armor, stat.regist, DAMAGE_TYPE::STRENGTH)) {
			CGameMgr::GetInstance()->GameOver(m_matchNum, true);
		}
		else {
			for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(m_matchNum)) {
				if (id == -1)
					continue;
				CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendPlayerHealthManaPacket(m_matchID, m_status->healthMana);
			}
		}
		m_accumulatedDamage += damage;

		return true;
	}

	void CClient::ProcessDeath(int killerID)
	{
		m_isDead = true;
		m_deathTime = TimeUtil::CurTime();

		// 보스인 경우 게임오버
		if (m_matchID == 3) 
		{
			CGameMgr::GetInstance()->GameOver(m_matchNum, true);
			for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(m_matchNum)) {
				if (-1 == id)
					continue;

				PACKET_SENDER(id)->SendGameOverPacket(false);
			}

			return;
		}
		
		// 영웅인 경우 처리
		m_pos = ClientInfos::HERO_START_POS[m_matchID];
		m_pos.y += 0.3f;
		UpdateBoundingBox();

		for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(m_matchNum)) {
			if (id == -1)
				continue;
			PACKET_SENDER(id)->SendPlayerRespawnPacket(m_matchID, true);
		}

		//Kill Golde for Boss
		if (killerID < NPC_ID) {
			std::shared_ptr<CClient> boss = CObjectMgr::GetInstance()->GetClient(killerID);
			int increasedGold = boss->GetGold() + ClientInfos::HERO_KILL_GOLD;
			boss->SetGold(increasedGold);
			boss->GetPacketSender()->SendGoldPacket(boss->GetMatchId(), increasedGold);
		}
	}
}