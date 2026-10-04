#include "pch.h"
#include "CMatch.h"
#include "ClientInfos.h"
#include "Stats.h"

namespace wod_server {
	void CMatch::SetReady(const CS_READY_PACKET* packet)
	{
		m_clientReady[packet->id] = packet->ready;

		for (int id : m_clientid) {
			if (id == -1)
				continue;

			PACKET_SENDER(id)->SendReadyPacket(packet->id, packet->ready);
		}
	}

	void CMatch::SelectSkill(const CS_SKILL_SELECT_PACKET* packet, std::shared_ptr<CClient> client)
	{
		if (client->GetMatchId() == 3) {
			client->SetSkillNum(packet->storage + 1, packet->skill - (21 - 96 - 1));
		}
		else
			client->SetSkillNum(packet->storage + 1, packet->skill);

		for (int id : m_clientid) {
			if (-1 == id)
				continue;

			if (packet->id == 3) {
				PACKET_SENDER(id)->SendSelectSkillPacket(packet->id, packet->storage, packet->skill + 1);
			}
			else {
				PACKET_SENDER(id)->SendSelectSkillPacket(packet->id, packet->storage, packet->skill);
			}
		}
	}

	void CMatch::SelectJob(const CS_JOB_SELECT_PACKET* packet, std::shared_ptr<CClient> client)
	{
		client->SetPlayerJob(packet->job);
		client->SetSkillNum(0, packet->job + SKILL_OFFSET);

		for (int id : m_clientid) {
			if (id == -1)
				continue;

			PACKET_SENDER(id)->SendJobSelectPacket(packet->id, packet->job);
		}
	}

	void CMatch::SelectStat(const CS_STAT_SELECT_PACKET* packet, std::shared_ptr<CClient> client)
	{
		client->SetInitializeStat(packet->hp, packet->mp, packet->attack, packet->magic_attack, 
			packet->defense, packet->magic_defense, packet->speed, packet->tenacity, packet->critical);
	}

	void CMatch::LoadingUpdate(int matchNum)
	{
		Update();

		if (!IsLoadComplete())
			return;

		// 스킬 쿨타임, 마나 소모량 세팅
		for (int id : m_clientid) 
		{
			if (id == -1)
				continue;

			std::shared_ptr<CClient> player = CObjectMgr::GetInstance()->GetClient(id);
			player->SetSkillCoolTime(0, 0);
			player->SetMpConsumption(0, 0);
			for (int i = 1; i < MAX_SKILL + 1; ++i) {
				player->SetSkillCoolTime(i, GameUtil::GetCoolTime(player->GetPlayerJob(), player->GetSkillNum(i)));
				player->SetMpConsumption(i, GameUtil::GetMpConsumption(player->GetPlayerJob(), player->GetSkillNum(i)));
			}
			player->GetPacketSender()->SendCoolTimePacket(player->GetSkillCoolTime(0), player->GetSkillCoolTime(1), player->GetSkillCoolTime(2), player->GetSkillCoolTime(3), player->GetSkillCoolTime(4));
		}

		//Initialize HP, Mp
		for (int i = 0; i < MAX_PLAYER; ++i) {
			if (-1 == m_clientid[i])
				continue;

			if (i == 3) {
				std::shared_ptr<CClient> boss = CObjectMgr::GetInstance()->GetClient(m_clientid[i]);
				boss->GetStatus()->healthMana.SetMaxHp(BossStats::INIT_HP);
				boss->GetStatus()->healthMana.SetCurHp(BossStats::INIT_HP);
				boss->GetStatus()->healthMana.SetMaxMp(BossStats::INIT_MP);
				boss->GetStatus()->healthMana.SetCurMp(BossStats::INIT_MP);
				if (boss->GetPlayerJob() == 4) {
					//Ogre
					boss->InitializeBoundingBox(GameUtil::GetBossPlayerInitBB().offset, GameUtil::GetBossPlayerInitBB().extent, OGRE_SCALE);
				}
				else {
					//Programmer
					boss->InitializeBoundingBox(GameUtil::GetPlayerInitBB().offset, GameUtil::GetPlayerInitBB().extent, PLAYER_SCALE);
				}
			}
			else {
				std::shared_ptr<CClient> hero = CObjectMgr::GetInstance()->GetClient(m_clientid[i]);
				hero->GetStatus()->healthMana.SetMaxHp(HeroStats::INIT_HP);
				hero->GetStatus()->healthMana.SetCurHp(HeroStats::INIT_HP);
				hero->GetStatus()->healthMana.SetMaxMp(HeroStats::INIT_MP);
				hero->GetStatus()->healthMana.SetCurMp(HeroStats::INIT_MP);
				hero->InitializeBoundingBox(GameUtil::GetPlayerInitBB().offset, GameUtil::GetPlayerInitBB().extent, PLAYER_SCALE);
			}

			for (int id : m_clientid) {
				if (-1 == id)
					continue;
				
				PACKET_SENDER(id)->SendPlayerHealthManaPacket(i, CObjectMgr::GetInstance()->GetClient(m_clientid[i])->GetStatus()->healthMana);
			}

			//Initialize Packet for Tower and Nexus
			for (int j = 0; j < PATH_NUM; ++j) {
				PACKET_SENDER(m_clientid[i])->SendStructureStatChangePacket(j, CGameMgr::GetInstance()->GetTower(matchNum, j)->GetMaxHp(), CGameMgr::GetInstance()->GetTower(matchNum, j)->GetCurHp());
			}
			PACKET_SENDER(m_clientid[i])->SendStructureStatChangePacket(PATH_NUM, CGameMgr::GetInstance()->GetNexus(matchNum)->GetMaxHp(), CGameMgr::GetInstance()->GetNexus(matchNum)->GetCurHp());
			PACKET_SENDER(m_clientid[i])->SendStructureStatusChangePacket(3);
		}

		//Start Game
		for (int i = 0; i < MAX_PLAYER; ++i) {
			std::shared_ptr<CClient> player = CObjectMgr::GetInstance()->GetClient(m_clientid[i]);
			for (int j = 0; j < MAX_PLAYER; ++j) {
				if (CMatchMgr::GetInstance()->GetMatch(matchNum).m_clientid[i] == -1)
					return;
				std::shared_ptr<CClient> otherClient = CObjectMgr::GetInstance()->GetClient(m_clientid[j]);
				player->GetPacketSender()->SendModelCustomizePacket(otherClient->GetMatchId(), otherClient->GetModelCustomize());
				player->GetPacketSender()->SendPlayerStatChangePacket(otherClient->GetMatchId(), otherClient->GetStatus()->GetStat());
			}
		}

		CGameMgr::GetInstance()->ActiveTower(true, matchNum, 3);
		CGameMgr::GetInstance()->SetLastTime(matchNum);
		network::GetInstance()->InitializeMonster(matchNum);
		network::GetInstance()->RegisterTimerEvent({ matchNum, TimeUtil::CurTime(), EVENT_TYPE::EV_MATCH_UPDATE, -1 });
	}

