#include "pch.h"
#include "CDataBaseThread.h"
#include "CNetworkMgr.h"

namespace wod_server {
	CDataBaseThread::CDataBaseThread()
	{
		m_dataBase = new CDataBase();
		if (m_dataBase->Connect(L"WOD_ODBC")) {
			LogPrinter::PrintMsg("Database Connected");
		}
		else {
			LogPrinter::PrintMsg("Database Connect Failed");
		}
	}

	CDataBaseThread::~CDataBaseThread()
	{
	}

	void CDataBaseThread::ThreadFunction()
	{
		while (!network::GetInstance()->IsStopping()) {
			DB_EVENT ev;
			auto current_time = std::chrono::system_clock::now();
			if (m_timerQueue.try_pop(ev)) {
				if (ev.wakeUpTime > current_time) {
					m_timerQueue.push(ev);
					std::this_thread::sleep_for(std::chrono::milliseconds(1));
					continue;
				}
				switch (ev.eventID) {
				case DB_EVENT_TYPE::EV_SAVE_INFO:
					if (m_dataBase->SavePlayerInfo(ev.id, ev.playerInfo)) {
						LogPrinter::PrintMsg("Saved Player Info");
					}
					break;
				case DB_EVENT_TYPE::EV_SHOP_BUY:
					if (m_dataBase->SaveShopBuyInfo(ev.id, ev.shopType, ev.shopNum)) {
						LogPrinter::PrintMsg("Saved Shop Buy Info");
					}
					break;
				case DB_EVENT_TYPE::EV_REGISTER_AUCTION: {
					std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
					std::time_t futureTime = std::chrono::system_clock::to_time_t(std::chrono::time_point_cast<std::chrono::seconds>(now + std::chrono::hours(24)));

					std::ostringstream deadLine;
					std::tm localTime;
					localtime_s(&localTime, &futureTime);
					deadLine << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S");

					if (m_dataBase->RegisterAuction(ev.id, ev.shopType, ev.shopNum, ev.buyPrice, deadLine.str())) {
						LogPrinter::PrintMsg("Register Auction");
					}
					break;
				}
				case DB_EVENT_TYPE::EV_SAVE_TOKEN: {
					if (m_dataBase->SaveToken(ev.id, ev.buyPrice)) {
						LogPrinter::PrintMsg("Saved Token");
					}
					break;
				}
				case DB_EVENT_TYPE::EV_BUY_AUCTION: {
					if (m_dataBase->BuyAuction(ev.sellerID, ev.id, ev.shopType, ev.shopNum, ev.buyPrice)) {
						LogPrinter::PrintMsg("Buy Auction");
					}
					break;
				}

				}
				
				
				continue;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}
}