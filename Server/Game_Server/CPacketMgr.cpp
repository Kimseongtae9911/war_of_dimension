#include "pch.h"
#include "CPacketMgr.h"
#include "CSkillHandlerFactory.h"
#include "CSkill.h"
#include "Stats.h"
#include "ClientInfos.h"
#include "NpcInfos.h"

namespace wod_server {
	std::unique_ptr<CPacketMgr> CPacketMgr::m_instance;

	bool CPacketMgr::Initialize()
	{
		m_packetfunc.insert({ CS_MOVE ,[this](BASE_PACKET* _p, std::shared_ptr<CClient> _c) {MovePacket(_p, _c); } });
		m_packetfunc.insert({ CS_LOGIN, [this](BASE_PACKET* _p, std::shared_ptr<CClient> _c) {LoginPacket(_p, _c); } });
		m_packetfunc.insert({ CS_ROTATE, [this](BASE_PACKET* _p, std::shared_ptr<CClient> _c) {RotatePacket(_p, _c); } });
		m_packetfunc.insert({ CS_SKILL, [this](BASE_PACKET* _p, std::shared_ptr<CClient> _c) {SkillPacket(_p, _c); } });
		m_packetfunc.insert({ CS_SKILL_SELECT, [this](BASE_PACKET* _p, std::shared_ptr<CClient> _c) {SkillSelectPacket(_p, _c); } });
		m_packetfunc.insert({ CS_READY, [this](BASE_PACKET* _p, std::shared_ptr<CClient> _c) {ReadyPacket(_p, _c); } });
		m_packetfunc.insert({ CS_JOB_SELECT, [this](BASE_PACKET* _p, std::shared_ptr<CClient> _c) {JobSelectPacket(_p, _c); } });
		m_packetfunc.insert({ CS_CHAT, [this](BASE_PACKET* _p, std::shared_ptr<CClient> _c) {ChatPacket(_p, _c); } });
		m_packetfunc.insert({ CS_SKILL_FINISH, [this](BASE_PACKET* _p, std::shared_ptr<CClient> _c) {SkillFinishPacket(_p, _c); } });
		m_packetfunc.insert({ CS_LOAD_COMPLETE, [this](BASE_PACKET* _p, std::shared_ptr<CClient> _c) {LoadCompletePacket(_p, _c); } });
		m_packetfunc.insert({ CS_TEST_INGAME, [this](BASE_PACKET* _p, std::shared_ptr<CClient> _c) {TestIngamePacket(_p, _c); } });
		m_packetfunc.insert({ CS_TEST_INGAME2, [this](BASE_PACKET* _p, std::shared_ptr<CClient> _c) {TestIngamePacket2(_p, _c); } });
		m_packetfunc.insert({ CS_JUMP, [this](BASE_PACKET* _p, std::shared_ptr<CClient> _c) {JumpPacket(_p, _c); } });
		m_packetfunc.insert({ CS_TELEPORT, [this](BASE_PACKET* _p , std::shared_ptr<CClient> _c) {TeleportPacket(_p, _c); } });
		m_packetfunc.insert({ CS_MINION_PATH, [this](BASE_PACKET* _p , std::shared_ptr<CClient> _c) {MinionPathPacket(_p, _c); } });
		m_packetfunc.insert({ CS_TOWER_ACTIVATE, [this](BASE_PACKET* _p , std::shared_ptr<CClient> _c) {TowerActivatePacket(_p, _c); } });
		m_packetfunc.insert({ CS_NPC_ATTACK_FINISH, [this](BASE_PACKET* _p , std::shared_ptr<CClient> _c) {NpcAttackFinishPacket(_p, _c); } });
		m_packetfunc.insert({ CS_STAT_SELECT, [this](BASE_PACKET* _p , std::shared_ptr<CClient> _c) {StatSelectPacket(_p, _c); } });
		m_packetfunc.insert({ CS_BUY_ITEM, [this](BASE_PACKET* _p , std::shared_ptr<CClient> _c) {BuyItemPacket(_p, _c); } });
		m_packetfunc.insert({ CS_BUY_STAT, [this](BASE_PACKET* _p , std::shared_ptr<CClient> _c) {BuyStatPacket(_p, _c); } });
		m_packetfunc.insert({ CS_USE_ITEM, [this](BASE_PACKET* _p , std::shared_ptr<CClient> _c) {UseItemPacket(_p, _c); } });
		m_packetfunc.insert({ CS_DEBUG_GOLD, [this](BASE_PACKET* _p , std::shared_ptr<CClient> _c) {DebugGoldPacket(_p, _c); } });
		m_packetfunc.insert({ CS_RTT, [this](BASE_PACKET* _p , std::shared_ptr<CClient> _c) {RTTPacket(_p, _c); } });

		m_packetfunc.insert({ CS_DUMMY_CLIENT, [this](BASE_PACKET* _p , std::shared_ptr<CClient> _c) {DummyClientPacket(_p, _c); } });

		return true;
	}

