#pragma once

#include <sqlext.h>
#include <sql.h>

namespace wod_server {

	class CDataBase
	{
	public:
		CDataBase();
		~CDataBase();

		bool Connect(const std::wstring& _database);
		void Disconnect();

		bool GetPlayerInfo(const std::string& _id, const std::string& _password, PlayerInfo& _playerInfo);
		bool CheckIdExists(const std::string& _id);
		bool CheckPassword(const std::string& _id, const std::string& _password);
		bool SignUp(const std::string& _id, const std::string& _password);
		std::vector<AuctionInfo> GetAuctionInfo(int _pageNum);
		int GetAuctionPartsNum();
		bool GetPlayerCustomizeParts(const std::string& _id, std::vector<std::tuple<short, short, short>>& _customizeParts);
		std::vector<std::string> GetPlayersByShopType(short _shopType);

		bool SavePlayerInfo(const std::string& _id, const PlayerInfo& _playerInfo);
		bool SaveToken(const std::string& _id, int _token);
		bool SaveShopBuyInfo(const std::string& _id, const int _shopType, const int _shopNum);
		bool RegisterAuction(const std::string& _id, const int _shopType, const int _shopNum, const int _buyPrice, const std::string& _deadline);
		bool BuyAuction(const std::string& _seller, const std::string& _myID, short _type, short _num, unsigned short _price);

		std::vector<std::pair<std::string, int>> GetValidatorIDs();
		bool StakeTokens(const std::string& _id, int _tokenNum, unsigned short _stakeDays);
		bool ChangeNodeType(const std::string& _id, bool _fullNode);
		bool GetFullNodeInfo(const std::string& _id, bool& _fullNode);
		bool GetStakedTokenData(const std::string& _id, int& _stakedToken, int& _stakedDays, int& _dayCount);

	private:
		void Show_Error(SQLHANDLE _hHandle, SQLSMALLINT _hType, RETCODE _retCode);

		bool AllocStatementHandle(SQLHSTMT& _hstmt);
		void FreeStatementHandle(SQLHSTMT _hstmt);

		template<typename T>
		bool BindParameter(SQLHSTMT _hstmt, int _index, const T& _value);

		template<typename T, typename... Args>
		void BindParameters(SQLHSTMT _hstmt, int _index, const T& _first, const Args&... _rest);
		void BindParameters(SQLHSTMT _hstmt, int _index) {} // Base case

		template<typename... Args>
		bool ExecuteSP(STORED_PROCEDURE _sp, const Args&... _args);

		template<typename... Args>
		bool ExecuteSP(SQLHSTMT& _hstmt, STORED_PROCEDURE _sp, const Args&... _args);

	private:
		SQLHENV m_henv;
		SQLHDBC m_hdbc;

		std::map<STORED_PROCEDURE, std::wstring> m_spMap;
	};

	template<typename T>
	bool CDataBase::BindParameter(SQLHSTMT _hstmt, int _index, const T& _value)
	{
		// Generic template, should be specialized
		return false;
	}

	template<>
	inline bool CDataBase::BindParameter<std::string>(SQLHSTMT _hstmt, int _index, const std::string& _value)
	{
		SQLRETURN ret = SQLBindParameter(_hstmt, _index, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, _value.length(), 0, (SQLPOINTER)_value.c_str(), _value.length(), NULL);
		return ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO;
	}

	template<>
	inline bool CDataBase::BindParameter<int>(SQLHSTMT _hstmt, int _index, const int& _value)
	{
		SQLRETURN ret = SQLBindParameter(_hstmt, _index, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, (SQLPOINTER)&_value, 0, NULL);
		return ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO;
	}

	template<>
	inline bool CDataBase::BindParameter<short>(SQLHSTMT _hstmt, int _index, const short& _value)
	{
		SQLRETURN ret = SQLBindParameter(_hstmt, _index, SQL_PARAM_INPUT, SQL_C_SHORT, SQL_SMALLINT, 0, 0, (SQLPOINTER)&_value, 0, NULL);
		return ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO;
	}

	template<>
	inline bool CDataBase::BindParameter<unsigned short>(SQLHSTMT _hstmt, int _index, const unsigned short& _value)
	{
		SQLRETURN ret = SQLBindParameter(_hstmt, _index, SQL_PARAM_INPUT, SQL_C_SHORT, SQL_SMALLINT, 0, 0, (SQLPOINTER)&_value, 0, NULL);
		return ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO;
	}

	template<>
	inline bool CDataBase::BindParameter<bool>(SQLHSTMT _hstmt, int _index, const bool& _value)
	{
		SQLRETURN ret = SQLBindParameter(_hstmt, _index, SQL_PARAM_INPUT, SQL_C_BIT, SQL_BIT, 0, 0, (SQLPOINTER)&_value, 0, NULL);
		return ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO;
	}

	template<typename T, typename... Args>
	void CDataBase::BindParameters(SQLHSTMT _hstmt, int _index, const T& _first, const Args&... _rest)
	{
		if (BindParameter(_hstmt, _index, _first)) {
			BindParameters(_hstmt, ++_index, _rest...);
		}
	}

	template<typename... Args>
	inline bool CDataBase::ExecuteSP(STORED_PROCEDURE _sp, const Args&... _args)
	{
		SQLHSTMT hstmt;
		if (!AllocStatementHandle(hstmt))
			return false;

		if (!ExecuteSP(hstmt, _sp, _args...)) {
			FreeStatementHandle(hstmt);
			return false;
		}

		FreeStatementHandle(hstmt);
		return true;
	}

	template<typename... Args>
	bool CDataBase::ExecuteSP(SQLHSTMT& _hstmt, STORED_PROCEDURE _sp, const Args&... _args)
	{
		std::wstring spName = m_spMap.at(_sp);
		std::wstring query = L"EXEC " + spName;
		if constexpr (sizeof...(_args) > 0) {
			query += L" ?";
			for (int i = 1; i < sizeof...(_args); ++i) {
				query += L", ?";
			}
		}

		SQLRETURN ret = SQLPrepare(_hstmt, (SQLWCHAR*)query.c_str(), SQL_NTS);
		if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
			Show_Error(_hstmt, SQL_HANDLE_STMT, ret);
			return false;
		}

		if constexpr (sizeof...(_args) > 0) {
			BindParameters(_hstmt, 1, _args...);
		}

		ret = SQLExecute(_hstmt);
		if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
			Show_Error(_hstmt, SQL_HANDLE_STMT, ret);
			return false;
		}

		return true;
	}
}