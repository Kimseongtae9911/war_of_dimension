#include "pch.h"
#include <Protocol/Validation.h>
#include "CClient.h"
#include "CPacketMgr.h"
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

		m_defensiveFunction.insert({ DEFENSIVE_BUFF::NONE, [this](int _damage, int _id)->bool {return NoDefensive(_damage, _id); } });
		m_defensiveFunction.insert({DEFENSIVE_BUFF::REFLECT, [this](int _damage, int _id)->bool {return Reflect(_damage, _id); } });
		m_defensiveFunction.insert({ DEFENSIVE_BUFF::DEFENSIVE_STANCE, [this](int _damage, int _id)->bool {return DefensiveStance(_damage, _id); } });
		m_defensiveFunction.insert({ DEFENSIVE_BUFF::INDESTRUCTIBLE, [this](int _damage, int _id)->bool {return Indestructible(_damage, _id); } });
		m_defensiveFunction.insert({ DEFENSIVE_BUFF::COUNTER, [this](int _damage, int _id)->bool {return Counter(_damage, _id); } });
		m_defensiveFunction.insert({ DEFENSIVE_BUFF::ENDURE, [this](int _damage, int _id)->bool {return Endure(_damage, _id); } });
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
		m_stateLock.lock();
		m_state = CL_STATE::ST_READY;
		m_stateLock.unlock();

		m_pos = { 0.0f, 4.0f, 0.0f };
	}

	void CClient::InitializeBoundingBox(const vec3& _boxCenter, const vec3& _boxExtent, float _scaleValue)
	{
		m_boundingBox.Center = { _boxCenter.m_x, _boxCenter.m_y, _boxCenter.m_z };
		m_boundingBox.Extents = { _boxExtent.m_x, _boxExtent.m_y, _boxExtent.m_z };

		m_initBoundingBox.Center = { _boxCenter.m_x, _boxCenter.m_y, _boxCenter.m_z };
		m_initBoundingBox.Extents = { _boxExtent.m_x, _boxExtent.m_y, _boxExtent.m_z };

		DirectX::XMStoreFloat4x4(&m_worldMatrix, DirectX::XMMatrixIdentity());
		DirectX::XMMATRIX mtxScale = DirectX::XMMatrixScaling(_scaleValue, _scaleValue, _scaleValue);
		XMStoreFloat4x4(&m_worldMatrix, mtxScale * DirectX::XMLoadFloat4x4(&m_worldMatrix));
		m_worldMatrix._21 = 0.f; m_worldMatrix._22 = 1.f; m_worldMatrix._23 = 0.f;
	}

	void CClient::SetInitializeStat(int _hp, int _mp, int _attack, int _magic, int _defense, int _regist, int _speed, int _tentacity, int _critical)
	{
		CStat tempStat;
		if (m_matchID == MAX_PLAYER - 1) {
			//boss
			m_status->m_healthMana.SetMaxHp(BossStats::INIT_HP + BossStats::HP_STAT_SELECT_INCREASE * _hp);
			m_status->m_healthMana.SetCurHp(BossStats::INIT_HP + BossStats::HP_STAT_SELECT_INCREASE * _hp);
			m_status->m_healthMana.SetCurHp(BossStats::INIT_HP + BossStats::HP_STAT_SELECT_INCREASE * _hp);
			m_status->m_healthMana.SetMaxMp(BossStats::INIT_MP + BossStats::MP_STAT_SELECT_INCREASE * _mp);
			m_status->m_healthMana.SetCurMp(BossStats::INIT_MP + BossStats::MP_STAT_SELECT_INCREASE * _mp);
			tempStat.m_strength = BossStats::INIT_ATTACK + BossStats::ATTACK_STAT_SELECT_INCREASE * _attack;
			tempStat.m_magic = BossStats::INIT_MAGIC + BossStats::MAGIC_STAT_SELECT_INCREASE * _magic;
			tempStat.m_armor = BossStats::INIT_DEFENSE + BossStats::DEFENSE_STAT_SELECT_INCREASE * _defense;
			tempStat.m_regist = BossStats::INIT_REGIST + BossStats::REGIST_STAT_SELECT_INCREASE * _regist;
			tempStat.m_speed = BossStats::INIT_SPEED + BossStats::SPEED_STAT_SELECT_INCREASE * static_cast<float>(_speed);
			tempStat.m_endure = BossStats::INIT_TENTACITY + BossStats::TENTACITY_STAT_SELECT_INCREASE * _tentacity;
			tempStat.m_critical = BossStats::INIT_CRITICAL + BossStats::CRITICAL_STAT_SELECT_INCREASE * _critical;
		}
		else {
			//hero
			m_status->m_healthMana.SetMaxHp(HeroStats::INIT_HP + HeroStats::HP_STAT_SELECT_INCREASE * _hp);
			m_status->m_healthMana.SetCurHp(HeroStats::INIT_HP + HeroStats::HP_STAT_SELECT_INCREASE * _hp);
			m_status->m_healthMana.SetMaxMp(HeroStats::INIT_MP + HeroStats::MP_STAT_SELECT_INCREASE * _mp);
			m_status->m_healthMana.SetCurMp(HeroStats::INIT_MP + HeroStats::MP_STAT_SELECT_INCREASE * _mp);
			tempStat.m_strength = HeroStats::INIT_ATTACK + HeroStats::ATTACK_STAT_SELECT_INCREASE * _attack;
			tempStat.m_magic = HeroStats::INIT_MAGIC + HeroStats::MAGIC_STAT_SELECT_INCREASE * _magic;
			tempStat.m_armor = HeroStats::INIT_DEFENSE + HeroStats::DEFENSE_STAT_SELECT_INCREASE * _defense;
			tempStat.m_regist = HeroStats::INIT_REGIST + HeroStats::REGIST_STAT_SELECT_INCREASE * _regist;
			tempStat.m_speed = HeroStats::INIT_SPEED + HeroStats::SPEED_STAT_SELECT_INCREASE * static_cast<float>(_speed);
			tempStat.m_endure = HeroStats::INIT_TENTACITY + HeroStats::TENTACITY_STAT_SELECT_INCREASE * _tentacity;
			tempStat.m_critical = HeroStats::INIT_CRITICAL + HeroStats::CRITICAL_STAT_SELECT_INCREASE * _critical;
		}
		m_status->SetStat(tempStat);
	}

	bool CClient::CheckTeleportCoolTime()
	{
		return (TimeUtil::CurTime() - m_teleportLastUsedTime > std::chrono::milliseconds(TELEPORT_COOLTIME));
	}

	void CClient::Move(float _elapsedTime)
	{
		if (m_usingSkill || (DEFENSIVE_BUFF::INDESTRUCTIBLE == m_status->m_defensiveBuff) || SKILL_BUFF::STUN == m_status->m_skillBuff || m_isDead) {
			m_vel = { 0.f, 0.f, 0.f };
			m_dir = 0;
			SetUpdateTime();
			return;
		}

		if (m_jump) {
			m_vel = { 0.f, 0.f, 0.f };
			m_dir = 0;
			Jump(_elapsedTime);
			return;
		}

		if (m_teleport) {
			m_vel = { 0.f, 0.f, 0.f };
			m_dir = 0;
			Teleport(_elapsedTime);
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

		float velocity = sqrtf(m_vel.m_x * m_vel.m_x + m_vel.m_z * m_vel.m_z);

		//Check the velocity is below the maximum velocity
		if (velocity > m_maxVelXZ) {
			m_vel.m_x *= (m_maxVelXZ / velocity);
			m_vel.m_z *= (m_maxVelXZ / velocity);
		}

		//Check collision and moves if it doesn't collide
		shift = m_vel * _elapsedTime * m_status->GetStat().m_speed;

		float height;

		if (3 == (m_matchID % MAX_PLAYER) && CGameMgr::GetInstance()->GetFence(m_matchNum) && GameUtil::FenceCollision(m_boundingBox, shift)) {
			m_vel = vec3(0.f, 0.f, 0.f);
			return;
		}

		if (GameUtil::ClientCollisionCheck(m_id, shift)) {
			if (GameUtil::MapCollision(m_pos, shift, height, m_curNode)) {
				m_pos += shift;
				m_pos.m_y = height;
			}
		}

		if (m_dir != 0)
			UpdateBoundingBox();

		//Deceleration calculation
		velocity = m_vel.Length();
		float deceleration = m_friction * _elapsedTime;
		if (deceleration > velocity)
			deceleration = velocity;
		m_vel = vec3::Add(m_vel, vec3::Normalize(m_vel * -deceleration));
	}

	void CClient::Jump(float _elapsedTime)
	{
		m_pos = GameUtil::GetJumpPos(m_jumpNum, m_jumpTime);

		m_jumpTime += _elapsedTime;
		if (m_jumpTime >= JUMP_TIME) {
			m_jump = false;
			m_jumpTime = 0.f;
			m_pos.m_y = 2.0f;
			m_dir = 0;
			m_packetSender->SendJumpFinishPacket();
		}
		m_packetSender->SendMovePacket(m_matchID, m_pos, m_dir);
	}

	void CClient::Teleport(float _elapsedTime)
	{
		vec2 dir = vec2::Normalize(TELEPORT_TARGET_POS[m_teleportNum] - TELEPORT_POS[m_teleportNum]);

		m_pos.m_x += dir.m_x * TELEPORT_SPEED * _elapsedTime;
		m_pos.m_z += dir.m_z * TELEPORT_SPEED * _elapsedTime;

		if(::sqrt(::pow(TELEPORT_TARGET_POS[m_teleportNum].m_x - m_pos.m_x, 2) + ::pow(TELEPORT_TARGET_POS[m_teleportNum].m_z - m_pos.m_z, 2)) < 1.f) {
			m_pos.m_x = TELEPORT_TARGET_POS[m_teleportNum].m_x;
			m_pos.m_z = TELEPORT_TARGET_POS[m_teleportNum].m_z;

			for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(m_matchNum)) {
				if (-1 == id)
					continue;
				PACKET_SENDER(id)->SendTeleportPacket(m_matchID, true);
			}
			m_teleport = false;
		}
		m_packetSender->SendMovePacket(m_matchID, m_pos, m_dir);
	}

	void CClient::Damage(int _power, int _critical, DAMAGE_TYPE _type, int _objectID)
	{
		if (m_defensiveFunction[m_status->m_defensiveBuff](_power, _objectID))
			return;

		CStat stat = m_status->GetStat();
		if (m_status->m_healthMana.Damage(_power, _critical, stat.m_armor, stat.m_regist, _type)) {
			ProcessDeath(_objectID);
		}
		else {
			for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(m_matchNum)) {
				if (id == -1)
					continue;
				PACKET_SENDER(id)->SendPlayerHealthManaPacket(m_matchID, m_status->m_healthMana);
			}
		}

		if (_objectID < NPC_ID) {
			if (CObjectMgr::GetInstance()->GetClient(_objectID)->GetStatus()->m_damageBuff == DAMAGE_BUFF::GLUTTONY) {
				CObjectMgr::GetInstance()->GetClient(_objectID)->GetStatus()->m_healthMana.HealHp(static_cast<int>(_power * 0.3f));
				for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(m_matchNum)) {
					if (id == -1)
						continue;
					PACKET_SENDER(id)->SendPlayerHealthManaPacket(3, CObjectMgr::GetInstance()->GetClient(_objectID)->GetStatus()->m_healthMana);
				}
			}
		}
	}

    CClient::SessionRef CClient::GetTransportSession() const
    {
        return m_packetSender->GetSession();
    }

    bool CClient::ValidateFrame(std::span<const char> _frame) const
    {
        return wod::protocol::Validate(_frame, wod::protocol::Endpoint::GameClient);
    }

    bool CClient::DispatchFrame(Frame _frame, const SessionRef& _session, uint64_t _generation)
    {
        auto *packet = reinterpret_cast<BASE_PACKET *>(_frame.data());
        if (packet->type == CS_LOGIN || packet->type == CS_RTT || packet->type == CS_TEST_INGAME || packet->type == CS_TEST_INGAME2)
        {
            _session->WithGeneration(_generation, [&] {
                CPacketMgr::GetInstance()->Packet_Exec(packet, shared_from_this());
            });
        }
        else
        {
            if (m_matchNum < 0 || m_matchNum >= MAX_MATCH)
                return false;

            auto& match = CMatchMgr::GetInstance()->GetMatch(m_matchNum);
            match.PushJob([client = shared_from_this(), session = _session, generation = _generation, frame = std::move(_frame)]() mutable {
                session->WithGeneration(generation, [&] {
                    CPacketMgr::GetInstance()->Packet_Exec(reinterpret_cast<BASE_PACKET *>(frame.data()), client);
                });
            });
        }

        return true;
    }

    void CClient::OnDisconnectRequested()
    {
        std::unique_lock lock(m_stateLock);
        m_state = CL_STATE::ST_FREE;
    }

	void CClient::ProcessRespawn(TimePoint _now)
	{
		if (_now - m_deathTime < std::chrono::milliseconds(CGameMgr::GetInstance()->GetHeroRespawnTime(m_matchNum)))
			return;

		m_isDead = false;
		m_usingSkill = false;
		m_status->m_healthMana.SetCurHp(m_status->m_healthMana.GetMaxHp());
		m_status->m_healthMana.SetCurMp(GetStatus()->m_healthMana.GetMaxMp());
		m_status->m_skillBuff = SKILL_BUFF::NONE;

		for (int playerID : CMatchMgr::GetInstance()->GetMatchPlayers(m_matchNum)) {
			if (-1 == playerID)
				continue;

			PACKET_SENDER(playerID)->SendPlayerRespawnPacket(m_matchID, false);
			PACKET_SENDER(playerID)->SendMovePacket(m_matchID, GetPos(), 0);
			PACKET_SENDER(playerID)->SendPlayerHealthManaPacket(m_matchID, GetStatus()->m_healthMana);
			PACKET_SENDER(playerID)->SendSkillFinishPacket(m_matchID);
			PACKET_SENDER(playerID)->SendPlayerStatusChangePakcet(m_matchID, 0, static_cast<short>(SKILL_BUFF::NONE));
		}
	}

	void CClient::ProcessBaseHeal(TimePoint _now)
	{
		if (_now - m_lastBaseHeal < std::chrono::milliseconds(ClientInfos::BASE_HEAL_COOLTIME))
			return;

		const auto SendHealthManaPkt = [this, _now](int _healAmount) {
			m_status->m_healthMana.HealHp(_healAmount);
			for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(m_matchNum))
			{
				if (-1 == id)
					continue;
				PACKET_SENDER(id)->SendPlayerHealthManaPacket(m_matchID, m_status->m_healthMana);
			}

			m_lastBaseHeal = _now;
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

	void CClient::ProcessTeleportCoolTime(TimePoint _now)
	{
		if (!m_isTeleportCool)
			return;

		if (_now - m_teleportLastUsedTime < std::chrono::milliseconds(TELEPORT_COOLTIME))
			return;

		m_teleportLastUsedTime = _now;
		m_isTeleportCool = false;
		m_packetSender->SendTeleportActivePacket(true);
	}

	bool CClient::Update(float _elapsedTime)
	{
		auto now = CGameMgr::GetInstance()->GetLastTime(m_matchNum);
		if (m_isDead)
		{
			ProcessRespawn(now);
			return true;
		}

		ProcessBaseHeal(now);
		ProcessTeleportCoolTime(now);

		Move(_elapsedTime);

		return true;
	}

	bool CClient::NoDefensive(int _damage, int _id)
	{
		return false;
	}

	bool CClient::Reflect(int _damage, int _id)
	{
		if (_id >= NPC_ID) {
			CObjectMgr::GetInstance()->GetNpc(m_matchNum, _id - NPC_ID)->Damaged(m_id, _damage, DAMAGE_TYPE::MAGIC, false);
		}
		else {
			CObjectMgr::GetInstance()->GetClient(_id)->Damage(_damage, 0, DAMAGE_TYPE::MAGIC, m_id);
		}

		return true;
	}

	bool CClient::DefensiveStance(int _damage, int _id)
	{
		CStat stat = m_status->GetStat();
		if (m_status->m_healthMana.Damage(static_cast<int>(_damage * 0.4f), 0, stat.m_armor, stat.m_regist, DAMAGE_TYPE::STRENGTH)) {
			//Player Respawn
			ProcessDeath(_id);
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
				CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendPlayerHealthManaPacket(m_matchID, m_status->m_healthMana);
			}
		}
		return true;
	}

	bool CClient::Indestructible(int _damage, int _id)
	{
		CStat stat = m_status->GetStat();
		if (m_status->m_healthMana.Damage(static_cast<int>(_damage * 0.1f), 0, stat.m_armor, stat.m_regist, DAMAGE_TYPE::STRENGTH)) {
			//Player Respawn
			ProcessDeath(_id);
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
				CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendPlayerHealthManaPacket(m_matchID, m_status->m_healthMana);
			}
		}
		return true;
	}

	bool CClient::Counter(int _damage, int _id)
	{
		m_accumulatedDamage += _damage;

		return true;
	}

	bool CClient::Endure(int _damage, int _id)
	{
		CStat stat = m_status->GetStat();
		if (m_status->m_healthMana.Damage(static_cast<int>(_damage * 0.5f), 0, stat.m_armor, stat.m_regist, DAMAGE_TYPE::STRENGTH)) {
			CGameMgr::GetInstance()->GameOver(m_matchNum, true);
		}
		else {
			for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(m_matchNum)) {
				if (id == -1)
					continue;
				CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendPlayerHealthManaPacket(m_matchID, m_status->m_healthMana);
			}
		}
		m_accumulatedDamage += _damage;

		return true;
	}

	void CClient::ProcessDeath(int _killerID)
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
		m_pos.m_y += 0.3f;
		UpdateBoundingBox();

		for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(m_matchNum)) {
			if (id == -1)
				continue;
			PACKET_SENDER(id)->SendPlayerRespawnPacket(m_matchID, true);
		}

		//Kill Golde for Boss
		if (_killerID < NPC_ID) {
			std::shared_ptr<CClient> boss = CObjectMgr::GetInstance()->GetClient(_killerID);
			int increasedGold = boss->GetGold() + ClientInfos::HERO_KILL_GOLD;
			boss->SetGold(increasedGold);
			boss->GetPacketSender()->SendGoldPacket(boss->GetMatchId(), increasedGold);
		}
	}
}
