#include "pch.h"
#include "CMatch.h"
#include "ClientInfos.h"
#include "Stats.h"

namespace wod_server {
	void CMatch::SetReady(const CS_READY_PACKET* _packet)
	{
		m_clientReady[_packet->id] = _packet->ready;

		for (int id : m_clientid) {
			if (id == -1)
				continue;

			PACKET_SENDER(id)->SendReadyPacket(_packet->id, _packet->ready);
		}
	}

	void CMatch::SelectSkill(const CS_SKILL_SELECT_PACKET* _packet, std::shared_ptr<CClient> _client)
	{
		if (_client->GetMatchId() == 3) {
			_client->SetSkillNum(_packet->storage + 1, _packet->skill - (21 - 96 - 1));
		}
		else
			_client->SetSkillNum(_packet->storage + 1, _packet->skill);

		for (int id : m_clientid) {
			if (-1 == id)
				continue;

			if (_packet->id == 3) {
				PACKET_SENDER(id)->SendSelectSkillPacket(_packet->id, _packet->storage, _packet->skill + 1);
			}
			else {
				PACKET_SENDER(id)->SendSelectSkillPacket(_packet->id, _packet->storage, _packet->skill);
			}
		}
	}

	void CMatch::SelectJob(const CS_JOB_SELECT_PACKET* _packet, std::shared_ptr<CClient> _client)
	{
		_client->SetPlayerJob(_packet->job);
		_client->SetSkillNum(0, _packet->job + SKILL_OFFSET);

		for (int id : m_clientid) {
			if (id == -1)
				continue;

			PACKET_SENDER(id)->SendJobSelectPacket(_packet->id, _packet->job);
		}
	}

	void CMatch::SelectStat(const CS_STAT_SELECT_PACKET* _packet, std::shared_ptr<CClient> _client)
	{
		_client->SetInitializeStat(_packet->hp, _packet->mp, _packet->attack, _packet->magic_attack,
			_packet->defense, _packet->magic_defense, _packet->speed, _packet->tenacity, _packet->critical);
	}

	void CMatch::LoadingUpdate(int _matchNum)
	{
		Update();

		if (!IsLoadComplete()) {
            network::GetInstance()->RegisterTimerEvent({ _matchNum, TimeUtil::PassedTimeMSec(50), EVENT_TYPE::EV_LOADING_UPDATE, 0 });
            return;
        }

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
				boss->GetStatus()->m_healthMana.SetMaxHp(BossStats::INIT_HP);
				boss->GetStatus()->m_healthMana.SetCurHp(BossStats::INIT_HP);
				boss->GetStatus()->m_healthMana.SetMaxMp(BossStats::INIT_MP);
				boss->GetStatus()->m_healthMana.SetCurMp(BossStats::INIT_MP);
				if (boss->GetPlayerJob() == 4) {
					//Ogre
					boss->InitializeBoundingBox(GameUtil::GetBossPlayerInitBB().m_offset, GameUtil::GetBossPlayerInitBB().m_extent, OGRE_SCALE);
				}
				else {
					//Programmer
					boss->InitializeBoundingBox(GameUtil::GetPlayerInitBB().m_offset, GameUtil::GetPlayerInitBB().m_extent, PLAYER_SCALE);
				}
			}
			else {
				std::shared_ptr<CClient> hero = CObjectMgr::GetInstance()->GetClient(m_clientid[i]);
				hero->GetStatus()->m_healthMana.SetMaxHp(HeroStats::INIT_HP);
				hero->GetStatus()->m_healthMana.SetCurHp(HeroStats::INIT_HP);
				hero->GetStatus()->m_healthMana.SetMaxMp(HeroStats::INIT_MP);
				hero->GetStatus()->m_healthMana.SetCurMp(HeroStats::INIT_MP);
				hero->InitializeBoundingBox(GameUtil::GetPlayerInitBB().m_offset, GameUtil::GetPlayerInitBB().m_extent, PLAYER_SCALE);
			}

			for (int id : m_clientid) {
				if (-1 == id)
					continue;

				PACKET_SENDER(id)->SendPlayerHealthManaPacket(i, CObjectMgr::GetInstance()->GetClient(m_clientid[i])->GetStatus()->m_healthMana);
			}