	void CMatch::ConnectUpdate(int matchNum)
	{
		Update();

		// 모든 클라인언트 로딩 완료 체크
		if (std::ranges::all_of(m_clientid, [](int id) {return id != -1; }))
		{
			for (const int id : m_clientid) 
			{
				const auto& matchClient = CObjectMgr::GetInstance()->GetClient(id);
				matchClient->GetPacketSender()->SendModelCustomizePacket(matchClient->GetMatchId(), matchClient->GetModelCustomize());
				for (auto otherId : m_clientid) 
				{
					if (id == otherId)
						continue;

					const auto& otherClient = CObjectMgr::GetInstance()->GetClient(otherId);
					matchClient->GetPacketSender()->SendAddPlayerPacket(otherClient->GetMatchId(), otherClient->GetLook(), otherClient->GetRight(), otherClient->GetModelCustomize());
				}
			}

			m_updateTime = TimeUtil::CurTime();
			network::GetInstance()->RegisterTimerEvent(TIMER_EVENT(matchNum, TimeUtil::CurTime(), EVENT_TYPE::EV_READY_UPDATE, -1));

			return;
		}

		network::GetInstance()->RegisterTimerEvent({ matchNum, TimeUtil::PassedTimeMSec(1000), EVENT_TYPE::EV_CONNECT_UPDATE, -1 });		
	}

	void CMatch::ClientConnect(const CS_LOGIN_PACKET* packet, std::shared_ptr<CClient> client)
	{
		UserDataFromLobby data = CObjectMgr::GetInstance()->GetUserData(packet->name);

		client->SetName(packet->name);
		client->SetMatchNum(data.matchNum);
		RegisterClient(data.id, client->GetID());
		client->SetMatchId(data.id);
		client->SetModelCustomize(data.model);

		LogPrinter::PrintMsg(std::string(packet->name) + " Connected");

		client->GetPacketSender()->SendLoginPacket();
	}

