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

    bool CDataBase::Connect(const std::wstring& database)
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

        ret = SQLConnect(m_hdbc, (SQLWCHAR*)database.c_str(), SQL_NTS, (SQLWCHAR*)NULL, 0, NULL, 0);
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

    bool CDataBase::GetPlayerInfo(const std::string& id, const std::string& password, PlayerInfo& playerInfo)
    {
        SQLHSTMT hstmt;
        if (!AllocStatementHandle(hstmt))
            return false;

        if (!ExecuteSP(hstmt, STORED_PROCEDURE::GET_PLAYER_INFO, id, password)) {
            FreeStatementHandle(hstmt);
            return false;
        }
        
        SQLLEN indicator = 0;
        SQLBindCol(hstmt, 1, SQL_C_LONG, &playerInfo.tokenNum, sizeof(playerInfo.tokenNum), &indicator);
        SQLBindCol(hstmt, 2, SQL_C_SHORT, &playerInfo.model.Chr_Sex, sizeof(short), &indicator);
        SQLBindCol(hstmt, 3, SQL_C_SHORT, &playerInfo.model.Chr_HeadCoverings_Base_Hair, sizeof(short), &indicator);
        SQLBindCol(hstmt, 4, SQL_C_SHORT, &playerInfo.model.Chr_HeadCoverings_No_FacialHair, sizeof(short), &indicator);
        SQLBindCol(hstmt, 5, SQL_C_SHORT, &playerInfo.model.Chr_HeadCoverings_No_Hair, sizeof(short), &indicator);
        SQLBindCol(hstmt, 6, SQL_C_SHORT, &playerInfo.model.Chr_Hair, sizeof(short), &indicator);
        SQLBindCol(hstmt, 7, SQL_C_SHORT, &playerInfo.model.Chr_HelmetAttachment, sizeof(short), &indicator);
        SQLBindCol(hstmt, 8, SQL_C_SHORT, &playerInfo.model.Chr_BackAttachment, sizeof(short), &indicator);
        SQLBindCol(hstmt, 9, SQL_C_SHORT, &playerInfo.model.Chr_ShoulderAttachRight, sizeof(short), &indicator);
        SQLBindCol(hstmt, 10, SQL_C_SHORT, &playerInfo.model.Chr_ShoulderAttachLeft, sizeof(short), &indicator);
        SQLBindCol(hstmt, 11, SQL_C_SHORT, &playerInfo.model.Chr_ElbowAttachRight, sizeof(short), &indicator);
        SQLBindCol(hstmt, 12, SQL_C_SHORT, &playerInfo.model.Chr_ElbowAttachLeft, sizeof(short), &indicator);
        SQLBindCol(hstmt, 13, SQL_C_SHORT, &playerInfo.model.Chr_HipsAttachment, sizeof(short), &indicator);
        SQLBindCol(hstmt, 14, SQL_C_SHORT, &playerInfo.model.Chr_KneeAttachRight, sizeof(short), &indicator);
        SQLBindCol(hstmt, 15, SQL_C_SHORT, &playerInfo.model.Chr_KneeAttachLeft, sizeof(short), &indicator);
        SQLBindCol(hstmt, 16, SQL_C_SHORT, &playerInfo.model.Chr_Ear_Ear, sizeof(short), &indicator);
        SQLBindCol(hstmt, 17, SQL_C_SHORT, &playerInfo.model.Chr_Head, sizeof(short), &indicator);
        SQLBindCol(hstmt, 18, SQL_C_SHORT, &playerInfo.model.Chr_Head_No_Elements, sizeof(short), &indicator);
        SQLBindCol(hstmt, 19, SQL_C_SHORT, &playerInfo.model.Chr_Eyebrow, sizeof(short), &indicator);
        SQLBindCol(hstmt, 20, SQL_C_SHORT, &playerInfo.model.Chr_Torso, sizeof(short), &indicator);
        SQLBindCol(hstmt, 21, SQL_C_SHORT, &playerInfo.model.Chr_ArmUpperRight, sizeof(short), &indicator);
        SQLBindCol(hstmt, 22, SQL_C_SHORT, &playerInfo.model.Chr_ArmUpperLeft, sizeof(short), &indicator);
        SQLBindCol(hstmt, 23, SQL_C_SHORT, &playerInfo.model.Chr_ArmLowerRight, sizeof(short), &indicator);
        SQLBindCol(hstmt, 24, SQL_C_SHORT, &playerInfo.model.Chr_ArmLowerLeft, sizeof(short), &indicator);
        SQLBindCol(hstmt, 25, SQL_C_SHORT, &playerInfo.model.Chr_HandRight, sizeof(short), &indicator);
        SQLBindCol(hstmt, 26, SQL_C_SHORT, &playerInfo.model.Chr_HandLeft, sizeof(short), &indicator);
        SQLBindCol(hstmt, 27, SQL_C_SHORT, &playerInfo.model.Chr_Hips, sizeof(short), &indicator);
        SQLBindCol(hstmt, 28, SQL_C_SHORT, &playerInfo.model.Chr_LegRight, sizeof(short), &indicator);
        SQLBindCol(hstmt, 29, SQL_C_SHORT, &playerInfo.model.Chr_LegLeft, sizeof(short), &indicator);

        SQLRETURN ret = SQLFetch(hstmt);
        if (ret == SQL_NO_DATA)
        {
            FreeStatementHandle(hstmt);
            return false;
        }
        
        FreeStatementHandle(hstmt);
        return true;
    }

    bool CDataBase::SavePlayerInfo(const std::string& id, const PlayerInfo& info)
    {
        return ExecuteSP(STORED_PROCEDURE::SAVE_PLAYER_INFO, id, info.tokenNum, info.model.Chr_Sex, info.model.Chr_HeadCoverings_Base_Hair,
            info.model.Chr_HeadCoverings_No_FacialHair, info.model.Chr_HeadCoverings_No_Hair, info.model.Chr_Hair,
            info.model.Chr_HelmetAttachment, info.model.Chr_BackAttachment, info.model.Chr_ShoulderAttachRight,
            info.model.Chr_ShoulderAttachLeft, info.model.Chr_ElbowAttachRight, info.model.Chr_ElbowAttachLeft,
            info.model.Chr_HipsAttachment, info.model.Chr_KneeAttachRight, info.model.Chr_KneeAttachLeft,
            info.model.Chr_Ear_Ear, info.model.Chr_Head, info.model.Chr_Head_No_Elements, info.model.Chr_Eyebrow,
            info.model.Chr_Torso, info.model.Chr_ArmUpperRight, info.model.Chr_ArmUpperLeft, info.model.Chr_ArmLowerRight,
            info.model.Chr_ArmLowerLeft, info.model.Chr_HandRight, info.model.Chr_HandLeft, info.model.Chr_Hips,
            info.model.Chr_LegRight, info.model.Chr_LegLeft);
    }

    bool CDataBase::SaveToken(const std::string& id, int token)
    {
        return ExecuteSP(STORED_PROCEDURE::SAVE_TOKEN, id, token);
    }

    bool CDataBase::RegisterAuction(const std::string& id, const int shopType, const int shopNum, const int buyPrice, const std::string& deadLine)
    {
        return ExecuteSP(STORED_PROCEDURE::REGISTER_AUCTION, id, shopType, shopNum, buyPrice, deadLine);
    }

    bool CDataBase::BuyAuction(const std::string& seller, const std::string& myID, short type, short num, unsigned short price)
    {
        return ExecuteSP(STORED_PROCEDURE::BUY_AUCTION, seller, myID, type, num, price);
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

    bool CDataBase::StakeTokens(const std::string& id, int tokenNum, unsigned short stakeDays)
    {
        return ExecuteSP(STORED_PROCEDURE::STAKE_TOKEN, id, tokenNum, stakeDays);
    }

    bool CDataBase::ChangeNodeType(const std::string& id, bool fullNode)
    {
        return ExecuteSP(STORED_PROCEDURE::CHANGE_NODE, id, fullNode);
    }

    bool CDataBase::GetFullNodeInfo(const std::string& id, bool& fullNode)
    {
        SQLHSTMT hstmt;
        if (!AllocStatementHandle(hstmt))
            return false;

        if (!ExecuteSP(hstmt, STORED_PROCEDURE::GET_FULL_NODE_INFO, id)) {
            FreeStatementHandle(hstmt);
            return false;
        }

        SQLLEN cbFullNode = 0;
        SQLBindCol(hstmt, 1, SQL_C_BIT, &fullNode, 0, &cbFullNode);
        
        SQLRETURN ret = SQLFetch(hstmt);
        if (ret == SQL_NO_DATA) {
            FreeStatementHandle(hstmt);
            return false;
        }

        FreeStatementHandle(hstmt);
        return true;
    }

    bool CDataBase::GetStakedTokenData(const std::string& id, int& stakedToken, int& stakedDays, int& dayCount)
    {
        SQLHSTMT hstmt;
        if (!AllocStatementHandle(hstmt))
            return false;

        if (!ExecuteSP(hstmt, STORED_PROCEDURE::GET_STAKED_TOKEN_DATA, id)) {
            FreeStatementHandle(hstmt);
            return false;
        }

        SQLLEN indicator = 0;
        SQLBindCol(hstmt, 1, SQL_C_LONG, &stakedToken, sizeof(int), &indicator);
        SQLBindCol(hstmt, 2, SQL_C_LONG, &stakedDays, sizeof(int), &indicator);
        SQLBindCol(hstmt, 3, SQL_C_LONG, &dayCount, sizeof(int), &indicator);

        SQLRETURN ret = SQLFetch(hstmt);
        if (ret == SQL_NO_DATA) {
            FreeStatementHandle(hstmt);
            return false;
        }

        FreeStatementHandle(hstmt);
        return true;
    }

    bool CDataBase::CheckIdExists(const std::string& id)
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

        BindParameter(hstmt, 1, id);

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

    bool CDataBase::CheckPassword(const std::string& id, const std::string& password)
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

        BindParameter(hstmt, 1, id);
        BindParameter(hstmt, 2, password);

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

    bool CDataBase::SignUp(const std::string& id, const std::string& password)
    {
        return ExecuteSP(STORED_PROCEDURE::SIGN_UP, id, password);
    }

    std::vector<AuctionInfo> CDataBase::GetAuctionInfo(int pageNum)
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


        if (!ExecuteSP(hstmt, STORED_PROCEDURE::GET_AUCTION_INFO, pageNum)) {
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

    bool CDataBase::GetPlayerCustomizeParts(const std::string& id, std::vector<std::tuple<short, short, short>>& customizeParts)
    {
        SQLHSTMT hstmt;
        if (!AllocStatementHandle(hstmt))
            return false;

        if (!ExecuteSP(hstmt, STORED_PROCEDURE::GET_PLAYER_CUSTOMIZE_PARTS, id)) {
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
            customizeParts.emplace_back(shopType, customizeNum, count);
        }

        FreeStatementHandle(hstmt);
        return true;
    }

    bool CDataBase::SaveShopBuyInfo(const std::string& id, const int shopType, const int shopNum)
    {
        return ExecuteSP(STORED_PROCEDURE::SAVE_SHOP_BUY_INFO, id, shopType, shopNum);
    }

    std::vector<std::string> CDataBase::GetPlayersByShopType(short shopType)
    {
        SQLHSTMT hstmt;
        if (!AllocStatementHandle(hstmt))
            return {};

        if (!ExecuteSP(hstmt, STORED_PROCEDURE::GET_PLAYERS_BY_SHOP_TYPE, shopType)) {
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

    void CDataBase::Show_Error(SQLHANDLE hHandle, SQLSMALLINT hType, RETCODE RetCode)
    {
        SQLSMALLINT iRec = 0;
        SQLINTEGER iError;
        WCHAR wszMessage[1000];
        WCHAR wszState[SQL_SQLSTATE_SIZE + 1];
        if (RetCode == SQL_INVALID_HANDLE) {
            fwprintf(stderr, L"Invalid handle!\n");
            return;
        }
        while (SQLGetDiagRec(hType, hHandle, ++iRec, wszState, &iError, wszMessage,
            (SQLSMALLINT)(sizeof(wszMessage) / sizeof(WCHAR)), (SQLSMALLINT*)NULL) == SQL_SUCCESS) {
            if (wcsncmp(wszState, L"01004", 5)) {
                fwprintf(stderr, L"[%5.5s] %s (%d)\n", wszState, wszMessage, iError);
            }
        }
    }

    bool CDataBase::AllocStatementHandle(SQLHSTMT& hstmt)
    {
        SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_STMT, m_hdbc, &hstmt);
        if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
            LogPrinter::PrintMsg("Failed To Alloc Statement Handle");
            return false;
        }
        return true;
    }

    void CDataBase::FreeStatementHandle(SQLHSTMT hstmt)
    {
        if (hstmt != NULL) {
            SQLCloseCursor(hstmt);
            SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
        }
    }
}
