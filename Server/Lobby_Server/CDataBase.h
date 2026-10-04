#pragma once

#include <sqlext.h>
#include <sql.h>

namespace wod_server {

	class CDataBase
	{
	public:
		CDataBase();
		~CDataBase();

		bool Connect(const std::wstring& database);
		void Disconnect();

		bool GetPlayerInfo(const std::string& id, const std::string& password, PlayerInfo& playerInfo);
		bool CheckIdExists(const std::string& id);
		bool CheckPassword(const std::string& id, const std::string& password);
		bool SignUp(const std::string& id, const std::string& password);
		std::vector<AuctionInfo> GetAuctionInfo(int pageNum);
		int GetAuctionPartsNum();
		bool GetPlayerCustomizeParts(const std::string& id, std::vector<std::tuple<short, short, short>>& customizeParts);
		std::vector<std::string> GetPlayersByShopType(short shopType);

		bool SavePlayerInfo(const std::string& id, const PlayerInfo& playerInfo);
		bool SaveToken(const std::string& id, int token);
		bool SaveShopBuyInfo(const std::string& id, const int shopType, const int shopNum);
		bool RegisterAuction(const std::string& id, const int shopType, const int shopNum, const int buyPrice, const std::string& deadline);
		bool BuyAuction(const std::string& seller, const std::string& myID, short type, short num, unsigned short price);

		std::vector<std::pair<std::string, int>> GetValidatorIDs();
		bool StakeTokens(const std::string& id, int tokenNum, unsigned short stakeDays);
		bool ChangeNodeType(const std::string& id, bool fullNode);
		bool GetFullNodeInfo(const std::string& id, bool& fullNode);
		bool GetStakedTokenData(const std::string& id, int& stakedToken, int& stakedDays, int& dayCount);

	private:
		void Show_Error(SQLHANDLE hHandle, SQLSMALLINT hType, RETCODE RetCode);

		bool AllocStatementHandle(SQLHSTMT& hstmt);
		void FreeStatementHandle(SQLHSTMT hstmt);

		template<typename T>
		bool BindParameter(SQLHSTMT hstmt, int index, const T& value);

		template<typename T, typename... Args>
		void BindParameters(SQLHSTMT hstmt, int index, const T& first, const Args&... rest);
		void BindParameters(SQLHSTMT hstmt, int index) {} // Base case

		template<typename... Args>
		bool ExecuteSP(STORED_PROCEDURE sp, const Args&... args);

		template<typename... Args>
		bool ExecuteSP(SQLHSTMT& hstmt, STORED_PROCEDURE sp, const Args&... args);

	private:
		SQLHENV m_henv;
		SQLHDBC m_hdbc;

		std::map<STORED_PROCEDURE, std::wstring> m_spMap;
	};

	template<typename T>
	bool CDataBase::BindParameter(SQLHSTMT hstmt, int index, const T& value)
	{
		// Generic template, should be specialized
		return false;
	}

	template<>
	inline bool CDataBase::BindParameter<std::string>(SQLHSTMT hstmt, int index, const std::string& value)
	{
		SQLRETURN ret = SQLBindParameter(hstmt, index, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, value.length(), 0, (SQLPOINTER)value.c_str(), value.length(), NULL);
		return ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO;
	}

	template<>
	inline bool CDataBase::BindParameter<int>(SQLHSTMT hstmt, int index, const int& value)
	{
		SQLRETURN ret = SQLBindParameter(hstmt, index, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, (SQLPOINTER)&value, 0, NULL);
		return ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO;
	}

	template<>
	inline bool CDataBase::BindParameter<short>(SQLHSTMT hstmt, int index, const short& value)
	{
		SQLRETURN ret = SQLBindParameter(hstmt, index, SQL_PARAM_INPUT, SQL_C_SHORT, SQL_SMALLINT, 0, 0, (SQLPOINTER)&value, 0, NULL);
		return ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO;
	}

	template<>
	inline bool CDataBase::BindParameter<unsigned short>(SQLHSTMT hstmt, int index, const unsigned short& value)
	{
		SQLRETURN ret = SQLBindParameter(hstmt, index, SQL_PARAM_INPUT, SQL_C_SHORT, SQL_SMALLINT, 0, 0, (SQLPOINTER)&value, 0, NULL);
		return ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO;
	}

	template<>
	inline bool CDataBase::BindParameter<bool>(SQLHSTMT hstmt, int index, const bool& value)
	{
		SQLRETURN ret = SQLBindParameter(hstmt, index, SQL_PARAM_INPUT, SQL_C_BIT, SQL_BIT, 0, 0, (SQLPOINTER)&value, 0, NULL);
		return ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO;
	}

	template<typename T, typename... Args>
	void CDataBase::BindParameters(SQLHSTMT hstmt, int index, const T& first, const Args&... rest)
	{
		if (BindParameter(hstmt, index, first)) {
			BindParameters(hstmt, ++index, rest...);
		}
	}

	template<typename... Args>
	inline bool CDataBase::ExecuteSP(STORED_PROCEDURE sp, const Args&... args)
	{
		SQLHSTMT hstmt;
		if (!AllocStatementHandle(hstmt))
			return false;

		if (!ExecuteSP(hstmt, sp, args...)) {
			FreeStatementHandle(hstmt);
			return false;
		}

		FreeStatementHandle(hstmt);
		return true;
	}

	template<typename... Args>
	bool CDataBase::ExecuteSP(SQLHSTMT& hstmt, STORED_PROCEDURE sp, const Args&... args)
	{
		std::wstring spName = m_spMap.at(sp);
		std::wstring query = L"EXEC " + spName;
		if constexpr (sizeof...(args) > 0) {
			query += L" ?";
			for (int i = 1; i < sizeof...(args); ++i) {
				query += L", ?";
			}
		}

		SQLRETURN ret = SQLPrepare(hstmt, (SQLWCHAR*)query.c_str(), SQL_NTS);
		if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
			Show_Error(hstmt, SQL_HANDLE_STMT, ret);
			return false;
		}

		if constexpr (sizeof...(args) > 0) {
			BindParameters(hstmt, 1, args...);
		}

		ret = SQLExecute(hstmt);
		if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
			Show_Error(hstmt, SQL_HANDLE_STMT, ret);
			return false;
		}

		return true;
	}
}