	void CMatch::ReadyUpdate(int matchNum)
	{
		auto curTime = TimeUtil::CurTime();
		auto elapsedTime = std::chrono::duration_cast<std::chrono::microseconds>(curTime - m_updateTime).count() * 0.000001f;
		m_readyTime -= elapsedTime;
		m_updateTime = curTime;

		Update();

		// 게임시작
		if (IsAllReady() || m_readyTime <= 0.f)
		{
			for (const auto id : m_clientid)
				CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendGameStartPacket();

			m_sceneType = ESceneType::LoadingScene;

			CGameMgr::GetInstance()->SkillAutoSelect(matchNum);

			//Initialize Client Position
			for (int i = 0; i < MAX_PLAYER; ++i)
				CObjectMgr::GetInstance()->GetClient(m_clientid[i])->SetPos(ClientInfos::HERO_START_POS[i]);

			CObjectMgr::GetInstance()->GetClient(m_clientid[3])->SetPos(ClientInfos::BOSS_START_POS);

			for (int i = 0; i < MAX_PLAYER; ++i) {
				const auto& client = CObjectMgr::GetInstance()->GetClient(m_clientid[i]);
				for (int j = 0; j < MAX_PLAYER; ++j) {
					PACKET_SENDER(m_clientid[j])->SendMovePacket(i, client->GetPos(), client->GetDir());
				}
			}

			network::GetInstance()->RegisterTimerEvent({ matchNum, std::chrono::system_clock::now(), EVENT_TYPE::EV_LOADING_UPDATE, 0 });
			return;
		}

		// 준비시간 업데이트
		for (const auto id : m_clientid)
			PACKET_SENDER(id)->SendGameTimePacket(static_cast<int>(::ceil(m_readyTime)), 0);

		network::GetInstance()->RegisterTimerEvent({ matchNum, TimeUtil::PassedTimeMSec(50), EVENT_TYPE::EV_READY_UPDATE, 0 });
	}

	void CMatch::TeleportStart(std::shared_ptr<CClient> client)
	{
		int teleport;
		if (!GameUtil::CheckTeleportCollision(client->GetPos(), teleport) || client->GetTeleport() || !client->CheckTeleportCoolTime())
			return;

		client->SetTeleport(true);
		client->SetPos(TELEPORT_POS[teleport]);
		client->SetTeleportNum(teleport);
		client->SetTeleportLastUsedTime();
		client->GetPacketSender()->SendTeleportActivePacket(false);

		for (int id : m_clientid) {
			if (id == -1)
				continue;
			PACKET_SENDER(id)->SendTeleportPacket(client->GetMatchId(), false);
		}		
	}

	void CMatch::UseItem(const CS_USE_ITEM_PACKET* packet, std::shared_ptr<CClient> client)
	{
		if (!client->GetItemInfo(packet->itemNum)->exist)
			return;

		auto it = ItemCsvMgr::ITEMKINDToEItemType.find(client->GetItemInfo(packet->itemNum)->GetType());
		if (it == ItemCsvMgr::ITEMKINDToEItemType.end())
			return;

		if (false == CItemFactory::UseItem(it->second, client)) {
			LogPrinter::PrintMsg("Failed To Use Item");
			return;
		}

		client->GetItemInfo(packet->itemNum)->exist = false;
		client->GetItemInfo(packet->itemNum)->SetType(ITEMKIND::NONE);
	}