	bool CPacketMgr::Release()
	{
		return true;
	}

	void CPacketMgr::Packet_Exec(BASE_PACKET* _packet, std::shared_ptr<CClient> _client)
	{
		if (true == m_packetfunc.contains(_packet->type)) {
			m_packetfunc[_packet->type](_packet, _client);
		}
		else {
			LogPrinter::PrintMsg(static_cast<int>(_packet->type) + ": Undefined Packet");
		}
	}

	void CPacketMgr::LoginPacket(BASE_PACKET* _packet, std::shared_ptr<CClient> _client)
	{
		CS_LOGIN_PACKET* p = reinterpret_cast<CS_LOGIN_PACKET*>(_packet);

		UserDataFromLobby data = CObjectMgr::GetInstance()->GetUserData(p->name);
        if (data.m_matchNum < 0 || data.m_matchNum >= MAX_MATCH || data.m_id < 0 || data.m_id >= MAX_PLAYER) {
            _client->Disconnect(); return;
        }
		_client->SetMatchNum(data.m_matchNum);

		auto& match = CMatchMgr::GetInstance()->GetMatch(data.m_matchNum);
		match.ClientConnect(p, _client);
	}

	void CPacketMgr::SkillSelectPacket(BASE_PACKET* _packet, std::shared_ptr<CClient> _client)
	{
		CS_SKILL_SELECT_PACKET* p = reinterpret_cast<CS_SKILL_SELECT_PACKET*>(_packet);
		auto& match = CMatchMgr::GetInstance()->GetMatch(_client->GetMatchNum());
		match.SelectSkill(p, _client);
	}

	void CPacketMgr::ReadyPacket(BASE_PACKET* _packet, std::shared_ptr<CClient> _client)
	{
		CS_READY_PACKET* p = reinterpret_cast<CS_READY_PACKET*>(_packet);

		auto& match = CMatchMgr::GetInstance()->GetMatch(_client->GetMatchNum());
		match.SetReady(p);
	}

	void CPacketMgr::JobSelectPacket(BASE_PACKET* _packet, std::shared_ptr<CClient> _client)
	{
		CS_JOB_SELECT_PACKET* p = reinterpret_cast<CS_JOB_SELECT_PACKET*>(_packet);
		auto& match = CMatchMgr::GetInstance()->GetMatch(_client->GetMatchNum());
		match.SelectJob(p, _client);
	}

	void CPacketMgr::ChatPacket(BASE_PACKET* _packet, std::shared_ptr<CClient> _client)
	{
		CS_CHAT_PACKET* p = reinterpret_cast<CS_CHAT_PACKET*>(_packet);

		//Don't have to send chat packet to boss player(id = 3)
		const auto& match = CMatchMgr::GetInstance()->GetMatch(_client->GetMatchNum());
		const auto& clientIds = match.GetClientIds();
		for (int i = 0; i < clientIds.size() - 1; ++i)
		{
			if (i == p->id)
				continue;

			PACKET_SENDER(clientIds[i])->SendChatPacket(p->name, p->chat);
		}
	}

	void CPacketMgr::StatSelectPacket(BASE_PACKET* _packet, std::shared_ptr<CClient> _client)
	{
		CS_STAT_SELECT_PACKET* p = reinterpret_cast<CS_STAT_SELECT_PACKET*>(_packet);
		auto& match = CMatchMgr::GetInstance()->GetMatch(_client->GetMatchNum());
		match.SelectStat(p, _client);
	}

