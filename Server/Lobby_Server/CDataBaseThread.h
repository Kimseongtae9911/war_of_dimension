#pragma once
#include "CDataBase.h"

namespace wod_server {
	enum class DB_EVENT_TYPE { EV_SAVE_INFO, EV_SHOP_BUY, EV_REGISTER_AUCTION, EV_SAVE_TOKEN, EV_BUY_AUCTION };
	struct DB_EVENT {
		DB_EVENT() {}
		DB_EVENT(std::string _id, PlayerInfo _playerInfo, std::chrono::system_clock::time_point _wakeUpTime, DB_EVENT_TYPE _eventID) : m_id(_id), m_playerInfo(_playerInfo), m_wakeUpTime(_wakeUpTime), m_eventID(_eventID) {}
		DB_EVENT(std::string _id, int _buyPrice, std::chrono::system_clock::time_point _wakeUpTime, DB_EVENT_TYPE _eventID) : m_id(_id), m_buyPrice(_buyPrice), m_wakeUpTime(_wakeUpTime), m_eventID(_eventID) {}
		DB_EVENT(std::string _id, std::chrono::system_clock::time_point _wakeUpTime, DB_EVENT_TYPE _eventID, int _shopType, int _shopNum) : m_id(_id), m_wakeUpTime(_wakeUpTime), m_eventID(_eventID), m_shopType(_shopType), m_shopNum(_shopNum) {}
		DB_EVENT(std::string _id, std::chrono::system_clock::time_point _wakeUpTime, DB_EVENT_TYPE _eventID, int _shopType, int _shopNum, int _buyPrice) : m_id(_id), m_wakeUpTime(_wakeUpTime), m_eventID(_eventID), m_shopType(_shopType), m_shopNum(_shopNum), m_buyPrice(_buyPrice) {}
		DB_EVENT(std::chrono::system_clock::time_point _wakeUpTime, DB_EVENT_TYPE _eventID, std::string _sellerID, std::string _myID, short _type, short _num, unsigned short _price) : m_wakeUpTime(_wakeUpTime), m_eventID(_eventID), m_id(_myID), m_sellerID(_sellerID), m_shopType(_type), m_shopNum(_num), m_buyPrice(_price) {}

		std::chrono::system_clock::time_point m_wakeUpTime = {};
		DB_EVENT_TYPE m_eventID;

		std::string m_id;
		std::string m_sellerID;
		PlayerInfo m_playerInfo;

		int m_shopType;
		int m_shopNum;
		int m_buyPrice;

		constexpr bool operator < (const DB_EVENT& _l) const
		{
			return (m_wakeUpTime > _l.m_wakeUpTime);
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

		void RegisterDBEvent(const DB_EVENT& _ev) { m_timerQueue.push(_ev); }

	private:
		CDataBase* m_dataBase;

		concurrency::concurrent_priority_queue<DB_EVENT> m_timerQueue;
	};
}