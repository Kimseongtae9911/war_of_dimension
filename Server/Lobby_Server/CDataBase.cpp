#include "pch.h"
#include "CDataBase.h"


namespace wod_server {
    CDataBase::CDataBase() : m_henv(NULL), m_hdbc(NULL)
    {
        setlocale(LC_ALL, "korean");

        m_spMap.emplace(STORED_PROCEDURE::GET_PLAYER_INFO, L"GetPlayerInfo");
        m_spMap.emplace(STORED_PROCEDURE::SAVE_PLAYER_INFO, L"SavePlayerInfo");
        m_spMap.emplace(STORED_PROCEDURE::SAVE_TOKEN, L"SaveToken");
        m_spMap.emplace(STORED_PROCEDURE::REGISTER_AUCTION, L"RegisterAuction");
        m_spMap.emplace(STORED_PROCEDURE::BUY_AUCTION, L"BuyAuction");
        m_spMap.emplace(STORED_PROCEDURE::GET_VALIDATOR_IDS, L"GetValidatorIDs");
        m_spMap.emplace(STORED_PROCEDURE::STAKE_TOKEN, L"StakeToken");
        m_spMap.emplace(STORED_PROCEDURE::CHANGE_NODE, L"ChangeNode");
        m_spMap.emplace(STORED_PROCEDURE::GET_FULL_NODE_INFO, L"GetFullNodeInfo");
        m_spMap.emplace(STORED_PROCEDURE::GET_STAKED_TOKEN_DATA, L"GetStakedTokenData");
        m_spMap.emplace(STORED_PROCEDURE::SIGN_UP, L"SignUp");
        m_spMap.emplace(STORED_PROCEDURE::GET_AUCTION_INFO, L"GetAuctionInfo");
        m_spMap.emplace(STORED_PROCEDURE::GET_AUCTION_PARTS_NUM, L"GetAuctionPartsNum");
        m_spMap.emplace(STORED_PROCEDURE::GET_PLAYER_CUSTOMIZE_PARTS, L"GetPlayerCustomizeParts");
        m_spMap.emplace(STORED_PROCEDURE::SAVE_SHOP_BUY_INFO, L"SaveShopBuyInfo");
        m_spMap.emplace(STORED_PROCEDURE::GET_PLAYERS_BY_SHOP_TYPE, L"GetPlayersByShopType");
    }

    CDataBase::~CDataBase()
    {
        Disconnect();
    }

