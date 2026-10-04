#include "pch.h"
#include "CServer.h"
#include "CNetworkMgr.h"
#include "CMatchMgr.h"
#include "CPacketMgr.h"
#include "CUserMgr.h"
#include "SocketUtil.h"

//#define Test

namespace wod_server {

	bool CServer::Initialize()
	{
		LogPrinter::PrintMsg("Server Initialize Start");
		SocketUtil::Startup();

		if (!network::GetInstance()->Initialize()) {
			LogPrinter::PrintMsg("NetworkMgr Initialize Fail");
			return false;
		}

		if (!match::GetInstance()->Initialize()) {
			LogPrinter::PrintMsg("MatchMgr Initialize Fail");
			return false;
		}

		if (!CPacketMgr::GetInstance()->Initialize()) {
			LogPrinter::PrintMsg("PacketMgr Initialize Fail");
			return false;
		}

		if (!CUserMgr::GetInstance()->Initialize()) {
			LogPrinter::PrintMsg("UserMgr Initialize Fail");
			return false;
		}

		if (!GameUtil::LoadNaviMesh("Resource/NavMeshData2.obj")) {
			LogPrinter::PrintMsg("Failed To Load NaviMesh");
			return false;
		}

		if (!GameUtil::LoadHeightMesh("Resource/HeightMesh2.obj")) {
			LogPrinter::PrintMsg("Failed To Load HeightMesh");
			return false;
		}

		if (!GameUtil::LoadMap("Resource/Map.txt")) {
			LogPrinter::PrintMsg("Failed To Load Map");
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

		if (!match::GetInstance()->Release()) {
			LogPrinter::PrintMsg("MatchMgr Release Fail");
			return false;
		}

		if (!CPacketMgr::GetInstance()->Release()) {
			LogPrinter::PrintMsg("PacketMgr Release Fail");
			return false;
		}

		if (!CUserMgr::GetInstance()->Release()) {
			LogPrinter::PrintMsg("UserMgr Release Fail");
			return false;
		}

		SocketUtil::Cleanup();
		return true;
	}

	void CServer::Run()
	{
		unsigned int threadNum = std::thread::hardware_concurrency() / 2;

		m_iocpThreads.reserve(threadNum);
		for (unsigned int i = 0; i < 4; ++i) {
			m_iocpThreads.emplace_back([this]() {network::GetInstance()->IOCPFunc(); });
		}

		for (unsigned int i = 0; i < 1; ++i) {
			m_workerThreads.emplace_back([this]() {GPacketJobQueue->ProcessJob(); });
		}

		//DataBase
#ifdef WITH_DATABASE
		std::thread database{[this]() {network::GetInstance()->DataBaseFunc(); }};
#endif

		// Timer -> Event, Npc
		std::thread timer{ [this]() {network::GetInstance()->TimerFunc(); } };

		timer.join();
#ifdef WITH_DATABASE
		database.join();
#endif
		for (unsigned int i = 0; i < threadNum; ++i) {
			m_workerThreads[i].join();
		}

		for (unsigned int i = 0; i < threadNum; ++i) {
			m_iocpThreads[i].join();
		}
	}

}