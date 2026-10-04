#pragma once
#include "CDataBase.h"

namespace wod_server {
	enum class DB_EVENT_TYPE { EV_SAVE_INFO, EV_SHOP_BUY, EV_REGISTER_AUCTION, EV_SAVE_TOKEN, EV_BUY_AUCTION };
	struct DB_EVENT {
		DB_EVENT() {}
		DB_EVENT(std::string id, PlayerInfo playerInfo, std::chrono::system_clock::time_point wakeUpTime, DB_EVENT_TYPE eventID) : id(id), playerInfo(playerInfo), wakeUpTime(wakeUpTime), eventID(eventID) {}
		DB_EVENT(std::string id, int buyPrice, std::chrono::system_clock::time_point wakeUpTime, DB_EVENT_TYPE eventID) : id(id), buyPrice(buyPrice), wakeUpTime(wakeUpTime), eventID(eventID) {}
		DB_EVENT(std::string id, std::chrono::system_clock::time_point wakeUpTime, DB_EVENT_TYPE eventID, int shopType, int shopNum) : id(id), wakeUpTime(wakeUpTime), eventID(eventID), shopType(shopType), shopNum(shopNum) {}
		DB_EVENT(std::string id, std::chrono::system_clock::time_point wakeUpTime, DB_EVENT_TYPE eventID, int shopType, int shopNum, int buyPrice) : id(id), wakeUpTime(wakeUpTime), eventID(eventID), shopType(shopType), shopNum(shopNum), buyPrice(buyPrice) {}		
		DB_EVENT(std::chrono::system_clock::time_point wakeUpTime, DB_EVENT_TYPE eventID, std::string sellerID, std::string myID, short type, short num, unsigned short price) : wakeUpTime(wakeUpTime), eventID(eventID), id(myID), sellerID(sellerID), shopType(type), shopNum(num), buyPrice(price) {}

		std::chrono::system_clock::time_point wakeUpTime = {};
		DB_EVENT_TYPE eventID;

		std::string id;
		std::string sellerID;
		PlayerInfo playerInfo;
				
		int shopType;
		int shopNum;
		int buyPrice;

		constexpr bool operator < (const DB_EVENT& L) const
		{
			return (wakeUpTime > L.wakeUpTime);
		}
	};

	class CDataBase;

	class CDataBaseThread
	{
	public:
		CDataBaseThread();
		~CDataBaseThread();

		void ThreadFunction();	// For NonBlocking DB Process
		CDataBase* GetDataBase() { return m_dataBase; }	// For Blocking DB Process

		void RegisterDBEvent(const DB_EVENT& ev) { m_timerQueue.push(ev); }

	private:
		CDataBase* m_dataBase;

		concurrency::concurrent_priority_queue<DB_EVENT> m_timerQueue;
	};
}