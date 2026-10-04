#include "pch.h"
#include "CServer.h"
#include "CPacketMgr.h"
#include "CSkillHandlerFactory.h"
#include "SocketUtil.h"

namespace wod_server {
	bool CServer::Initialize()
	{
		LogPrinter::PrintMsg("Server Initialize Start");
		SocketUtil::Startup();

		if (!GameUtil::LoadCoolTime("Resource/SkillCoolTime.txt")) {
			LogPrinter::PrintMsg("Failed To Load Skill CoolTime");
			return false;
		}

		if (!GameUtil::LoadManaConsumption("Resource/SkillManaConsumption.txt")) {
			LogPrinter::PrintMsg("Failed To Load Skill Mana Consumption");
			return false;
		}

		if (!GameUtil::LoadNaviMesh("Resource/NavMeshData3.obj")) {
			LogPrinter::PrintMsg("Failed To Load NaviMesh");
			return false;
		}

		if (!GameUtil::LoadHeightMesh("Resource/HeightMesh.obj")) {
			LogPrinter::PrintMsg("Failed To Load HeightMesh");
			return false;
		}

		if (!GameUtil::LoadNexusBB("Resource/NexusBB.txt")) {
			LogPrinter::PrintMsg("Failed To Load NexusBB");
			return false;
		}

		if (!GameUtil::LoadTowerBB()) {
			LogPrinter::PrintMsg("Failed To Load TowerBBs");
			return false;
		}

		if (!GameUtil::LoadMap("Resource/Map.txt")) {
			LogPrinter::PrintMsg("Failed To Load Map");
			return false;
		}
		if (!GameUtil::LoadPlayerBB("Resource/PlayerBB.txt", 0)) {
			LogPrinter::PrintMsg("Failed To Load PlayerBoundingBox");
			return false;
		}
		if (!GameUtil::LoadPlayerBB("Resource/OgreBB.txt", 1)) {
			LogPrinter::PrintMsg("Failed To Load BossBoundingBox");
			return false;
		}
		if (!GameUtil::LoadMinionBB("Resource/MinionBB.txt")) {
			LogPrinter::PrintMsg("Failed To Load MinionBB");
			return false;
		}
		if (!GameUtil::LoadMinionPath("Resource/MinionPath1.txt", 0)) {
			LogPrinter::PrintMsg("Failed To Load MinionPath1");
			return false;
		}
		if (!GameUtil::LoadMinionPath("Resource/MinionPath2.txt", 1)) {
			LogPrinter::PrintMsg("Failed To Load MinionPath2");
			return false;
		}
		if (!GameUtil::LoadMinionPath("Resource/MinionPath3.txt", 2)) {
			LogPrinter::PrintMsg("Failed To Load MinionPath3");
			return false;
		}
		if (!GameUtil::LoadMinionPath("Resource/MinionPath4.txt", 3)) {
			LogPrinter::PrintMsg("Failed To Load MinionPath4");
			return false;
		}
		if (!GameUtil::LoadFenceBB("Resource/MagneticFence.txt")) {
			LogPrinter::PrintMsg("Failed To Load MagneticFenceBB");
			return false;
		}
		if (!GameUtil::LoadMonsterBB()) {
			LogPrinter::PrintMsg("In Loading MonsterBB");
			return false;
		}

		if (!network::GetInstance()->Initialize()) {
			LogPrinter::PrintMsg("NetworkMgr Initialize Fail");
			return false;
		}

		if (!SkillCsvMgr::GetInstance()->Initialize()) {
			LogPrinter::PrintMsg("SkillCsvMgr Initialize Fail");
			return false;
		}
		SkillCsvMgr::GetInstance()->Load("DataFile/CSV/skill_info.csv");

		if (!NpcCsvMgr::GetInstance()->Initialize()) {
			LogPrinter::PrintMsg("NpcCsvMgr Initialize Fail");
			return false;
		}
		NpcCsvMgr::GetInstance()->Load("DataFile/CSV/npc_info.csv");

		if (!ItemCsvMgr::GetInstance()->Initialize()) {
			LogPrinter::PrintMsg("ItemCsvMgr Initialize Fail");
			return false;
		}
		ItemCsvMgr::GetInstance()->Load("DataFile/CSV/item_info.csv");

		if (!CGameMgr::GetInstance()->Initialize()) {
			LogPrinter::PrintMsg("GameMgr Initialize Fail");
			return false;
		}

		if (!CPacketMgr::GetInstance()->Initialize()) {
			LogPrinter::PrintMsg("PacketMgr Initialize Fail");
			return false;
		}

		if (!CObjectMgr::GetInstance()->Initialize()) {
			LogPrinter::PrintMsg("ObjectMgr Initialize Fail");
			return false;
		}

		if (!CMatchMgr::GetInstance()->Initialize()) {
			LogPrinter::PrintMsg("MatchMgr Initalize Fail");
			return false;
		}

		if (!CSkillHandlerFactory::GetInstance()->Initialize()) {
			LogPrinter::PrintMsg("SkillHandlerFactory Initialize Fail");
			return false;
		}

		LogPrinter::PrintMsg("Server Initialize Finish");
		return true;
	}

	bool CServer::Release()
	{
		LogPrinter::PrintMsg("Server Release");

		if (!network::GetInstance()->Release()) {
			LogPrinter::PrintMsg("NetworkMgr Release Fail");
			return false;
		}
		if (!CPacketMgr::GetInstance()->Release()) {
			LogPrinter::PrintMsg("PacketMgr Release Fail");
			return false;
		}
		if (!CGameMgr::GetInstance()->Release()) {
			LogPrinter::PrintMsg("GameMgr Release Fail");
			return false;
		}
		if (!CSkillHandlerFactory::GetInstance()->Release()) {
			LogPrinter::PrintMsg("SkillHandlerFactory Release Fail");
			return false;
		}

		SocketUtil::Cleanup();
		return true;
	}

	void CServer::Run()
	{
		// Create Worker Thread -> Packet Process(Game Update), 
		unsigned int thread_num = std::thread::hardware_concurrency() / 2;
		m_iocpThread.reserve(thread_num);
		for (unsigned int i = 0; i < thread_num; ++i) {
			m_iocpThread.emplace_back([this]() { network::GetInstance()->IOCPFunc(); });
		}
		
		// Timer -> Event, Npc
		std::thread timer{ [this]() {TimerFunc(); } };

		
		timer.join();
		for (unsigned int i = 0; i < thread_num; ++i) {
			m_iocpThread[i].join();
		}
	}

	void CServer::TimerFunc()
	{
		network::GetInstance()->TimerFunc();
	}
}