	void CPacketMgr::LoadCompletePacket(BASE_PACKET* _packet, std::shared_ptr<CClient> _client)
	{
		CS_LOAD_COMPLETE_PACKET* p = reinterpret_cast<CS_LOAD_COMPLETE_PACKET*>(_packet);
		auto& match = CMatchMgr::GetInstance()->GetMatch(_client->GetMatchNum());
		match.LoadComplete(p, _client->GetMatchId());
	}

	void CPacketMgr::RotatePacket(BASE_PACKET* _packet, std::shared_ptr<CClient> _client)
	{
		CS_ROTATE_PACKET* p = reinterpret_cast<CS_ROTATE_PACKET*>(_packet);

		_client->SetLook(vec3(p->lookX, p->lookY, p->lookZ));
		_client->SetRight(vec3(p->rightX, p->rightY, p->rightZ));
	}

	void CPacketMgr::SkillPacket(BASE_PACKET* _packet, std::shared_ptr<CClient> _client)
	{
		CS_SKILL_PACKET* p = reinterpret_cast<CS_SKILL_PACKET*>(_packet);

		if (p->onOff) {
			CSkillHandlerFactory::GetInstance()->Handle(_client->GetSkillNum(p->skillType - 1), _client);
			_client->SetSkillLastUsedTime(static_cast<int>(p->skillType) - 1);

			for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(_client->GetMatchNum())) {
				if (id == -1)
					continue;
				CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendSkillPacket(p->id, p->skillType, _client->GetSkillNum(p->skillType - 1), true);
			}
		}
		else {
			if ((_client->GetStatus()->m_skillBuff == SKILL_BUFF::SILENCE && p->skillType != 1) || _client->IsDead()) {
				return;
			}

			if (CGameMgr::GetInstance()->CheckCoolTime(_client, p->skillType) && !_client->GetJump() && !_client->GetTeleport()) {
				if (p->skillType != 1 && _client->GetStatus()->m_healthMana.GetCurMp() < _client->GetMpConsumption(static_cast<int>(p->skillType) - 1)) {
					return;
				}
				_client->SetUsingSkill(true);
				_client->SetSkillLastUsedTime(static_cast<int>(p->skillType) - 1);
				CSkillHandlerFactory::GetInstance()->Handle(_client->GetSkillNum(p->skillType - 1), _client);

				if (p->skillType != 1) {
					_client->GetStatus()->m_healthMana.UseMp(_client->GetMpConsumption(static_cast<int>(p->skillType) - 1));
				}

				for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(_client->GetMatchNum())) {
					if (id == -1)
						continue;
					CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendSkillPacket(p->id, p->skillType, _client->GetSkillNum(p->skillType - 1));

					if (p->skillType != 1)
						CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendPlayerHealthManaPacket(_client->GetMatchId(), _client->GetStatus()->m_healthMana);
				}
			}
		}
	}

	void CPacketMgr::SkillFinishPacket(BASE_PACKET* _packet, std::shared_ptr<CClient> _client)
	{
		auto& match = CMatchMgr::GetInstance()->GetMatch(_client->GetMatchNum());
		_client->SetUsingSkill(false);
	}

	void CPacketMgr::MovePacket(BASE_PACKET* _packet, std::shared_ptr<CClient> _client)
	{
		CS_MOVE_PACKET* p = reinterpret_cast<CS_MOVE_PACKET*>(_packet);

		_client->SetDir(p->direction);

		if (p->direction == 0) {
			_client->SetVelocity(vec3(0, 0, 0));
		}
	}

	void CPacketMgr::JumpPacket(BASE_PACKET* _packet, std::shared_ptr<CClient> _client)
	{
		int jumpNum;
		if (GameUtil::CheckJumpCollision(_client->GetPos(), jumpNum) && !_client->GetJump()) {
			_client->SetJump(true);
			_client->SetJumpNum(jumpNum);
		}
	}

	void CPacketMgr::TeleportPacket(BASE_PACKET* _packet, std::shared_ptr<CClient> _client)
	{
		if (!CGameMgr::GetInstance()->GetTeleport(_client->GetMatchNum()))
			return;

		auto& match = CMatchMgr::GetInstance()->GetMatch(_client->GetMatchNum());
		match.TeleportStart(_client);
	}

	void CPacketMgr::TowerActivatePacket(BASE_PACKET* _packet, std::shared_ptr<CClient> _client)
	{
		CS_TOWER_ACTIVATE_PACKET* p = reinterpret_cast<CS_TOWER_ACTIVATE_PACKET*>(_packet);

		int matchNum = _client->GetMatchNum();

		if (CGameMgr::GetInstance()->GetTower(matchNum, p->num)->GetBroken() || CGameMgr::GetInstance()->GetTower(matchNum, p->num)->m_active)
			return;

		CGameMgr::GetInstance()->ActiveTower(true, matchNum, p->num);
		for (int i = 0; i < MAX_PATH; ++i) {
			if (p->num != i)
				CGameMgr::GetInstance()->ActiveTower(false, matchNum, i);
		}

		for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(matchNum)) {
			if (-1 == id)
				continue;
			CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendStructureStatusChangePacket(p->num);
		}
	}

	void CPacketMgr::MinionPathPacket(BASE_PACKET* _packet, std::shared_ptr<CClient> _client)
	{
		CS_MINION_PATH_PACKET* p = reinterpret_cast<CS_MINION_PATH_PACKET*>(_packet);
		CGameMgr::GetInstance()->SetPathNum(_client->GetMatchNum(), p->path);
	}

	void CPacketMgr::NpcAttackFinishPacket(BASE_PACKET* _packet, std::shared_ptr<CClient> _client)
	{
		CS_NPC_ATTACK_FINISH_PACKET* p = reinterpret_cast<CS_NPC_ATTACK_FINISH_PACKET*>(_packet);
		CObjectMgr::GetInstance()->GetNpc(_client->GetMatchNum(), p->id)->SetAttack(false);
	}

	void CPacketMgr::BuyItemPacket(BASE_PACKET* _packet, std::shared_ptr<CClient> _client)
	{
		if (_client->GetMatchId() != 3 && DistanceXZ(_client->GetPos(), vec3(SHOP_POS_X, 0.0f, SHOP_POS_Z)) > SHOP_DISTANCE)
			return;
		else if (_client->GetMatchId() == 3 && DistanceXZ(_client->GetPos(), vec3(BOSS_SHOP_POS_X, 0.0f, BOSS_SHOP_POS_Z)) > SHOP_DISTANCE)
			return;

		CS_BUY_ITEM_PACKET* p = reinterpret_cast<CS_BUY_ITEM_PACKET*>(_packet);
		const auto BuyItemProcess = [&_client, &p](int8_t _buyGold) {

			if (_client->GetGold() >= _buyGold) {
				for (int i = 0; i < ITEM_NUM; ++i) {
					if (!_client->GetItemInfo(i)->m_exist) {
						_client->GetItemInfo(i)->m_exist = true;
						_client->GetItemInfo(i)->SetType(static_cast<ITEMKIND>(p->itemType));
						int changedGold = _client->GetGold() - _buyGold;
						_client->SetGold(changedGold);
						_client->GetPacketSender()->SendGoldPacket(_client->GetMatchId(), changedGold);
						break;
					}
				}
			}
			else {
				LogPrinter::PrintMsg("Not Enough Gold to buy item");
			}
		};

		auto it = ItemCsvMgr::m_ITEMKINDToEItemType.find(static_cast<ITEMKIND>(p->itemType));
		if (it == ItemCsvMgr::m_ITEMKINDToEItemType.end())
			return;

		switch (it->second)
		{
		case EItemType::HealHp:
		case EItemType::HealMp:
			BuyItemProcess(HEAL_ITEM_GOLD);
			break;
		case EItemType::MaxHp:
		case EItemType::MaxMp:
		case EItemType::StrengthIncrease:
		case EItemType::MagicIncrease:
		case EItemType::DefenseIncrease:
		case EItemType::RegistIncrease:
		case EItemType::SpeedIncrease:
		case EItemType::TenacityIncrease:
		case EItemType::CriticalIncrease:
			BuyItemProcess(STAT_ITEM_GOLD);
			break;

		default:
			break;
		}
	}

	void CPacketMgr::BuyStatPacket(BASE_PACKET* _packet, std::shared_ptr<CClient> _client)
	{
		if (_client->GetMatchId() != 3 && DistanceXZ(_client->GetPos(), vec3(SHOP_POS_X, 0.0f, SHOP_POS_Z)) > SHOP_DISTANCE)
			return;
		else if (_client->GetMatchId() == 3 && DistanceXZ(_client->GetPos(), vec3(BOSS_SHOP_POS_X, 0.0f, BOSS_SHOP_POS_Z)) > SHOP_DISTANCE)
			return;

		CS_BUY_STAT_PACKET* p = reinterpret_cast<CS_BUY_STAT_PACKET*>(_packet);
		int matchNum = _client->GetMatchNum();
		int statIndex = static_cast<int>(p->statType) - 2;
		if (_client->GetGold() < CGameMgr::GetInstance()->GetCurrentPrice(matchNum, statIndex)) {
			return;
		}
		int changedGold = _client->GetGold() - CGameMgr::GetInstance()->GetCurrentPrice(matchNum, statIndex);
		_client->SetGold(changedGold);
		CGameMgr::GetInstance()->BuyStat(matchNum, statIndex);
		CGameMgr::GetInstance()->SetCurrentPrice(matchNum, statIndex, static_cast<int>(GET_SHOP_PRICE(CGameMgr::GetInstance()->GetCurrentPrice(matchNum, statIndex), CGameMgr::GetInstance()->GetStatLevel(matchNum, statIndex))));
		_client->GetPacketSender()->SendGoldPacket(_client->GetMatchId(), changedGold);

		switch (static_cast<ITEMKIND>(p->statType)) {
		case ITEMKIND::HP:
			_client->GetStatus()->m_healthMana.SetMaxHp(_client->GetStatus()->m_healthMana.GetMaxHp() + ShopStats::STAT_HP_INCREASE);
			_client->GetStatus()->m_healthMana.SetMaxHp(_client->GetStatus()->m_healthMana.GetMaxHp());
			for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(matchNum)) {
				if (-1 == id)
					continue;
				CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendPlayerHealthManaPacket(_client->GetMatchId(), _client->GetStatus()->m_healthMana);
			}
			break;
		case ITEMKIND::MP:
			_client->GetStatus()->m_healthMana.SetMaxMp(_client->GetStatus()->m_healthMana.GetMaxMp() + ShopStats::STAT_MP_INCREASE);
			_client->GetStatus()->m_healthMana.SetCurMp(_client->GetStatus()->m_healthMana.GetMaxMp());
			for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(matchNum)) {
				if (-1 == id)
					continue;
				CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendPlayerHealthManaPacket(_client->GetMatchId(), _client->GetStatus()->m_healthMana);
			}
			break;
		case ITEMKIND::ATTACK:
		{
			CStat stat = _client->GetStatus()->GetStat();
			stat.m_strength += ShopStats::STAT_STRENGTH_INCREASE;
			_client->GetStatus()->SetStat(stat);
			break;
		}
		case ITEMKIND::MATTACK:
		{
			CStat stat = _client->GetStatus()->GetStat();
			stat.m_magic += ShopStats::STAT_MAGIC_INCREASE;
			_client->GetStatus()->SetStat(stat);
			break;
		}
		case ITEMKIND::DEFENSE:
		{
			CStat stat = _client->GetStatus()->GetStat();
			stat.m_armor += ShopStats::STAT_ARMOR_INCREASE;
			_client->GetStatus()->SetStat(stat);
			break;
		}
		case ITEMKIND::MDEFENSE:
		{
			CStat stat = _client->GetStatus()->GetStat();
			stat.m_regist += ShopStats::STAT_REGIST_INCREASE;
			_client->GetStatus()->SetStat(stat);
			break;
		}
		case ITEMKIND::SPEED:
		{
			CStat stat = _client->GetStatus()->GetStat();
			stat.m_speed += ShopStats::STAT_SPEED_INCREASE;
			_client->GetStatus()->SetStat(stat);
			for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(matchNum)) {
				if (-1 == id)
					continue;
				CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendPlayerStatChangePacket(_client->GetMatchId(), _client->GetStatus()->GetStat());
			}
			break;
		}
		case ITEMKIND::TENACITY:
		{
			CStat stat = _client->GetStatus()->GetStat();
			stat.m_endure += ShopStats::STAT_ENDURE_INCREASE;
			_client->GetStatus()->SetStat(stat);
			break;
		}
		case ITEMKIND::CRITICAL:
		{
			CStat stat = _client->GetStatus()->GetStat();
			stat.m_critical += ShopStats::STAT_CRITICAL_INCREASE;
			_client->GetStatus()->SetStat(stat);
			break;
		}
		}

	}

	void CPacketMgr::UseItemPacket(BASE_PACKET* _packet, std::shared_ptr<CClient> _client)
	{
		CS_USE_ITEM_PACKET* p = reinterpret_cast<CS_USE_ITEM_PACKET*>(_packet);

		auto& match = CMatchMgr::GetInstance()->GetMatch(_client->GetMatchNum());
		match.UseItem(p, _client);
	}

	void CPacketMgr::DebugGoldPacket(BASE_PACKET* _packet, std::shared_ptr<CClient> _client)
	{
		short gold = _client->GetGold() + 10000;
		_client->SetGold(gold);
		_client->GetPacketSender()->SendGoldPacket(_client->GetMatchId(), gold);
	}

	void CPacketMgr::RTTPacket(BASE_PACKET* _packet, std::shared_ptr<CClient> _client)
	{
		CS_RTT_PACKET* p = reinterpret_cast<CS_RTT_PACKET*>(_packet);

		long long currentTime = std::chrono::high_resolution_clock::now().time_since_epoch().count();
		long long packetDelayTime = (currentTime - p->serverTime) / 2;
		long long timeDifference = p->time - (p->serverTime + packetDelayTime);
		_client->GetPacketSender()->SetTimeDifference(timeDifference);
		std::cout << timeDifference << std::endl;
	}

	void CPacketMgr::TestIngamePacket(BASE_PACKET* _packet, std::shared_ptr<CClient> _client)
	{
		LogPrinter::PrintMsg("TestIngamePacket");
		CS_TEST_INGAME_PACKET* p = reinterpret_cast<CS_TEST_INGAME_PACKET*>(_packet);

		const auto& userData = CObjectMgr::GetInstance()->GetUserData(_client->GetName());

		_client->SetMatchNum(userData.m_matchNum);
		_client->SetMatchId(userData.m_id);

		int matchNum = _client->GetMatchNum();
		auto& match = CMatchMgr::GetInstance()->GetMatch(matchNum);
		match.RegisterClient(0, _client->GetID());

		/*client->SetState(CL_STATE::ST_INGAME);

		client->SetUpdateTime();
		client->SetPos(ClientInfos::BOSS_START_POS);
		client->GetPacketSender()->SendMovePacket(client->GetMatchId(), client->GetPos(), client->GetDir());
		client->SetInitializeStat(0, 0, 0, 0, 0, 0, 0, 0, 0);
		if (client->GetMatchId() == 3) {
			client->InitializeBoundingBox(GameUtil::GetBossPlayerInitBB().offset, GameUtil::GetBossPlayerInitBB().extent, PLAYER_SCALE);
		}
		else {
			client->InitializeBoundingBox(GameUtil::GetPlayerInitBB().offset, GameUtil::GetPlayerInitBB().extent, PLAYER_SCALE);
		}
		client->GetPacketSender()->SendPlayerStatChangePacket(client->GetMatchId(), client->GetStatus()->GetStat());
#ifdef WITH_DATABASE
		client->GetPacketSender()->SendModelCustomizePacket(client->GetMatchId(), ModelCustomize(0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
			0, -1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0));
#else
		client->GetPacketSender()->SendModelCustomizePacket(client->GetMatchId(), ModelCustomize(0, 1, 0, 0, 1, 1, 3, 1, 0, 1, 0, 0, 1, 1, 1,
			1, 0, 1, 3, 2, 2, 2, 2, 1, 1, 1, 1, 1));
#endif


		client->SetSkillCoolTime(0, 0);
		client->SetMpConsumption(0, 0);
		for (int i = 1; i < MAX_SKILL + 1; ++i) {
			client->SetSkillCoolTime(i, GameUtil::GetCoolTime(client->GetPlayerJob(), client->GetSkillNum(i)));
			client->SetMpConsumption(i, GameUtil::GetMpConsumption(client->GetPlayerJob(), client->GetSkillNum(i)));
		}
		client->GetPacketSender()->SendCoolTimePacket(client->GetSkillCoolTime(0), client->GetSkillCoolTime(1), client->GetSkillCoolTime(2), client->GetSkillCoolTime(3), client->GetSkillCoolTime(4));

		client->GetStatus()->healthMana.SetMaxHp(HeroStats::INIT_HP);
		client->GetStatus()->healthMana.SetCurHp(HeroStats::INIT_HP);
		client->GetStatus()->healthMana.SetMaxMp(HeroStats::INIT_MP);
		client->GetStatus()->healthMana.SetCurMp(HeroStats::INIT_MP);
		client->GetPacketSender()->SendPlayerHealthManaPacket(client->GetMatchId(), client->GetStatus()->healthMana);
		for (int i = 0; i < PATH_NUM; ++i) {
			client->GetPacketSender()->SendStructureStatChangePacket(i, CGameMgr::GetInstance()->GetTower(matchNum, i)->GetMaxHp(), CGameMgr::GetInstance()->GetTower(matchNum, i)->GetCurHp());
		}
		client->GetPacketSender()->SendStructureStatChangePacket(PATH_NUM, CGameMgr::GetInstance()->GetNexus(matchNum)->GetMaxHp(), CGameMgr::GetInstance()->GetNexus(matchNum)->GetCurHp());
		client->GetPacketSender()->SendStructureStatusChangePacket(3);

		CGameMgr::GetInstance()->ActiveTower(true, matchNum, 3);

		CGameMgr::GetInstance()->SetLastTime(matchNum);
		network::GetInstance()->RegisterTimerEvent({ matchNum, TimeUtil::CurTime(), EVENT_TYPE::EV_MATCH_UPDATE, -1 });

		if (!testOnce) {
			testOnce = true;
			auto respawnTime = NpcCsvMgr::GetInstance()->GetNpcCsv(ENpcType::Minion)->RespawnTime;
			network::GetInstance()->InitializeMonster(matchNum);
		}*/
	}

	void CPacketMgr::TestIngamePacket2(BASE_PACKET* _packet, std::shared_ptr<CClient> _client)
	{
		LogPrinter::PrintMsg("TestIngamePacket2");

		int matchNum = _client->GetMatchNum();
		auto& match = CMatchMgr::GetInstance()->GetMatch(matchNum);
		match.RegisterClient(0, _client->GetID());

		_client->SetState(CL_STATE::ST_INGAME);

		_client->SetUpdateTime();
		_client->SetPos(ClientInfos::BOSS_START_POS);
		_client->GetPacketSender()->SendMovePacket(_client->GetMatchId(), _client->GetPos(), _client->GetDir());
		_client->SetInitializeStat(0, 0, 0, 0, 0, 0, 0, 0, 0);
		if (_client->GetMatchId() == 3) {
			_client->InitializeBoundingBox(GameUtil::GetBossPlayerInitBB().m_offset, GameUtil::GetBossPlayerInitBB().m_extent, PLAYER_SCALE);
		}
		else {
			_client->InitializeBoundingBox(GameUtil::GetPlayerInitBB().m_offset, GameUtil::GetPlayerInitBB().m_extent, PLAYER_SCALE);
		}
		_client->GetPacketSender()->SendPlayerStatChangePacket(_client->GetMatchId(), _client->GetStatus()->GetStat());
#ifdef WITH_DATABASE
		client->GetPacketSender()->SendModelCustomizePacket(client->GetMatchId(), ModelCustomize(0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
			0, -1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0));
#else
		_client->GetPacketSender()->SendModelCustomizePacket(_client->GetMatchId(), ModelCustomize(0, 1, 0, 0, 1, 1, 3, 1, 0, 1, 0, 0, 1, 1, 1,
			1, 0, 1, 3, 2, 2, 2, 2, 1, 1, 1, 1, 1));
#endif


		_client->SetSkillCoolTime(0, 0);
		_client->SetMpConsumption(0, 0);
		for (int i = 1; i < MAX_SKILL + 1; ++i) {
			_client->SetSkillCoolTime(i, GameUtil::GetCoolTime(_client->GetPlayerJob(), _client->GetSkillNum(i)));
			_client->SetMpConsumption(i, GameUtil::GetMpConsumption(_client->GetPlayerJob(), _client->GetSkillNum(i)));
		}
		_client->GetPacketSender()->SendCoolTimePacket(_client->GetSkillCoolTime(0), _client->GetSkillCoolTime(1), _client->GetSkillCoolTime(2), _client->GetSkillCoolTime(3), _client->GetSkillCoolTime(4));

		_client->GetStatus()->m_healthMana.SetMaxHp(HeroStats::INIT_HP);
		_client->GetStatus()->m_healthMana.SetCurHp(HeroStats::INIT_HP);
		_client->GetStatus()->m_healthMana.SetMaxMp(HeroStats::INIT_MP);
		_client->GetStatus()->m_healthMana.SetCurMp(HeroStats::INIT_MP);
		_client->GetPacketSender()->SendPlayerHealthManaPacket(_client->GetMatchId(), _client->GetStatus()->m_healthMana);
		for (int i = 0; i < PATH_NUM; ++i) {
			_client->GetPacketSender()->SendStructureStatChangePacket(i, CGameMgr::GetInstance()->GetTower(matchNum, i)->GetMaxHp(), CGameMgr::GetInstance()->GetTower(matchNum, i)->GetCurHp());
		}
		_client->GetPacketSender()->SendStructureStatChangePacket(PATH_NUM, CGameMgr::GetInstance()->GetNexus(matchNum)->GetMaxHp(), CGameMgr::GetInstance()->GetNexus(matchNum)->GetCurHp());
		_client->GetPacketSender()->SendStructureStatusChangePacket(3);

		CGameMgr::GetInstance()->ActiveTower(true, matchNum, 3);

		CGameMgr::GetInstance()->SetLastTime(matchNum);
		network::GetInstance()->RegisterTimerEvent({ matchNum, TimeUtil::CurTime(), EVENT_TYPE::EV_MATCH_UPDATE, -1 });

		if (!m_testOnce) {
			m_testOnce = true;
			auto respawnTime = NpcCsvMgr::GetInstance()->GetNpcCsv(ENpcType::Minion)->RespawnTime;
			network::GetInstance()->InitializeMonster(matchNum);
		}
	}

	void CPacketMgr::DummyClientPacket(BASE_PACKET* _packet, std::shared_ptr<CClient> _client)
	{
		if (m_dummyNums == MAX_PLAYER * MAX_MATCH)
			return;

		_client->m_stateLock.lock();
		_client->SetState(CL_STATE::ST_INGAME);
		_client->m_stateLock.unlock();
		_client->SetUpdateTime();
		int num = m_dummyNums % 4;
		switch (num) {
		case 0:
		case 1:
		case 2:
			_client->SetPos(ClientInfos::HERO_START_POS[num]);
			break;
		case 3:
			_client->SetPos(ClientInfos::BOSS_START_POS);
			break;
		}
		_client->GetPacketSender()->SendMovePacket(_client->GetMatchId(), _client->GetPos(), _client->GetDir());
		_client->GetPacketSender()->SendDummyLoginInfoPacket(m_dummyNums, _client->GetPos());
		_client->SetMatchId(m_dummyNums);
		m_dummyNums++;

		if (m_dummyNums % 4 == 0) {
			CGameMgr::GetInstance()->ActiveTower(true, m_matchNum, 3);

			CGameMgr::GetInstance()->SetLastTime(m_matchNum);
			network::GetInstance()->RegisterTimerEvent({ m_matchNum, TimeUtil::CurTime(), EVENT_TYPE::EV_MATCH_UPDATE, -1 });
			auto respawnTime = NpcCsvMgr::GetInstance()->GetNpcCsv(ENpcType::Minion)->RespawnTime;
			network::GetInstance()->InitializeMonster(m_matchNum);
			m_matchNum++;
		}
	}

}