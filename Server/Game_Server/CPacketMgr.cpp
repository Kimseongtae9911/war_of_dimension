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
		m_packetfunc.insert({ CS_MOVE ,[this](BASE_PACKET* p, std::shared_ptr<CClient> c) {MovePacket(p, c); } });
		m_packetfunc.insert({ CS_LOGIN, [this](BASE_PACKET* p, std::shared_ptr<CClient> c) {LoginPacket(p, c); } });
		m_packetfunc.insert({ CS_ROTATE, [this](BASE_PACKET* p, std::shared_ptr<CClient> c) {RotatePacket(p, c); } });
		m_packetfunc.insert({ CS_SKILL, [this](BASE_PACKET* p, std::shared_ptr<CClient> c) {SkillPacket(p, c); } });
		m_packetfunc.insert({ CS_SKILL_SELECT, [this](BASE_PACKET* p, std::shared_ptr<CClient> c) {SkillSelectPacket(p, c); } });
		m_packetfunc.insert({ CS_READY, [this](BASE_PACKET* p, std::shared_ptr<CClient> c) {ReadyPacket(p, c); } });
		m_packetfunc.insert({ CS_JOB_SELECT, [this](BASE_PACKET* p, std::shared_ptr<CClient> c) {JobSelectPacket(p, c); } });
		m_packetfunc.insert({ CS_CHAT, [this](BASE_PACKET* p, std::shared_ptr<CClient> c) {ChatPacket(p, c); } });
		m_packetfunc.insert({ CS_SKILL_FINISH, [this](BASE_PACKET* p, std::shared_ptr<CClient> c) {SkillFinishPacket(p, c); } });
		m_packetfunc.insert({ CS_LOAD_COMPLETE, [this](BASE_PACKET* p, std::shared_ptr<CClient> c) {LoadCompletePacket(p, c); } });
		m_packetfunc.insert({ CS_TEST_INGAME, [this](BASE_PACKET* p, std::shared_ptr<CClient> c) {TestIngamePacket(p, c); } });
		m_packetfunc.insert({ CS_TEST_INGAME2, [this](BASE_PACKET* p, std::shared_ptr<CClient> c) {TestIngamePacket2(p, c); } });
		m_packetfunc.insert({ CS_JUMP, [this](BASE_PACKET* p, std::shared_ptr<CClient> c) {JumpPacket(p, c); } });
		m_packetfunc.insert({ CS_TELEPORT, [this](BASE_PACKET* p , std::shared_ptr<CClient> c) {TeleportPacket(p, c); } });
		m_packetfunc.insert({ CS_MINION_PATH, [this](BASE_PACKET* p , std::shared_ptr<CClient> c) {MinionPathPacket(p, c); } });
		m_packetfunc.insert({ CS_TOWER_ACTIVATE, [this](BASE_PACKET* p , std::shared_ptr<CClient> c) {TowerActivatePacket(p, c); } });
		m_packetfunc.insert({ CS_NPC_ATTACK_FINISH, [this](BASE_PACKET* p , std::shared_ptr<CClient> c) {NpcAttackFinishPacket(p, c); } });
		m_packetfunc.insert({ CS_STAT_SELECT, [this](BASE_PACKET* p , std::shared_ptr<CClient> c) {StatSelectPacket(p, c); } });
		m_packetfunc.insert({ CS_BUY_ITEM, [this](BASE_PACKET* p , std::shared_ptr<CClient> c) {BuyItemPacket(p, c); } });
		m_packetfunc.insert({ CS_BUY_STAT, [this](BASE_PACKET* p , std::shared_ptr<CClient> c) {BuyStatPacket(p, c); } });
		m_packetfunc.insert({ CS_USE_ITEM, [this](BASE_PACKET* p , std::shared_ptr<CClient> c) {UseItemPacket(p, c); } });
		m_packetfunc.insert({ CS_DEBUG_GOLD, [this](BASE_PACKET* p , std::shared_ptr<CClient> c) {DebugGoldPacket(p, c); } });
		m_packetfunc.insert({ CS_RTT, [this](BASE_PACKET* p , std::shared_ptr<CClient> c) {RTTPacket(p, c); } });
		
		m_packetfunc.insert({ CS_DUMMY_CLIENT, [this](BASE_PACKET* p , std::shared_ptr<CClient> c) {DummyClientPacket(p, c); } });

		return true;
	}

	bool CPacketMgr::Release()
	{
		return true;
	}

	void CPacketMgr::Packet_Exec(BASE_PACKET* packet, std::shared_ptr<CClient> client)
	{	
		if (true == m_packetfunc.contains(packet->type)) {
			m_packetfunc[packet->type](packet, client);
		}
		else {
			LogPrinter::PrintMsg(static_cast<int>(packet->type) + ": Undefined Packet");
		}
	}

	void CPacketMgr::LoginPacket(BASE_PACKET* packet, std::shared_ptr<CClient> client)
	{
		CS_LOGIN_PACKET* p = reinterpret_cast<CS_LOGIN_PACKET*>(packet);

		UserDataFromLobby data = CObjectMgr::GetInstance()->GetUserData(p->name);
		client->SetMatchNum(data.matchNum);

		auto& match = CMatchMgr::GetInstance()->GetMatch(data.matchNum);
		match.ClientConnect(p, client);
	}

	void CPacketMgr::SkillSelectPacket(BASE_PACKET* packet, std::shared_ptr<CClient> client)
	{
		CS_SKILL_SELECT_PACKET* p = reinterpret_cast<CS_SKILL_SELECT_PACKET*>(packet);
		auto& match = CMatchMgr::GetInstance()->GetMatch(client->GetMatchNum());
		match.SelectSkill(p, client);
	}

	void CPacketMgr::ReadyPacket(BASE_PACKET* packet, std::shared_ptr<CClient> client)
	{
		CS_READY_PACKET* p = reinterpret_cast<CS_READY_PACKET*>(packet);

		auto& match = CMatchMgr::GetInstance()->GetMatch(client->GetMatchNum());
		match.SetReady(p);
	}

	void CPacketMgr::JobSelectPacket(BASE_PACKET* packet, std::shared_ptr<CClient> client)
	{
		CS_JOB_SELECT_PACKET* p = reinterpret_cast<CS_JOB_SELECT_PACKET*>(packet);
		auto& match = CMatchMgr::GetInstance()->GetMatch(client->GetMatchNum());
		match.SelectJob(p, client);
	}

	void CPacketMgr::ChatPacket(BASE_PACKET* packet, std::shared_ptr<CClient> client)
	{
		CS_CHAT_PACKET* p = reinterpret_cast<CS_CHAT_PACKET*>(packet);

		//Don't have to send chat packet to boss player(id = 3)
		const auto& match = CMatchMgr::GetInstance()->GetMatch(client->GetMatchNum());
		const auto& clientIds = match.GetClientIds();
		for (int i = 0; i < clientIds.size() - 1; ++i)
		{
			if (i == p->id)
				continue;

			PACKET_SENDER(clientIds[i])->SendChatPacket(p->name, p->chat);
		}
	}

	void CPacketMgr::StatSelectPacket(BASE_PACKET* packet, std::shared_ptr<CClient> client)
	{
		CS_STAT_SELECT_PACKET* p = reinterpret_cast<CS_STAT_SELECT_PACKET*>(packet);
		auto& match = CMatchMgr::GetInstance()->GetMatch(client->GetMatchNum());
		match.SelectStat(p, client);
	}

	void CPacketMgr::LoadCompletePacket(BASE_PACKET* packet, std::shared_ptr<CClient> client)
	{
		CS_LOAD_COMPLETE_PACKET* p = reinterpret_cast<CS_LOAD_COMPLETE_PACKET*>(packet);
		auto& match = CMatchMgr::GetInstance()->GetMatch(client->GetMatchNum());
		match.LoadComplete(p, client->GetMatchId());
	}

	void CPacketMgr::RotatePacket(BASE_PACKET* packet, std::shared_ptr<CClient> client)
	{
		CS_ROTATE_PACKET* p = reinterpret_cast<CS_ROTATE_PACKET*>(packet);

		client->SetLook(vec3(p->lookX, p->lookY, p->lookZ));
		client->SetRight(vec3(p->rightX, p->rightY, p->rightZ));
	}

	void CPacketMgr::SkillPacket(BASE_PACKET* packet, std::shared_ptr<CClient> client)
	{
		CS_SKILL_PACKET* p = reinterpret_cast<CS_SKILL_PACKET*>(packet);

		if (p->onOff) {			
			CSkillHandlerFactory::GetInstance()->Handle(client->GetSkillNum(p->skillType - 1), client);
			client->SetSkillLastUsedTime(static_cast<int>(p->skillType) - 1);

			for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(client->GetMatchNum())) {
				if (id == -1)
					continue;
				CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendSkillPacket(p->id, p->skillType, client->GetSkillNum(p->skillType - 1), true);
			}
		}
		else {			
			if ((client->GetStatus()->skillBuff == SKILL_BUFF::SILENCE && p->skillType != 1) || client->IsDead()) {
				return;
			}

			if (CGameMgr::GetInstance()->CheckCoolTime(client, p->skillType) && !client->GetJump() && !client->GetTeleport()) {
				if (p->skillType != 1 && client->GetStatus()->healthMana.GetCurMp() < client->GetMpConsumption(static_cast<int>(p->skillType) - 1)) {
					return;
				}
				client->SetUsingSkill(true);
				client->SetSkillLastUsedTime(static_cast<int>(p->skillType) - 1);
				CSkillHandlerFactory::GetInstance()->Handle(client->GetSkillNum(p->skillType - 1), client);

				if (p->skillType != 1) {
					client->GetStatus()->healthMana.UseMp(client->GetMpConsumption(static_cast<int>(p->skillType) - 1));
				}

				for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(client->GetMatchNum())) {
					if (id == -1)
						continue;
					CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendSkillPacket(p->id, p->skillType, client->GetSkillNum(p->skillType - 1));

					if (p->skillType != 1)
						CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendPlayerHealthManaPacket(client->GetMatchId(), client->GetStatus()->healthMana);
				}
			}
		}
	}

	void CPacketMgr::SkillFinishPacket(BASE_PACKET* packet, std::shared_ptr<CClient> client)
	{
		auto& match = CMatchMgr::GetInstance()->GetMatch(client->GetMatchNum());
		client->SetUsingSkill(false);
	}

	void CPacketMgr::MovePacket(BASE_PACKET* packet, std::shared_ptr<CClient> client)
	{
		CS_MOVE_PACKET* p = reinterpret_cast<CS_MOVE_PACKET*>(packet);

		client->SetDir(p->direction);

		if (p->direction == 0) {
			client->SetVelocity(vec3(0, 0, 0));
		}
	}

	void CPacketMgr::JumpPacket(BASE_PACKET* packet, std::shared_ptr<CClient> client)
	{		
		int jumpNum;
		if (GameUtil::CheckJumpCollision(client->GetPos(), jumpNum) && !client->GetJump()) {
			client->SetJump(true);
			client->SetJumpNum(jumpNum);
		}
	}

	void CPacketMgr::TeleportPacket(BASE_PACKET* packet, std::shared_ptr<CClient> client)
	{
		if (!CGameMgr::GetInstance()->GetTeleport(client->GetMatchNum()))
			return;

		auto& match = CMatchMgr::GetInstance()->GetMatch(client->GetMatchNum());
		match.TeleportStart(client);
	}

	void CPacketMgr::TowerActivatePacket(BASE_PACKET* packet, std::shared_ptr<CClient> client)
	{
		CS_TOWER_ACTIVATE_PACKET* p = reinterpret_cast<CS_TOWER_ACTIVATE_PACKET*>(packet);

		int matchNum = client->GetMatchNum();

		if (CGameMgr::GetInstance()->GetTower(matchNum, p->num)->GetBroken() || CGameMgr::GetInstance()->GetTower(matchNum, p->num)->active)
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

	void CPacketMgr::MinionPathPacket(BASE_PACKET* packet, std::shared_ptr<CClient> client)
	{
		CS_MINION_PATH_PACKET* p = reinterpret_cast<CS_MINION_PATH_PACKET*>(packet);
		CGameMgr::GetInstance()->SetPathNum(client->GetMatchNum(), p->path);
	}

	void CPacketMgr::NpcAttackFinishPacket(BASE_PACKET* packet, std::shared_ptr<CClient> client)
	{		
		CS_NPC_ATTACK_FINISH_PACKET* p = reinterpret_cast<CS_NPC_ATTACK_FINISH_PACKET*>(packet);
		CObjectMgr::GetInstance()->GetNpc(client->GetMatchNum(), p->id)->SetAttack(false);
	}

	void CPacketMgr::BuyItemPacket(BASE_PACKET* packet, std::shared_ptr<CClient> client)
	{
		if (client->GetMatchId() != 3 && DistanceXZ(client->GetPos(), vec3(SHOP_POS_X, 0.0f, SHOP_POS_Z)) > SHOP_DISTANCE)
			return;
		else if (client->GetMatchId() == 3 && DistanceXZ(client->GetPos(), vec3(BOSS_SHOP_POS_X, 0.0f, BOSS_SHOP_POS_Z)) > SHOP_DISTANCE)
			return;

		CS_BUY_ITEM_PACKET* p = reinterpret_cast<CS_BUY_ITEM_PACKET*>(packet);
		const auto BuyItemProcess = [&client, &p](int8_t _buyGold) {
			
			if (client->GetGold() >= _buyGold) {
				for (int i = 0; i < ITEM_NUM; ++i) {
					if (!client->GetItemInfo(i)->exist) {
						client->GetItemInfo(i)->exist = true;
						client->GetItemInfo(i)->SetType(static_cast<ITEMKIND>(p->itemType));
						int changedGold = client->GetGold() - _buyGold;
						client->SetGold(changedGold);
						client->GetPacketSender()->SendGoldPacket(client->GetMatchId(), changedGold);
						break;
					}
				}
			}
			else {
				LogPrinter::PrintMsg("Not Enough Gold to buy item");
			}
		};

		auto it = ItemCsvMgr::ITEMKINDToEItemType.find(static_cast<ITEMKIND>(p->itemType));
		if (it == ItemCsvMgr::ITEMKINDToEItemType.end())
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

	void CPacketMgr::BuyStatPacket(BASE_PACKET* packet, std::shared_ptr<CClient> client)
	{
		if (client->GetMatchId() != 3 && DistanceXZ(client->GetPos(), vec3(SHOP_POS_X, 0.0f, SHOP_POS_Z)) > SHOP_DISTANCE)
			return;
		else if (client->GetMatchId() == 3 && DistanceXZ(client->GetPos(), vec3(BOSS_SHOP_POS_X, 0.0f, BOSS_SHOP_POS_Z)) > SHOP_DISTANCE)
			return;

		CS_BUY_STAT_PACKET* p = reinterpret_cast<CS_BUY_STAT_PACKET*>(packet);
		int matchNum = client->GetMatchNum();
		int statIndex = static_cast<int>(p->statType) - 2;
		if (client->GetGold() < CGameMgr::GetInstance()->GetCurrentPrice(matchNum, statIndex)) {
			return;
		}
		int changedGold = client->GetGold() - CGameMgr::GetInstance()->GetCurrentPrice(matchNum, statIndex);
		client->SetGold(changedGold);
		CGameMgr::GetInstance()->BuyStat(matchNum, statIndex);
		CGameMgr::GetInstance()->SetCurrentPrice(matchNum, statIndex, static_cast<int>(GET_SHOP_PRICE(CGameMgr::GetInstance()->GetCurrentPrice(matchNum, statIndex), CGameMgr::GetInstance()->GetStatLevel(matchNum, statIndex))));
		client->GetPacketSender()->SendGoldPacket(client->GetMatchId(), changedGold);

		switch (static_cast<ITEMKIND>(p->statType)) {
		case ITEMKIND::HP:
			client->GetStatus()->healthMana.SetMaxHp(client->GetStatus()->healthMana.GetMaxHp() + ShopStats::STAT_HP_INCREASE);
			client->GetStatus()->healthMana.SetMaxHp(client->GetStatus()->healthMana.GetMaxHp());
			for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(matchNum)) {
				if (-1 == id)
					continue;
				CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendPlayerHealthManaPacket(client->GetMatchId(), client->GetStatus()->healthMana);
			}
			break;
		case ITEMKIND::MP:
			client->GetStatus()->healthMana.SetMaxMp(client->GetStatus()->healthMana.GetMaxMp() + ShopStats::STAT_MP_INCREASE);
			client->GetStatus()->healthMana.SetCurMp(client->GetStatus()->healthMana.GetMaxMp());
			for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(matchNum)) {
				if (-1 == id)
					continue;
				CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendPlayerHealthManaPacket(client->GetMatchId(), client->GetStatus()->healthMana);
			}
			break;
		case ITEMKIND::ATTACK:
		{
			CStat stat = client->GetStatus()->GetStat();
			stat.strength += ShopStats::STAT_STRENGTH_INCREASE;
			client->GetStatus()->SetStat(stat);
			break;
		}
		case ITEMKIND::MATTACK:
		{
			CStat stat = client->GetStatus()->GetStat();
			stat.magic += ShopStats::STAT_MAGIC_INCREASE;
			client->GetStatus()->SetStat(stat);
			break;
		}
		case ITEMKIND::DEFENSE:
		{
			CStat stat = client->GetStatus()->GetStat();
			stat.armor += ShopStats::STAT_ARMOR_INCREASE;
			client->GetStatus()->SetStat(stat);
			break;
		}
		case ITEMKIND::MDEFENSE:
		{
			CStat stat = client->GetStatus()->GetStat();
			stat.regist += ShopStats::STAT_REGIST_INCREASE;
			client->GetStatus()->SetStat(stat);
			break;
		}
		case ITEMKIND::SPEED:
		{
			CStat stat = client->GetStatus()->GetStat();
			stat.speed += ShopStats::STAT_SPEED_INCREASE;
			client->GetStatus()->SetStat(stat);
			for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(matchNum)) {
				if (-1 == id)
					continue;
				CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendPlayerStatChangePacket(client->GetMatchId(), client->GetStatus()->GetStat());
			}
			break;
		}
		case ITEMKIND::TENACITY:
		{
			CStat stat = client->GetStatus()->GetStat();
			stat.endure += ShopStats::STAT_ENDURE_INCREASE;
			client->GetStatus()->SetStat(stat);
			break;
		}
		case ITEMKIND::CRITICAL:
		{
			CStat stat = client->GetStatus()->GetStat();
			stat.critical += ShopStats::STAT_CRITICAL_INCREASE;
			client->GetStatus()->SetStat(stat);
			break;
		}
		}

	}

	void CPacketMgr::UseItemPacket(BASE_PACKET* packet, std::shared_ptr<CClient> client)
	{
		CS_USE_ITEM_PACKET* p = reinterpret_cast<CS_USE_ITEM_PACKET*>(packet);

		auto& match = CMatchMgr::GetInstance()->GetMatch(client->GetMatchNum());
		match.UseItem(p, client);
	}

	void CPacketMgr::DebugGoldPacket(BASE_PACKET* packet, std::shared_ptr<CClient> client)
	{		
		short gold = client->GetGold() + 10000;
		client->SetGold(gold);		
		client->GetPacketSender()->SendGoldPacket(client->GetMatchId(), gold);
	}

	void CPacketMgr::RTTPacket(BASE_PACKET* packet, std::shared_ptr<CClient> client)
	{
		CS_RTT_PACKET* p = reinterpret_cast<CS_RTT_PACKET*>(packet);

		long long currentTime = std::chrono::high_resolution_clock::now().time_since_epoch().count();
		long long packetDelayTime = (currentTime - p->serverTime) / 2;
		long long timeDifference = p->time - (p->serverTime + packetDelayTime);
		client->GetPacketSender()->SetTimeDifference(timeDifference);
		std::cout << timeDifference << std::endl;
	}

	void CPacketMgr::TestIngamePacket(BASE_PACKET* packet, std::shared_ptr<CClient> client)
	{
		LogPrinter::PrintMsg("TestIngamePacket");
		CS_TEST_INGAME_PACKET* p = reinterpret_cast<CS_TEST_INGAME_PACKET*>(packet);
		
		const auto& userData = CObjectMgr::GetInstance()->GetUserData(client->GetName());

		client->SetMatchNum(userData.matchNum);
		client->SetMatchId(userData.id);

		int matchNum = client->GetMatchNum();
		auto& match = CMatchMgr::GetInstance()->GetMatch(matchNum);
		match.RegisterClient(0, client->GetID());
		
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

	void CPacketMgr::TestIngamePacket2(BASE_PACKET* packet, std::shared_ptr<CClient> client)
	{
		LogPrinter::PrintMsg("TestIngamePacket2");

		int matchNum = client->GetMatchNum();
		auto& match = CMatchMgr::GetInstance()->GetMatch(matchNum);
		match.RegisterClient(0, client->GetID());

		client->SetState(CL_STATE::ST_INGAME);

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
		}
	}

	void CPacketMgr::DummyClientPacket(BASE_PACKET* packet, std::shared_ptr<CClient> client)
	{
		if (m_dummyNums == MAX_PLAYER * MAX_MATCH)
			return;

		client->stateLock.lock();
		client->SetState(CL_STATE::ST_INGAME);
		client->stateLock.unlock();
		client->SetUpdateTime();
		int num = m_dummyNums % 4;
		switch (num) {
		case 0:
		case 1:
		case 2:
			client->SetPos(ClientInfos::HERO_START_POS[num]);
			break;
		case 3:
			client->SetPos(ClientInfos::BOSS_START_POS);
			break;
		}
		client->GetPacketSender()->SendMovePacket(client->GetMatchId(), client->GetPos(), client->GetDir());
		client->GetPacketSender()->SendDummyLoginInfoPacket(m_dummyNums, client->GetPos());
		client->SetMatchId(m_dummyNums);
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