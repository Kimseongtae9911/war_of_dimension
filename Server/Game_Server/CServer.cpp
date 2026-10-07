#include "pch.h"
#include <ServerCore/Session.h>
#include "Resource.h"
#include "CServer.h"
#include "CPacketMgr.h"
#include "CSkillHandlerFactory.h"
#include "SocketUtil.h"

namespace wod_server {
	bool CServer::Initialize()
	{
		LogPrinter::PrintMsg("Server Initialize Start");
		SocketUtil::Startup();
        m_transportReady = true;

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
        m_skillCsvReady = true;
		SkillCsvMgr::GetInstance()->Load("DataFile/CSV/skill_info.csv");

		if (!NpcCsvMgr::GetInstance()->Initialize()) {
			LogPrinter::PrintMsg("NpcCsvMgr Initialize Fail");
			return false;
		}
        m_npcCsvReady = true;
		NpcCsvMgr::GetInstance()->Load("DataFile/CSV/npc_info.csv");

		if (!ItemCsvMgr::GetInstance()->Initialize()) {
			LogPrinter::PrintMsg("ItemCsvMgr Initialize Fail");
			return false;
		}
        m_itemCsvReady = true;
		ItemCsvMgr::GetInstance()->Load("DataFile/CSV/item_info.csv");

		if (!CGameMgr::GetInstance()->Initialize()) {
			LogPrinter::PrintMsg("GameMgr Initialize Fail");
			return false;
		}
        m_gameReady = true;

		if (!CPacketMgr::GetInstance()->Initialize()) {
			LogPrinter::PrintMsg("PacketMgr Initialize Fail");
			return false;
		}
        m_packetReady = true;

		if (!CObjectMgr::GetInstance()->Initialize()) {
			LogPrinter::PrintMsg("ObjectMgr Initialize Fail");
			return false;
		}

		if (!CMatchMgr::GetInstance()->Initialize()) {
			LogPrinter::PrintMsg("MatchMgr Initalize Fail");
			return false;
		}
        m_matchReady = true;

		if (!CSkillHandlerFactory::GetInstance()->Initialize()) {
			LogPrinter::PrintMsg("SkillHandlerFactory Initialize Fail");
			return false;
		}
        m_skillFactoryReady = true;

		LogPrinter::PrintMsg("Server Initialize Finish");
		return true;
	}

	bool CServer::Release()
	{
        if (!m_transportReady) return true;
        bool ok = true;
        ok = network::GetInstance()->Release() && ok;
        if (std::exchange(m_skillCsvReady, false)) ok = SkillCsvMgr::GetInstance()->Release() && ok;
        if (std::exchange(m_npcCsvReady, false)) ok = NpcCsvMgr::GetInstance()->Release() && ok;
        if (std::exchange(m_itemCsvReady, false)) ok = ItemCsvMgr::GetInstance()->Release() && ok;
        if (std::exchange(m_gameReady, false)) ok = CGameMgr::GetInstance()->Release() && ok;
        if (std::exchange(m_packetReady, false)) ok = CPacketMgr::GetInstance()->Release() && ok;
        if (std::exchange(m_matchReady, false)) ok = CMatchMgr::GetInstance()->Release() && ok;
        if (std::exchange(m_skillFactoryReady, false)) ok = CSkillHandlerFactory::GetInstance()->Release() && ok;
        ok = CObjectMgr::GetInstance()->Release() && ok;
        SocketUtil::Cleanup(); m_transportReady = false; return ok;
	}

	void CServer::Run()
	{
        wod::core::ProcessStopSignal signal;
        const unsigned int count = (std::max)(1u, std::thread::hardware_concurrency()/2);
        const auto failure = [](std::exception_ptr error) {
            try { std::rethrow_exception(error); }
            catch (const std::exception& detail) { LogPrinter::PrintMsg(std::string("Worker failed: ") + detail.what()); }
            catch (...) { LogPrinter::PrintMsg("Worker failed: unknown exception"); }
            wod::core::ProcessStopSignal::Request(GetCurrentProcessId());
        };
        wod::core::ThreadGroup iocp([&] { network::GetInstance()->PrepareStop(); SocketUtil::Runtime().RequestStop(count); },failure);
        wod::core::ThreadGroup producers([] { network::GetInstance()->PrepareStop(); },failure);
        for (unsigned int i=0; i<count; ++i) iocp.Launch([] { network::GetInstance()->IOCPFunc(); });
        producers.Launch([] { network::GetInstance()->TimerFunc(); });
        signal.Wait();
        producers.StopAndJoin(); iocp.StopAndJoin();
        if (producers.Failed() || iocp.Failed() || network::GetInstance()->WorkerFailed())
            throw std::runtime_error("server worker failure");

	}

}