    bool CDataBase::Connect(const std::wstring& _database)
    {
        SQLRETURN ret;

        ret = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &m_henv);
        if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO)
        {
            LogPrinter::PrintMsg("Failed to allocate environment handle.");
            return false;
        }

        ret = SQLSetEnvAttr(m_henv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);
        if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO)
        {
            LogPrinter::PrintMsg("Failed to set ODBC version.");
            Disconnect();
            return false;
        }

        ret = SQLAllocHandle(SQL_HANDLE_DBC, m_henv, &m_hdbc);
        if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO)
        {
            LogPrinter::PrintMsg("Failed to allocate connection handle.");
            Disconnect();
            return false;
        }

        ret = SQLConnect(m_hdbc, (SQLWCHAR*)_database.c_str(), SQL_NTS, (SQLWCHAR*)NULL, 0, NULL, 0);
        if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO)
        {
            LogPrinter::PrintMsg("Failed to connect to the database.");
            Disconnect();
            return false;
        }

        return true;
    }

    void CDataBase::Disconnect()
    {
        if (m_hdbc != NULL)
        {
            SQLDisconnect(m_hdbc);
            SQLFreeHandle(SQL_HANDLE_DBC, m_hdbc);
            m_hdbc = NULL;
        }

        if (m_henv != NULL)
        {
            SQLFreeHandle(SQL_HANDLE_ENV, m_henv);
            m_henv = NULL;
        }
    }

    bool CDataBase::GetPlayerInfo(const std::string& _id, const std::string& _password, PlayerInfo& _playerInfo)
    {
        SQLHSTMT hstmt;
        if (!AllocStatementHandle(hstmt))
            return false;

        if (!ExecuteSP(hstmt, STORED_PROCEDURE::GET_PLAYER_INFO, _id, _password)) {
            FreeStatementHandle(hstmt);
            return false;
        }

        SQLLEN indicator = 0;
        SQLBindCol(hstmt, 1, SQL_C_LONG, &_playerInfo.m_tokenNum, sizeof(_playerInfo.m_tokenNum), &indicator);
        SQLBindCol(hstmt, 2, SQL_C_SHORT, &_playerInfo.m_model.Chr_Sex, sizeof(short), &indicator);
        SQLBindCol(hstmt, 3, SQL_C_SHORT, &_playerInfo.m_model.Chr_HeadCoverings_Base_Hair, sizeof(short), &indicator);
        SQLBindCol(hstmt, 4, SQL_C_SHORT, &_playerInfo.m_model.Chr_HeadCoverings_No_FacialHair, sizeof(short), &indicator);
        SQLBindCol(hstmt, 5, SQL_C_SHORT, &_playerInfo.m_model.Chr_HeadCoverings_No_Hair, sizeof(short), &indicator);
        SQLBindCol(hstmt, 6, SQL_C_SHORT, &_playerInfo.m_model.Chr_Hair, sizeof(short), &indicator);
        SQLBindCol(hstmt, 7, SQL_C_SHORT, &_playerInfo.m_model.Chr_HelmetAttachment, sizeof(short), &indicator);
        SQLBindCol(hstmt, 8, SQL_C_SHORT, &_playerInfo.m_model.Chr_BackAttachment, sizeof(short), &indicator);
        SQLBindCol(hstmt, 9, SQL_C_SHORT, &_playerInfo.m_model.Chr_ShoulderAttachRight, sizeof(short), &indicator);
        SQLBindCol(hstmt, 10, SQL_C_SHORT, &_playerInfo.m_model.Chr_ShoulderAttachLeft, sizeof(short), &indicator);
        SQLBindCol(hstmt, 11, SQL_C_SHORT, &_playerInfo.m_model.Chr_ElbowAttachRight, sizeof(short), &indicator);
        SQLBindCol(hstmt, 12, SQL_C_SHORT, &_playerInfo.m_model.Chr_ElbowAttachLeft, sizeof(short), &indicator);
        SQLBindCol(hstmt, 13, SQL_C_SHORT, &_playerInfo.m_model.Chr_HipsAttachment, sizeof(short), &indicator);
        SQLBindCol(hstmt, 14, SQL_C_SHORT, &_playerInfo.m_model.Chr_KneeAttachRight, sizeof(short), &indicator);
        SQLBindCol(hstmt, 15, SQL_C_SHORT, &_playerInfo.m_model.Chr_KneeAttachLeft, sizeof(short), &indicator);
        SQLBindCol(hstmt, 16, SQL_C_SHORT, &_playerInfo.m_model.Chr_Ear_Ear, sizeof(short), &indicator);
        SQLBindCol(hstmt, 17, SQL_C_SHORT, &_playerInfo.m_model.Chr_Head, sizeof(short), &indicator);
        SQLBindCol(hstmt, 18, SQL_C_SHORT, &_playerInfo.m_model.Chr_Head_No_Elements, sizeof(short), &indicator);
        SQLBindCol(hstmt, 19, SQL_C_SHORT, &_playerInfo.m_model.Chr_Eyebrow, sizeof(short), &indicator);
        SQLBindCol(hstmt, 20, SQL_C_SHORT, &_playerInfo.m_model.Chr_Torso, sizeof(short), &indicator);
        SQLBindCol(hstmt, 21, SQL_C_SHORT, &_playerInfo.m_model.Chr_ArmUpperRight, sizeof(short), &indicator);
        SQLBindCol(hstmt, 22, SQL_C_SHORT, &_playerInfo.m_model.Chr_ArmUpperLeft, sizeof(short), &indicator);
        SQLBindCol(hstmt, 23, SQL_C_SHORT, &_playerInfo.m_model.Chr_ArmLowerRight, sizeof(short), &indicator);
        SQLBindCol(hstmt, 24, SQL_C_SHORT, &_playerInfo.m_model.Chr_ArmLowerLeft, sizeof(short), &indicator);
        SQLBindCol(hstmt, 25, SQL_C_SHORT, &_playerInfo.m_model.Chr_HandRight, sizeof(short), &indicator);
        SQLBindCol(hstmt, 26, SQL_C_SHORT, &_playerInfo.m_model.Chr_HandLeft, sizeof(short), &indicator);
        SQLBindCol(hstmt, 27, SQL_C_SHORT, &_playerInfo.m_model.Chr_Hips, sizeof(short), &indicator);
        SQLBindCol(hstmt, 28, SQL_C_SHORT, &_playerInfo.m_model.Chr_LegRight, sizeof(short), &indicator);
        SQLBindCol(hstmt, 29, SQL_C_SHORT, &_playerInfo.m_model.Chr_LegLeft, sizeof(short), &indicator);

        SQLRETURN ret = SQLFetch(hstmt);
        if (ret == SQL_NO_DATA)
        {
            FreeStatementHandle(hstmt);
            return false;
        }

        FreeStatementHandle(hstmt);
        return true;
    }

    bool CDataBase::SavePlayerInfo(const std::string& _id, const PlayerInfo& _info)
    {
        return ExecuteSP(STORED_PROCEDURE::SAVE_PLAYER_INFO, _id, _info.m_tokenNum, _info.m_model.Chr_Sex, _info.m_model.Chr_HeadCoverings_Base_Hair,
            _info.m_model.Chr_HeadCoverings_No_FacialHair, _info.m_model.Chr_HeadCoverings_No_Hair, _info.m_model.Chr_Hair,
            _info.m_model.Chr_HelmetAttachment, _info.m_model.Chr_BackAttachment, _info.m_model.Chr_ShoulderAttachRight,
            _info.m_model.Chr_ShoulderAttachLeft, _info.m_model.Chr_ElbowAttachRight, _info.m_model.Chr_ElbowAttachLeft,
            _info.m_model.Chr_HipsAttachment, _info.m_model.Chr_KneeAttachRight, _info.m_model.Chr_KneeAttachLeft,
            _info.m_model.Chr_Ear_Ear, _info.m_model.Chr_Head, _info.m_model.Chr_Head_No_Elements, _info.m_model.Chr_Eyebrow,
            _info.m_model.Chr_Torso, _info.m_model.Chr_ArmUpperRight, _info.m_model.Chr_ArmUpperLeft, _info.m_model.Chr_ArmLowerRight,
            _info.m_model.Chr_ArmLowerLeft, _info.m_model.Chr_HandRight, _info.m_model.Chr_HandLeft, _info.m_model.Chr_Hips,
            _info.m_model.Chr_LegRight, _info.m_model.Chr_LegLeft);
    }

    bool CDataBase::SaveToken(const std::string& _id, int _token)
    {
        return ExecuteSP(STORED_PROCEDURE::SAVE_TOKEN, _id, _token);
    }

    bool CDataBase::RegisterAuction(const std::string& _id, const int _shopType, const int _shopNum, const int _buyPrice, const std::string& _deadLine)
    {
        return ExecuteSP(STORED_PROCEDURE::REGISTER_AUCTION, _id, _shopType, _shopNum, _buyPrice, _deadLine);
    }

    bool CDataBase::BuyAuction(const std::string& _seller, const std::string& _myID, short _type, short _num, unsigned short _price)
    {
        return ExecuteSP(STORED_PROCEDURE::BUY_AUCTION, _seller, _myID, _type, _num, _price);
    }

    std::vector<std::pair<std::string, int>> CDataBase::GetValidatorIDs()
    {
        SQLHSTMT hstmt;
        if (!AllocStatementHandle(hstmt))
            return {};

        if (!ExecuteSP(hstmt, STORED_PROCEDURE::GET_VALIDATOR_IDS)) {
            FreeStatementHandle(hstmt);
            return {};
        }

        std::vector<std::pair<std::string, int>> validatorInfos;
        SQLCHAR validatorID[NAME_SIZE + 1];
        SQLINTEGER stakedToken;
        SQLLEN cbValidatorID, cbStakedToken;

        while (SQLFetch(hstmt) == SQL_SUCCESS) {
            SQLGetData(hstmt, 1, SQL_C_CHAR, validatorID, sizeof(validatorID), &cbValidatorID);
            SQLGetData(hstmt, 2, SQL_C_SLONG, &stakedToken, sizeof(stakedToken), &cbStakedToken);
            validatorInfos.emplace_back(std::string(reinterpret_cast<char*>(validatorID), cbValidatorID), stakedToken);
        }

        FreeStatementHandle(hstmt);
        return validatorInfos;
    }

    bool CDataBase::StakeTokens(const std::string& _id, int _tokenNum, unsigned short _stakeDays)
    {
        return ExecuteSP(STORED_PROCEDURE::STAKE_TOKEN, _id, _tokenNum, _stakeDays);
    }

    bool CDataBase::ChangeNodeType(const std::string& _id, bool _fullNode)
    {
        return ExecuteSP(STORED_PROCEDURE::CHANGE_NODE, _id, _fullNode);
    }

    bool CDataBase::GetFullNodeInfo(const std::string& _id, bool& _fullNode)
    {
        SQLHSTMT hstmt;
        if (!AllocStatementHandle(hstmt))
            return false;

        if (!ExecuteSP(hstmt, STORED_PROCEDURE::GET_FULL_NODE_INFO, _id)) {
            FreeStatementHandle(hstmt);
            return false;
        }

        SQLLEN cbFullNode = 0;
        SQLBindCol(hstmt, 1, SQL_C_BIT, &_fullNode, 0, &cbFullNode);

        SQLRETURN ret = SQLFetch(hstmt);
        if (ret == SQL_NO_DATA) {
            FreeStatementHandle(hstmt);
            return false;
        }

        FreeStatementHandle(hstmt);
        return true;
    }

    bool CDataBase::GetStakedTokenData(const std::string& _id, int& _stakedToken, int& _stakedDays, int& _dayCount)
    {
        SQLHSTMT hstmt;
        if (!AllocStatementHandle(hstmt))
            return false;

        if (!ExecuteSP(hstmt, STORED_PROCEDURE::GET_STAKED_TOKEN_DATA, _id)) {
            FreeStatementHandle(hstmt);
            return false;
        }

        SQLLEN indicator = 0;
        SQLBindCol(hstmt, 1, SQL_C_LONG, &_stakedToken, sizeof(int), &indicator);
        SQLBindCol(hstmt, 2, SQL_C_LONG, &_stakedDays, sizeof(int), &indicator);
        SQLBindCol(hstmt, 3, SQL_C_LONG, &_dayCount, sizeof(int), &indicator);

        SQLRETURN ret = SQLFetch(hstmt);
        if (ret == SQL_NO_DATA) {
            FreeStatementHandle(hstmt);
            return false;
        }

        FreeStatementHandle(hstmt);
        return true;
    }

    bool CDataBase::CheckIdExists(const std::string& _id)
    {
        SQLHSTMT hstmt;
        if (!AllocStatementHandle(hstmt))
            return false;

        std::string query = "SELECT COUNT(*) FROM dbo.user_data WHERE Player_ID = ?";

        SQLRETURN ret = SQLPrepareA(hstmt, (SQLCHAR*)query.c_str(), SQL_NTS);
        if (ret != SQL_SUCCESS) {
            FreeStatementHandle(hstmt);
            return false;
        }

        BindParameter(hstmt, 1, _id);

        ret = SQLExecute(hstmt);
        if (ret != SQL_SUCCESS) {
            FreeStatementHandle(hstmt);
            return false;
        }

        SQLLEN count = 0;
        SQLBindCol(hstmt, 1, SQL_C_LONG, &count, sizeof(count), NULL);

        ret = SQLFetch(hstmt);
        if (ret == SQL_NO_DATA || ret != SQL_SUCCESS) {
            FreeStatementHandle(hstmt);
            return false;
        }

        FreeStatementHandle(hstmt);
        return count > 0;
    }

    bool CDataBase::CheckPassword(const std::string& _id, const std::string& _password)
    {
        SQLHSTMT hstmt;
        if (!AllocStatementHandle(hstmt))
            return false;

        std::string query = "SELECT COUNT(*) FROM dbo.user_data WHERE Player_ID = ? AND Player_Password = ?";

        SQLRETURN ret = SQLPrepareA(hstmt, (SQLCHAR*)query.c_str(), SQL_NTS);
        if (ret != SQL_SUCCESS) {
            FreeStatementHandle(hstmt);
            return false;
        }

        BindParameter(hstmt, 1, _id);
        BindParameter(hstmt, 2, _password);

        ret = SQLExecute(hstmt);
        if (ret != SQL_SUCCESS) {
            FreeStatementHandle(hstmt);
            return false;
        }

        SQLLEN result = 0;
        SQLBindCol(hstmt, 1, SQL_C_LONG, &result, sizeof(result), NULL);

        ret = SQLFetch(hstmt);
        if (ret != SQL_SUCCESS) {
            FreeStatementHandle(hstmt);
            return false;
        }

        FreeStatementHandle(hstmt);
        return result > 0;
    }

    bool CDataBase::SignUp(const std::string& _id, const std::string& _password)
    {
        return ExecuteSP(STORED_PROCEDURE::SIGN_UP, _id, _password);
    }

    std::vector<AuctionInfo> CDataBase::GetAuctionInfo(int _pageNum)
    {
        SQLHSTMT hstmt;
        if (!AllocStatementHandle(hstmt))
            return {};

        // You might want to create a separate SP for deleting expired auctions
        // For now, keeping the direct delete query
        std::time_t currentTime = std::chrono::system_clock::to_time_t(std::chrono::time_point_cast<std::chrono::seconds>(std::chrono::system_clock::now()));
        std::ostringstream deadLine;
        std::tm localTime;
        localtime_s(&localTime, &currentTime);
        deadLine << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S");
        std::string deleteQuery = "DELETE FROM auction_data WHERE DeadLine <= '" + deadLine.str() + "'";
        SQLExecDirectA(hstmt, (SQLCHAR*)deleteQuery.c_str(), SQL_NTS);
        SQLCloseCursor(hstmt);


        if (!ExecuteSP(hstmt, STORED_PROCEDURE::GET_AUCTION_INFO, _pageNum)) {
            FreeStatementHandle(hstmt);
            return {};
        }

        std::vector<AuctionInfo> result;
        while (SQLFetch(hstmt) == SQL_SUCCESS)
        {
            SQLCHAR playerID[11];
            SQLSMALLINT customizeType;
            SQLSMALLINT customizeNum;
            SQLINTEGER buyPrice;
            SQL_TIMESTAMP_STRUCT deadline;

            SQLGetData(hstmt, 1, SQL_C_CHAR, playerID, sizeof(playerID), NULL);
            SQLGetData(hstmt, 2, SQL_C_SHORT, &customizeType, sizeof(customizeType), NULL);
            SQLGetData(hstmt, 3, SQL_C_SHORT, &customizeNum, sizeof(customizeNum), NULL);
            SQLGetData(hstmt, 4, SQL_C_LONG, &buyPrice, sizeof(buyPrice), NULL);
            SQLGetData(hstmt, 5, SQL_C_TYPE_TIMESTAMP, &deadline, sizeof(deadline), NULL);

            AuctionInfo auctionInfo;
            memcpy_s(auctionInfo.playerName, NAME_SIZE, playerID, NAME_SIZE);
            auctionInfo.customizeType = customizeType;
            auctionInfo.customizeNum = customizeNum;
            auctionInfo.buyPrice = static_cast<unsigned short>(buyPrice);
            sprintf_s(auctionInfo.deadLine, "%04d-%02d-%02d %02d:%02d", deadline.year, deadline.month, deadline.day, deadline.hour, deadline.minute);

            result.push_back(auctionInfo);
        }

        FreeStatementHandle(hstmt);
        return result;
    }

    int CDataBase::GetAuctionPartsNum()
    {
        SQLHSTMT hstmt;
        if (!AllocStatementHandle(hstmt))
            return -1;

        if (!ExecuteSP(hstmt, STORED_PROCEDURE::GET_AUCTION_PARTS_NUM)) {
            FreeStatementHandle(hstmt);
            return -1;
        }

        SQLINTEGER rowCount = 0;
        SQLBindCol(hstmt, 1, SQL_C_LONG, &rowCount, 0, NULL);

        SQLRETURN ret = SQLFetch(hstmt);
        if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
            FreeStatementHandle(hstmt);
            return -1;
        }

        FreeStatementHandle(hstmt);
        return rowCount;
    }

    bool CDataBase::GetPlayerCustomizeParts(const std::string& _id, std::vector<std::tuple<short, short, short>>& _customizeParts)
    {
        SQLHSTMT hstmt;
        if (!AllocStatementHandle(hstmt))
            return false;

        if (!ExecuteSP(hstmt, STORED_PROCEDURE::GET_PLAYER_CUSTOMIZE_PARTS, _id)) {
            FreeStatementHandle(hstmt);
            return false;
        }

        short shopType = 0;
        short customizeNum = 0;
        short count = 0;
        SQLLEN indicator;

        while (SQLFetch(hstmt) == SQL_SUCCESS) {
            SQLGetData(hstmt, 1, SQL_C_SHORT, &shopType, sizeof(shopType), &indicator);
            SQLGetData(hstmt, 2, SQL_C_SHORT, &customizeNum, sizeof(customizeNum), &indicator);
            SQLGetData(hstmt, 3, SQL_C_SHORT, &count, sizeof(count), &indicator);
            _customizeParts.emplace_back(shopType, customizeNum, count);
        }

        FreeStatementHandle(hstmt);
        return true;
    }

    bool CDataBase::SaveShopBuyInfo(const std::string& _id, const int _shopType, const int _shopNum)
    {
        return ExecuteSP(STORED_PROCEDURE::SAVE_SHOP_BUY_INFO, _id, _shopType, _shopNum);
    }

    std::vector<std::string> CDataBase::GetPlayersByShopType(short _shopType)
    {
        SQLHSTMT hstmt;
        if (!AllocStatementHandle(hstmt))
            return {};

        if (!ExecuteSP(hstmt, STORED_PROCEDURE::GET_PLAYERS_BY_SHOP_TYPE, _shopType)) {
            FreeStatementHandle(hstmt);
            return {};
        }

        std::vector<std::string> playerIDs;
        SQLCHAR playerID[NAME_SIZE + 1];
        SQLLEN cbPlayerID;

        while (SQLFetch(hstmt) == SQL_SUCCESS) {
            SQLGetData(hstmt, 1, SQL_C_CHAR, playerID, sizeof(playerID), &cbPlayerID);
            playerIDs.emplace_back(std::string(reinterpret_cast<char*>(playerID), cbPlayerID));
        }

        FreeStatementHandle(hstmt);
        return playerIDs;
    }

    void CDataBase::Show_Error(SQLHANDLE _hHandle, SQLSMALLINT _hType, RETCODE _retCode)
    {
        SQLSMALLINT iRec = 0;
        SQLINTEGER iError;
        WCHAR wszMessage[1000];
        WCHAR wszState[SQL_SQLSTATE_SIZE + 1];
        if (_retCode == SQL_INVALID_HANDLE) {
            fwprintf(stderr, L"Invalid handle!\n");
            return;
        }
        while (SQLGetDiagRec(_hType, _hHandle, ++iRec, wszState, &iError, wszMessage,
            (SQLSMALLINT)(sizeof(wszMessage) / sizeof(WCHAR)), (SQLSMALLINT*)NULL) == SQL_SUCCESS) {
            if (wcsncmp(wszState, L"01004", 5)) {
                fwprintf(stderr, L"[%5.5s] %s (%d)\n", wszState, wszMessage, iError);
            }
        }
    }

    bool CDataBase::AllocStatementHandle(SQLHSTMT& _hstmt)
    {
        SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_STMT, m_hdbc, &_hstmt);
        if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
            LogPrinter::PrintMsg("Failed To Alloc Statement Handle");
            return false;
        }
        return true;
    }

    void CDataBase::FreeStatementHandle(SQLHSTMT _hstmt)
    {
        if (_hstmt != NULL) {
            SQLCloseCursor(_hstmt);
            SQLFreeHandle(SQL_HANDLE_STMT, _hstmt);
        }
    }
}