			//Initialize Packet for Tower and Nexus
			for (int j = 0; j < PATH_NUM; ++j) {
				PACKET_SENDER(m_clientid[i])->SendStructureStatChangePacket(j, CGameMgr::GetInstance()->GetTower(_matchNum, j)->GetMaxHp(), CGameMgr::GetInstance()->GetTower(_matchNum, j)->GetCurHp());
			}
			PACKET_SENDER(m_clientid[i])->SendStructureStatChangePacket(PATH_NUM, CGameMgr::GetInstance()->GetNexus(_matchNum)->GetMaxHp(), CGameMgr::GetInstance()->GetNexus(_matchNum)->GetCurHp());
			PACKET_SENDER(m_clientid[i])->SendStructureStatusChangePacket(3);
		}

		//Start Game
		for (int i = 0; i < MAX_PLAYER; ++i) {
			std::shared_ptr<CClient> player = CObjectMgr::GetInstance()->GetClient(m_clientid[i]);
			for (int j = 0; j < MAX_PLAYER; ++j) {
				if (CMatchMgr::GetInstance()->GetMatch(_matchNum).m_clientid[i] == -1)
					return;
				std::shared_ptr<CClient> otherClient = CObjectMgr::GetInstance()->GetClient(m_clientid[j]);
				player->GetPacketSender()->SendModelCustomizePacket(otherClient->GetMatchId(), otherClient->GetModelCustomize());
				player->GetPacketSender()->SendPlayerStatChangePacket(otherClient->GetMatchId(), otherClient->GetStatus()->GetStat());
			}
		}

		CGameMgr::GetInstance()->ActiveTower(true, _matchNum, 3);
		CGameMgr::GetInstance()->SetLastTime(_matchNum);
		network::GetInstance()->InitializeMonster(_matchNum);
		network::GetInstance()->RegisterTimerEvent({ _matchNum, TimeUtil::CurTime(), EVENT_TYPE::EV_MATCH_UPDATE, -1 });
	}

	void CMatch::ConnectUpdate(int _matchNum)
	{
		Update();

		// 모든 클라인언트 로딩 완료 체크
		if (std::ranges::all_of(m_clientid, [](int _id) {return _id != -1; }))
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
			network::GetInstance()->RegisterTimerEvent(TIMER_EVENT(_matchNum, TimeUtil::CurTime(), EVENT_TYPE::EV_READY_UPDATE, -1));

			return;
		}

		network::GetInstance()->RegisterTimerEvent({ _matchNum, TimeUtil::PassedTimeMSec(1000), EVENT_TYPE::EV_CONNECT_UPDATE, -1 });
	}

	void CMatch::ClientConnect(const CS_LOGIN_PACKET* _packet, std::shared_ptr<CClient> _client)
	{
		UserDataFromLobby data = CObjectMgr::GetInstance()->GetUserData(_packet->name);

		_client->SetName(_packet->name);
		_client->SetMatchNum(data.m_matchNum);
		RegisterClient(data.m_id, _client->GetID());
		_client->SetMatchId(data.m_id);
		_client->SetModelCustomize(data.m_model);

		LogPrinter::PrintMsg(std::string(_packet->name) + " Connected");

		_client->GetPacketSender()->SendLoginPacket();
	}

	void CMatch::ReadyUpdate(int _matchNum)
	{
		auto curTime = TimeUtil::CurTime();
		auto elapsedTime = std::chrono::duration_cast<std::chrono::microseconds>(curTime - m_updateTime).count() * 0.000001f;
		m_readyTime -= elapsedTime;
		m_updateTime = curTime;

		Update();

		// 게임시작
		if (IsAllReady() || m_readyTime <= 0.f)
		{
			// 선택 결과를 같은 TCP 연결에서 먼저 전송해야 클라이언트가 필요한 효과만 생성할 수 있다.
			CGameMgr::GetInstance()->SkillAutoSelect(_matchNum);

			for (const auto id : m_clientid)
				CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendGameStartPacket();

			m_sceneType = ESceneType::LoadingScene;

			//Initialize Client Position
			for (size_t i = 0; i < ClientInfos::HERO_START_POS.size(); ++i)
				CObjectMgr::GetInstance()->GetClient(m_clientid[i])->SetPos(ClientInfos::HERO_START_POS[i]);

			CObjectMgr::GetInstance()->GetClient(m_clientid[3])->SetPos(ClientInfos::BOSS_START_POS);

			for (int i = 0; i < MAX_PLAYER; ++i) {
				const auto& client = CObjectMgr::GetInstance()->GetClient(m_clientid[i]);
				for (int j = 0; j < MAX_PLAYER; ++j) {
					PACKET_SENDER(m_clientid[j])->SendMovePacket(i, client->GetPos(), client->GetDir());
				}
			}

			network::GetInstance()->RegisterTimerEvent({ _matchNum, std::chrono::system_clock::now(), EVENT_TYPE::EV_LOADING_UPDATE, 0 });
			return;
		}

		// 준비시간 업데이트
		for (const auto id : m_clientid)
			PACKET_SENDER(id)->SendGameTimePacket(static_cast<int>(::ceil(m_readyTime)), 0);

		network::GetInstance()->RegisterTimerEvent({ _matchNum, TimeUtil::PassedTimeMSec(50), EVENT_TYPE::EV_READY_UPDATE, 0 });
	}

	void CMatch::TeleportStart(std::shared_ptr<CClient> _client)
	{
		int teleport;
		if (!GameUtil::CheckTeleportCollision(_client->GetPos(), teleport) || _client->GetTeleport() || !_client->CheckTeleportCoolTime())
			return;

		_client->SetTeleport(true);
		_client->SetPos(TELEPORT_POS[teleport]);
		_client->SetTeleportNum(teleport);
		_client->SetTeleportLastUsedTime();
		_client->GetPacketSender()->SendTeleportActivePacket(false);

		for (int id : m_clientid) {
			if (id == -1)
				continue;
			PACKET_SENDER(id)->SendTeleportPacket(_client->GetMatchId(), false);
		}
	}

	void CMatch::UseItem(const CS_USE_ITEM_PACKET* _packet, std::shared_ptr<CClient> _client)
	{
		if (!_client->GetItemInfo(_packet->itemNum)->m_exist)
			return;

		auto it = ItemCsvMgr::m_ITEMKINDToEItemType.find(_client->GetItemInfo(_packet->itemNum)->GetType());
		if (it == ItemCsvMgr::m_ITEMKINDToEItemType.end())
			return;

		if (false == CItemFactory::UseItem(it->second, _client)) {
			LogPrinter::PrintMsg("Failed To Use Item");
			return;
		}

		_client->GetItemInfo(_packet->itemNum)->m_exist = false;
		_client->GetItemInfo(_packet->itemNum)->SetType(ITEMKIND::NONE);
	}

	void CMatch::InGameUpdate(int _matchNum)
	{
		// id == match number
		if (-1 == CGameMgr::GetInstance()->IsGameOver(_matchNum)) {
			CNetworkMgr::GetInstance()->RegisterTimerEvent({ _matchNum, TimeUtil::NextFrameTime(), EVENT_TYPE::EV_MATCH_UPDATE, -1 });
		}
		else {
			CNetworkMgr::GetInstance()->RegisterTimerEvent({ _matchNum, std::chrono::system_clock::now() + std::chrono::seconds(10), EVENT_TYPE::EV_MATCH_FINISH, -1 });

			constexpr short INIT_EARN_TOKEN = 10;

			bool heroWin = static_cast<bool>(CGameMgr::GetInstance()->IsGameOver(_matchNum));
			short earnToken = INIT_EARN_TOKEN + static_cast<int>(CGameMgr::GetInstance()->GetGameTime(_matchNum) / 30.0f);

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

		float elapsedTime = CGameMgr::GetInstance()->UpdateGameData(_matchNum);
		auto now = CGameMgr::GetInstance()->GetLastTime(_matchNum);

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
		for (int i = 0; i < MAX_MINION + MONSTER_NUM; ++i) {
            const auto npc = CObjectMgr::GetInstance()->GetNpc(_matchNum,i);
            if (npc->m_active.load()) npc->Update(elapsedTime);
        }

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
				healedClient->GetStatus()->m_healthMana.HealHp(static_cast<int>(healedClient->GetStatus()->m_healthMana.GetMaxHp() * HP_HEAL_PERCENTAGE));
				healedClient->GetStatus()->m_healthMana.HealMp(static_cast<int>(healedClient->GetStatus()->m_healthMana.GetMaxMp() * MP_HEAL_PERCENTAGE));

				for (int clID : m_clientid) {
					if (clID == -1)
						continue;
					PACKET_SENDER(clID)->SendPlayerHealthManaPacket(healedClient->GetMatchId(), healedClient->GetStatus()->m_healthMana);
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

				auto npc = CObjectMgr::GetInstance()->GetNpc(_matchNum, i);
				if (!npc || npc->m_active)
					continue;

				network::GetInstance()->RegisterTimerEvent({ NPC_ID + i, std::chrono::system_clock::now() + std::chrono::seconds(2 * (cnt + 1)/*미니언 하나하나 출현 간격 데이터로 정의하자*/), EVENT_TYPE::EV_NPC_ACTIVE, _matchNum });
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