	void CMatch::InGameUpdate(int matchNum)
	{
		// id == match number
		if (-1 == CGameMgr::GetInstance()->IsGameOver(matchNum)) {
			CNetworkMgr::GetInstance()->RegisterTimerEvent({ matchNum, TimeUtil::NextFrameTime(), EVENT_TYPE::EV_MATCH_UPDATE, -1 });			
		}
		else {
			CNetworkMgr::GetInstance()->RegisterTimerEvent({ matchNum, std::chrono::system_clock::now() + std::chrono::seconds(10), EVENT_TYPE::EV_MATCH_FINISH, -1 });

			constexpr short INIT_EARN_TOKEN = 10;

			bool heroWin = static_cast<bool>(CGameMgr::GetInstance()->IsGameOver(matchNum));
			short earnToken = INIT_EARN_TOKEN + static_cast<int>(CGameMgr::GetInstance()->GetGameTime(matchNum) / 30.0f);

			std::time_t t = time(nullptr);
			std::tm time;
			gmtime_s(&time, &t);
			char curTime[TIME_SIZE];
			strftime(curTime, sizeof(curTime), "%Y-%m-%d %X", &time);

			for (int i = 0; i < MAX_PLAYER - 1; ++i) {
				GL_TRANSACTIONS_PACKET p;
				p.size = sizeof(p);
				p.type = GL_TRANSACTIONS;
				memcpy_s(p.name, NAME_SIZE, CObjectMgr::GetInstance()->GetClient(m_clientid[i])->GetName().c_str(), NAME_SIZE);
				p.token = earnToken + INIT_EARN_TOKEN * heroWin;
				memcpy_s(p.time, TIME_SIZE, curTime, TIME_SIZE);
				network::GetInstance()->SendPacketToLobby(&p);
			}

			GL_TRANSACTIONS_PACKET p;
			p.size = sizeof(p);
			p.type = GL_TRANSACTIONS;
			memcpy_s(p.name, NAME_SIZE, CObjectMgr::GetInstance()->GetClient(m_clientid[3])->GetName().c_str(), NAME_SIZE);
			p.token = earnToken + INIT_EARN_TOKEN * !heroWin;
			memcpy_s(p.time, TIME_SIZE, curTime, TIME_SIZE);
			network::GetInstance()->SendPacketToLobby(&p);

			return;
		}

		float elapsedTime = CGameMgr::GetInstance()->UpdateGameData(matchNum);
		auto now = CGameMgr::GetInstance()->GetLastTime(matchNum);

		Update();

		//Match Update -> move, rotate
		for (auto id : m_clientid)
		{
			if (-1 == id)
				continue;

			std::shared_ptr<CClient> myClient = CObjectMgr::GetInstance()->GetClient(id);
			myClient->Update(elapsedTime);
			for (auto sendId : m_clientid) {
				if (sendId == -1)
					continue;

				// Send Move Packet to other client (i info to j)
				PACKET_SENDER(sendId)->SendMovePacket(myClient->GetMatchId(), myClient->GetPos(), myClient->GetDir());
				PACKET_SENDER(sendId)->SendMovePacket(myClient->GetMatchId(), myClient->GetPos(), myClient->GetDir());
			}
		}

		//Npc Update
		for (int i = 0; i < MAX_MINION + MONSTER_NUM; ++i)
			CObjectMgr::GetInstance()->GetNpc(matchNum, i)->Update(elapsedTime);

		//골드 지급
		if (now - m_lastGoldUpdateTime >= std::chrono::milliseconds(300))
		{
			for (int clientID : m_clientid) 
			{
				if (clientID == -1)
					continue;

				auto& client = CObjectMgr::GetInstance()->GetClient(clientID);
				client->SetGold(client->GetGold() + 1);
				client->GetPacketSender()->SendGoldPacket(client->GetMatchId(), client->GetGold());
			}

			m_lastGoldUpdateTime = now;
		}
		
		// 플레이어 힐
		if (now - m_lastPlayerHeal >= std::chrono::milliseconds(ClientInfos::HEAL_COOLTIME))
		{
			constexpr float HP_HEAL_PERCENTAGE = 0.1f;	// 데이터로 빼자
			constexpr float MP_HEAL_PERCENTAGE = 0.15f; // 데이터로 빼자
			for (int clientID : m_clientid) 
			{
				if (clientID == -1)
					continue;

				const auto& healedClient = CObjectMgr::GetInstance()->GetClient(clientID);
				healedClient->GetStatus()->healthMana.HealHp(static_cast<int>(healedClient->GetStatus()->healthMana.GetMaxHp() * HP_HEAL_PERCENTAGE));
				healedClient->GetStatus()->healthMana.HealMp(static_cast<int>(healedClient->GetStatus()->healthMana.GetMaxMp() * MP_HEAL_PERCENTAGE));

				for (int clID : m_clientid) {
					if (clID == -1)
						continue;
					PACKET_SENDER(clID)->SendPlayerHealthManaPacket(healedClient->GetMatchId(), healedClient->GetStatus()->healthMana);
				}
			}

			m_lastPlayerHeal = now;
		}

		//미니언 리스폰
		// 한 웨이브에 4마리, 맵에 최대 12마리 존재가능
		if (now - m_lastMinionRespawn >= std::chrono::milliseconds(NpcCsvMgr::GetInstance()->GetNpcCsv(ENpcType::Minion)->RespawnTime))
		{
			int cnt = 0;
			for (int i = 0; ; ++i) 
			{
				if (cnt == MINION_WAVE || i == MAX_MINION)
					break;

				auto npc = CObjectMgr::GetInstance()->GetNpc(matchNum, i);
				if (!npc || npc->active)
					continue;

				network::GetInstance()->RegisterTimerEvent({ NPC_ID + i, std::chrono::system_clock::now() + std::chrono::seconds(2 * (cnt + 1)/*미니언 하나하나 출현 간격 데이터로 정의하자*/), EVENT_TYPE::EV_NPC_ACTIVE, matchNum });
				cnt++;
			}

			m_lastMinionRespawn = now;
		}
	}

	void CMatch::Update()
	{
		m_jobQueue.ProcessJob();
	}

}