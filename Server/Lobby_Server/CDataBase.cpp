#include "pch.h"
#include "CDataBase.h"
#include "PasswordHash.h"

namespace wod_server
{
CDataBase::CDataBase() : m_henv(NULL), m_hdbc(NULL)
{

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

    if (!SQL_SUCCEEDED(SQLSetConnectAttr(m_hdbc, SQL_ATTR_LOGIN_TIMEOUT, reinterpret_cast<SQLPOINTER>(5), 0)))
    {
        Disconnect();
        return false;
    }

    ret = SQLDriverConnectW(m_hdbc, nullptr, reinterpret_cast<SQLWCHAR*>(const_cast<wchar_t*>(_database.c_str())), SQL_NTS, nullptr, 0, nullptr, SQL_DRIVER_NOPROMPT);
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
    ResetRequest();
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
    std::string encoded;
    if (!VerifyCredential(_id, _password, encoded))
        return false;

    SQLHSTMT hstmt;
    if (!AllocStatementHandle(hstmt))
        return false;

    if (!ExecuteSP(hstmt, STORED_PROCEDURE::GET_PLAYER_INFO, _id, encoded))
    {
        FreeStatementHandle(hstmt);
        return false;
    }

    SQLLEN indicator = 0;
    CheckResult(SQLBindCol(hstmt, 1, SQL_C_LONG, &_playerInfo.m_tokenNum, sizeof(_playerInfo.m_tokenNum), &indicator), hstmt);
    CheckResult(SQLBindCol(hstmt, 2, SQL_C_SHORT, &_playerInfo.m_model.Chr_Sex, sizeof(short), &indicator), hstmt);
    CheckResult(SQLBindCol(hstmt, 3, SQL_C_SHORT, &_playerInfo.m_model.Chr_HeadCoverings_Base_Hair, sizeof(short), &indicator), hstmt);
    CheckResult(SQLBindCol(hstmt, 4, SQL_C_SHORT, &_playerInfo.m_model.Chr_HeadCoverings_No_FacialHair, sizeof(short), &indicator), hstmt);
    CheckResult(SQLBindCol(hstmt, 5, SQL_C_SHORT, &_playerInfo.m_model.Chr_HeadCoverings_No_Hair, sizeof(short), &indicator), hstmt);
    CheckResult(SQLBindCol(hstmt, 6, SQL_C_SHORT, &_playerInfo.m_model.Chr_Hair, sizeof(short), &indicator), hstmt);
    CheckResult(SQLBindCol(hstmt, 7, SQL_C_SHORT, &_playerInfo.m_model.Chr_HelmetAttachment, sizeof(short), &indicator), hstmt);
    CheckResult(SQLBindCol(hstmt, 8, SQL_C_SHORT, &_playerInfo.m_model.Chr_BackAttachment, sizeof(short), &indicator), hstmt);
    CheckResult(SQLBindCol(hstmt, 9, SQL_C_SHORT, &_playerInfo.m_model.Chr_ShoulderAttachRight, sizeof(short), &indicator), hstmt);
    CheckResult(SQLBindCol(hstmt, 10, SQL_C_SHORT, &_playerInfo.m_model.Chr_ShoulderAttachLeft, sizeof(short), &indicator), hstmt);
    CheckResult(SQLBindCol(hstmt, 11, SQL_C_SHORT, &_playerInfo.m_model.Chr_ElbowAttachRight, sizeof(short), &indicator), hstmt);
    CheckResult(SQLBindCol(hstmt, 12, SQL_C_SHORT, &_playerInfo.m_model.Chr_ElbowAttachLeft, sizeof(short), &indicator), hstmt);
    CheckResult(SQLBindCol(hstmt, 13, SQL_C_SHORT, &_playerInfo.m_model.Chr_HipsAttachment, sizeof(short), &indicator), hstmt);
    CheckResult(SQLBindCol(hstmt, 14, SQL_C_SHORT, &_playerInfo.m_model.Chr_KneeAttachRight, sizeof(short), &indicator), hstmt);
    CheckResult(SQLBindCol(hstmt, 15, SQL_C_SHORT, &_playerInfo.m_model.Chr_KneeAttachLeft, sizeof(short), &indicator), hstmt);
    CheckResult(SQLBindCol(hstmt, 16, SQL_C_SHORT, &_playerInfo.m_model.Chr_Ear_Ear, sizeof(short), &indicator), hstmt);
    CheckResult(SQLBindCol(hstmt, 17, SQL_C_SHORT, &_playerInfo.m_model.Chr_Head, sizeof(short), &indicator), hstmt);
    CheckResult(SQLBindCol(hstmt, 18, SQL_C_SHORT, &_playerInfo.m_model.Chr_Head_No_Elements, sizeof(short), &indicator), hstmt);
    CheckResult(SQLBindCol(hstmt, 19, SQL_C_SHORT, &_playerInfo.m_model.Chr_Eyebrow, sizeof(short), &indicator), hstmt);
    CheckResult(SQLBindCol(hstmt, 20, SQL_C_SHORT, &_playerInfo.m_model.Chr_Torso, sizeof(short), &indicator), hstmt);
    CheckResult(SQLBindCol(hstmt, 21, SQL_C_SHORT, &_playerInfo.m_model.Chr_ArmUpperRight, sizeof(short), &indicator), hstmt);
    CheckResult(SQLBindCol(hstmt, 22, SQL_C_SHORT, &_playerInfo.m_model.Chr_ArmUpperLeft, sizeof(short), &indicator), hstmt);
    CheckResult(SQLBindCol(hstmt, 23, SQL_C_SHORT, &_playerInfo.m_model.Chr_ArmLowerRight, sizeof(short), &indicator), hstmt);
    CheckResult(SQLBindCol(hstmt, 24, SQL_C_SHORT, &_playerInfo.m_model.Chr_ArmLowerLeft, sizeof(short), &indicator), hstmt);
    CheckResult(SQLBindCol(hstmt, 25, SQL_C_SHORT, &_playerInfo.m_model.Chr_HandRight, sizeof(short), &indicator), hstmt);
    CheckResult(SQLBindCol(hstmt, 26, SQL_C_SHORT, &_playerInfo.m_model.Chr_HandLeft, sizeof(short), &indicator), hstmt);
    CheckResult(SQLBindCol(hstmt, 27, SQL_C_SHORT, &_playerInfo.m_model.Chr_Hips, sizeof(short), &indicator), hstmt);
    CheckResult(SQLBindCol(hstmt, 28, SQL_C_SHORT, &_playerInfo.m_model.Chr_LegRight, sizeof(short), &indicator), hstmt);
    CheckResult(SQLBindCol(hstmt, 29, SQL_C_SHORT, &_playerInfo.m_model.Chr_LegLeft, sizeof(short), &indicator), hstmt);

    CheckResult(SQLBindCol(hstmt, 30, SQL_C_UBIGINT, &_playerInfo.m_balanceVersion, sizeof(_playerInfo.m_balanceVersion), &indicator), hstmt);

    SQLRETURN ret = FetchRow(hstmt) ? SQL_SUCCESS : SQL_NO_DATA;
    if (!SQL_SUCCEEDED(ret))
    {
        if (ret != SQL_NO_DATA)
            Show_Error(hstmt, SQL_HANDLE_STMT, ret);

        FreeStatementHandle(hstmt);
        return false;
    }

    FreeStatementHandle(hstmt);
    return true;
}

bool CDataBase::SavePlayerInfo(const std::string& _id, const PlayerInfo& _info)
{
    return ExecuteSP(STORED_PROCEDURE::SAVE_PLAYER_INFO, _id, _info.m_tokenNum, _info.m_model.Chr_Sex, _info.m_model.Chr_HeadCoverings_Base_Hair, _info.m_model.Chr_HeadCoverings_No_FacialHair, _info.m_model.Chr_HeadCoverings_No_Hair,
                     _info.m_model.Chr_Hair, _info.m_model.Chr_HelmetAttachment, _info.m_model.Chr_BackAttachment, _info.m_model.Chr_ShoulderAttachRight, _info.m_model.Chr_ShoulderAttachLeft, _info.m_model.Chr_ElbowAttachRight,
                     _info.m_model.Chr_ElbowAttachLeft, _info.m_model.Chr_HipsAttachment, _info.m_model.Chr_KneeAttachRight, _info.m_model.Chr_KneeAttachLeft, _info.m_model.Chr_Ear_Ear, _info.m_model.Chr_Head, _info.m_model.Chr_Head_No_Elements,
                     _info.m_model.Chr_Eyebrow, _info.m_model.Chr_Torso, _info.m_model.Chr_ArmUpperRight, _info.m_model.Chr_ArmUpperLeft, _info.m_model.Chr_ArmLowerRight, _info.m_model.Chr_ArmLowerLeft, _info.m_model.Chr_HandRight,
                     _info.m_model.Chr_HandLeft, _info.m_model.Chr_Hips, _info.m_model.Chr_LegRight, _info.m_model.Chr_LegLeft);
}

bool CDataBase::SaveToken(const std::string& _id, int _token)
{
    return ExecuteSP(STORED_PROCEDURE::SAVE_TOKEN, _id, _token);
}

PlayerInfo CDataBase::ReadBalance(const std::string& _id)
{
    SQLHSTMT statement = nullptr;
    if (!AllocStatementHandle(statement))
        throw std::runtime_error("balance allocation failed");

    const wchar_t* sql = L"SELECT Token,CONVERT(bigint,BalanceVersion) FROM dbo.user_data WHERE Player_ID=?";
    CheckResult(SQLPrepareW(statement, reinterpret_cast<SQLWCHAR*>(const_cast<wchar_t*>(sql)), SQL_NTS), statement);
    if (!BindParameter(statement, 1, _id))
        throw std::runtime_error("balance parameter binding failed");

    CheckResult(SQLExecute(statement), statement);
    PlayerInfo balance{};
    CheckResult(SQLBindCol(statement, 1, SQL_C_LONG, &balance.m_tokenNum, sizeof(balance.m_tokenNum), nullptr), statement);
    CheckResult(SQLBindCol(statement, 2, SQL_C_UBIGINT, &balance.m_balanceVersion, sizeof(balance.m_balanceVersion), nullptr), statement);
    const bool found = FetchRow(statement);
    FreeStatementHandle(statement);
    ThrowIfFailed();
    if (!found)
        throw std::runtime_error("balance account missing");

    return balance;
}

void CDataBase::ApplyReward(const std::string& _operation, const std::string& _id, int _token)
{
    SQLHSTMT statement = nullptr;
    if (!AllocStatementHandle(statement))
        throw std::runtime_error("reward allocation failed");

    const wchar_t* sql = L"EXEC dbo.ApplyReward ?,?,?";
    CheckResult(SQLPrepareW(statement, reinterpret_cast<SQLWCHAR*>(const_cast<wchar_t*>(sql)), SQL_NTS), statement);
    if (!BindParameters(statement, 1, _operation, _id, _token))
        throw std::runtime_error("reward parameter binding failed");

    auto result = SQLExecute(statement);
    while (SQL_SUCCEEDED(result))
        result = SQLMoreResults(statement);

    if (result != SQL_NO_DATA)
        CheckResult(result, statement);

    FreeStatementHandle(statement);
    ThrowIfFailed();
}

std::wstring CDataBase::JournalNamespace()
{
    SQLHSTMT statement = nullptr;
    if (!AllocStatementHandle(statement))
        throw std::runtime_error("reward namespace allocation failed");

    const wchar_t* sql = L"SELECT CONVERT(nvarchar(36),service_broker_guid)+N'|'+name FROM sys.databases WHERE database_id=DB_ID()";
    CheckResult(SQLExecDirectW(statement, reinterpret_cast<SQLWCHAR*>(const_cast<wchar_t*>(sql)), SQL_NTS), statement);
    if (!FetchRow(statement))
        throw std::runtime_error("Cannot determine reward database namespace");

    SQLWCHAR identity[256]{};
    SQLLEN bytes = 0;
    CheckResult(SQLGetData(statement, 1, SQL_C_WCHAR, identity, sizeof(identity), &bytes), statement);
    FreeStatementHandle(statement);
    if (bytes <= 0 || bytes >= sizeof(identity))
        throw std::runtime_error("Invalid reward database namespace");

    return std::wstring(reinterpret_cast<wchar_t*>(identity));
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

    if (!ExecuteSP(hstmt, STORED_PROCEDURE::GET_VALIDATOR_IDS))
    {
        FreeStatementHandle(hstmt);
        return {};
    }

    std::vector<std::pair<std::string, int>> validatorInfos;
    SQLCHAR validatorID[NAME_SIZE + 1]{};
    SQLINTEGER stakedToken;
    SQLLEN cbValidatorID, cbStakedToken;

    while (FetchRow(hstmt))
    {
        CheckResult(SQLGetData(hstmt, 1, SQL_C_CHAR, validatorID, sizeof(validatorID), &cbValidatorID), hstmt);
        CheckResult(SQLGetData(hstmt, 2, SQL_C_SLONG, &stakedToken, sizeof(stakedToken), &cbStakedToken), hstmt);
        if (cbValidatorID < 0 || cbValidatorID >= NAME_SIZE || cbStakedToken == SQL_NULL_DATA)
            throw std::runtime_error("Invalid validator DB row");

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

    if (!ExecuteSP(hstmt, STORED_PROCEDURE::GET_FULL_NODE_INFO, _id))
    {
        FreeStatementHandle(hstmt);
        return false;
    }

    SQLLEN cbFullNode = 0;
    CheckResult(SQLBindCol(hstmt, 1, SQL_C_BIT, &_fullNode, 0, &cbFullNode), hstmt);

    SQLRETURN ret = FetchRow(hstmt) ? SQL_SUCCESS : SQL_NO_DATA;
    if (!SQL_SUCCEEDED(ret))
    {
        if (ret != SQL_NO_DATA)
            Show_Error(hstmt, SQL_HANDLE_STMT, ret);

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

    if (!ExecuteSP(hstmt, STORED_PROCEDURE::GET_STAKED_TOKEN_DATA, _id))
    {
        FreeStatementHandle(hstmt);
        return false;
    }

    SQLLEN indicator = 0;
    CheckResult(SQLBindCol(hstmt, 1, SQL_C_LONG, &_stakedToken, sizeof(int), &indicator), hstmt);
    CheckResult(SQLBindCol(hstmt, 2, SQL_C_LONG, &_stakedDays, sizeof(int), &indicator), hstmt);
    CheckResult(SQLBindCol(hstmt, 3, SQL_C_LONG, &_dayCount, sizeof(int), &indicator), hstmt);

    SQLRETURN ret = FetchRow(hstmt) ? SQL_SUCCESS : SQL_NO_DATA;
    if (!SQL_SUCCEEDED(ret))
    {
        if (ret != SQL_NO_DATA)
            Show_Error(hstmt, SQL_HANDLE_STMT, ret);

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
    if (ret != SQL_SUCCESS)
    {
        if (ret != SQL_NO_DATA)
            Show_Error(hstmt, SQL_HANDLE_STMT, ret);

        FreeStatementHandle(hstmt);
        return false;
    }

    if (!BindParameter(hstmt, 1, _id))
    {
        Show_Error(hstmt, SQL_HANDLE_STMT, SQL_ERROR);
        FreeStatementHandle(hstmt);
        return false;
    }

    ret = SQLExecute(hstmt);
    if (ret != SQL_SUCCESS)
    {
        if (ret != SQL_NO_DATA)
            Show_Error(hstmt, SQL_HANDLE_STMT, ret);

        FreeStatementHandle(hstmt);
        return false;
    }

    SQLINTEGER count = 0;
    CheckResult(SQLBindCol(hstmt, 1, SQL_C_LONG, &count, sizeof(count), NULL), hstmt);

    ret = FetchRow(hstmt) ? SQL_SUCCESS : SQL_NO_DATA;
    if (ret == SQL_NO_DATA || ret != SQL_SUCCESS)
    {
        if (ret != SQL_NO_DATA)
            Show_Error(hstmt, SQL_HANDLE_STMT, ret);

        FreeStatementHandle(hstmt);
        return false;
    }

    FreeStatementHandle(hstmt);
    return count > 0;
}

bool CDataBase::CheckPassword(const std::string& _id, const std::string& _password)
{
    std::string encoded;

    return VerifyCredential(_id, _password, encoded);
}

bool CDataBase::VerifyCredential(const std::string& _id, const std::string& _password, std::string& _encoded)
{
    SQLHSTMT statement = nullptr;
    if (!AllocStatementHandle(statement))
        return false;

    const wchar_t* sql = L"SELECT Player_Password FROM dbo.user_data WHERE Player_ID=?";
    CheckResult(SQLPrepareW(statement, reinterpret_cast<SQLWCHAR*>(const_cast<wchar_t*>(sql)), SQL_NTS), statement);
    if (!BindParameter(statement, 1, _id))
        throw std::runtime_error("credential parameter binding failed");

    CheckResult(SQLExecute(statement), statement);
    if (!FetchRow(statement))
    {
        FreeStatementHandle(statement);
        ThrowIfFailed();

        return false;
    }

    char encoded[257]{};
    SQLLEN length = 0;
    CheckResult(SQLGetData(statement, 1, SQL_C_CHAR, encoded, sizeof(encoded), &length), statement);
    FreeStatementHandle(statement);
    if (length <= 0 || length >= sizeof(encoded))
        return false;

    _encoded.assign(encoded, static_cast<size_t>(length));

    return PasswordHash::Verify(_password, _encoded);
}

bool CDataBase::SignUp(const std::string& _id, const std::string& _password)
{
    return ExecuteSP(STORED_PROCEDURE::SIGN_UP, _id, PasswordHash::Encode(_password));
}

std::vector<AuctionInfo> CDataBase::GetAuctionInfo(int _pageNum)
{
    SQLHSTMT hstmt;
    if (!AllocStatementHandle(hstmt))
        return {};

    if (!ExecuteSP(hstmt, STORED_PROCEDURE::GET_AUCTION_INFO, _pageNum))
    {
        FreeStatementHandle(hstmt);
        return {};
    }

    std::vector<AuctionInfo> result;
    while (FetchRow(hstmt))
    {
        SQLCHAR playerID[11]{};
        SQLSMALLINT customizeType;
        SQLSMALLINT customizeNum;
        SQLINTEGER buyPrice;
        SQL_TIMESTAMP_STRUCT deadline;

        CheckResult(SQLGetData(hstmt, 1, SQL_C_CHAR, playerID, sizeof(playerID), NULL), hstmt);
        CheckResult(SQLGetData(hstmt, 2, SQL_C_SHORT, &customizeType, sizeof(customizeType), NULL), hstmt);
        CheckResult(SQLGetData(hstmt, 3, SQL_C_SHORT, &customizeNum, sizeof(customizeNum), NULL), hstmt);
        CheckResult(SQLGetData(hstmt, 4, SQL_C_LONG, &buyPrice, sizeof(buyPrice), NULL), hstmt);
        CheckResult(SQLGetData(hstmt, 5, SQL_C_TYPE_TIMESTAMP, &deadline, sizeof(deadline), NULL), hstmt);

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

    if (!ExecuteSP(hstmt, STORED_PROCEDURE::GET_AUCTION_PARTS_NUM))
    {
        FreeStatementHandle(hstmt);
        return -1;
    }

    SQLINTEGER rowCount = 0;
    CheckResult(SQLBindCol(hstmt, 1, SQL_C_LONG, &rowCount, 0, NULL), hstmt);

    SQLRETURN ret = FetchRow(hstmt) ? SQL_SUCCESS : SQL_NO_DATA;
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO)
    {
        if (ret != SQL_NO_DATA)
            Show_Error(hstmt, SQL_HANDLE_STMT, ret);

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

    if (!ExecuteSP(hstmt, STORED_PROCEDURE::GET_PLAYER_CUSTOMIZE_PARTS, _id))
    {
        FreeStatementHandle(hstmt);
        return false;
    }

    short shopType = 0;
    short customizeNum = 0;
    short count = 0;
    SQLLEN indicator;

    while (FetchRow(hstmt))
    {
        CheckResult(SQLGetData(hstmt, 1, SQL_C_SHORT, &shopType, sizeof(shopType), &indicator), hstmt);
        CheckResult(SQLGetData(hstmt, 2, SQL_C_SHORT, &customizeNum, sizeof(customizeNum), &indicator), hstmt);
        CheckResult(SQLGetData(hstmt, 3, SQL_C_SHORT, &count, sizeof(count), &indicator), hstmt);
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

    if (!ExecuteSP(hstmt, STORED_PROCEDURE::GET_PLAYERS_BY_SHOP_TYPE, _shopType))
    {
        FreeStatementHandle(hstmt);
        return {};
    }

    std::vector<std::string> playerIDs;
    SQLCHAR playerID[NAME_SIZE + 1]{};
    SQLLEN cbPlayerID;

    while (FetchRow(hstmt))
    {
        CheckResult(SQLGetData(hstmt, 1, SQL_C_CHAR, playerID, sizeof(playerID), &cbPlayerID), hstmt);
        if (cbPlayerID < 0 || cbPlayerID >= NAME_SIZE)
            throw std::runtime_error("Invalid player DB row");

        playerIDs.emplace_back(std::string(reinterpret_cast<char*>(playerID), cbPlayerID));
    }

    FreeStatementHandle(hstmt);
    return playerIDs;
}

void CDataBase::ExecuteDirect(const std::wstring& _sql)
{
    SQLHSTMT statement = nullptr;
    if (!AllocStatementHandle(statement))
        throw std::runtime_error("DB statement allocation failed");

    auto result = SQLExecDirectW(statement, reinterpret_cast<SQLWCHAR*>(const_cast<wchar_t*>(_sql.c_str())), SQL_NTS);
    while (SQL_SUCCEEDED(result))
        result = SQLMoreResults(statement);

    if (result != SQL_NO_DATA)
        Show_Error(statement, SQL_HANDLE_STMT, result);

    FreeStatementHandle(statement);
    ThrowIfFailed();
}

void CDataBase::ValidateSchema()
{
    std::wstring sql = L"IF OBJECT_ID('dbo.user_data','U') IS NULL OR OBJECT_ID('dbo.auction_data','U') IS NULL THROW 50001, 'Missing DB tables', 1; ";
    for (const auto& entry : m_spMap)
        sql += L"IF OBJECT_ID('dbo." + entry.second + L"','P') IS NULL THROW 50002, 'Missing DB procedure', 1; ";

    sql += L"IF OBJECT_ID('dbo.SettleExpiredAuctions','P') IS NULL OR OBJECT_ID('dbo.ApplyReward','P') IS NULL OR ISNULL(COL_LENGTH('dbo.user_data','BalanceVersion'),0) <> 8 "
           L"OR OBJECT_ID('dbo.reward_applied','U') IS NULL OR ISNULL(COL_LENGTH('dbo.user_data','Player_Password'),0)<118 THROW 50003,'Missing DB consistency migration',1; "
           L"IF OBJECT_ID('dbo.WodLocalSchema','U') IS NOT NULL AND NOT EXISTS(SELECT 1 FROM dbo.WodLocalSchema WHERE Version=3) THROW 50004,'Unsupported DB schema version',1;";

    ExecuteDirect(sql);
}

void CDataBase::Transaction(const std::function<void()>& _work)
{
    if (!SQL_SUCCEEDED(SQLSetConnectAttr(m_hdbc, SQL_ATTR_AUTOCOMMIT, reinterpret_cast<SQLPOINTER>(SQL_AUTOCOMMIT_OFF), 0)))
        throw std::runtime_error("Cannot begin DB transaction");

    try
    {
        _work();
        ThrowIfFailed();
        if (!SQL_SUCCEEDED(SQLEndTran(SQL_HANDLE_DBC, m_hdbc, SQL_COMMIT)))
        {
            m_usable = false;
            throw std::runtime_error("DB commit failed; outcome may be unknown");
        }
    }
    catch (...)
    {
        if (!SQL_SUCCEEDED(SQLEndTran(SQL_HANDLE_DBC, m_hdbc, SQL_ROLLBACK)))
            m_usable = false;

        if (!SQL_SUCCEEDED(SQLSetConnectAttr(m_hdbc, SQL_ATTR_AUTOCOMMIT, reinterpret_cast<SQLPOINTER>(SQL_AUTOCOMMIT_ON), 0)))
            m_usable = false;
        throw;
    }

    if (!SQL_SUCCEEDED(SQLSetConnectAttr(m_hdbc, SQL_ATTR_AUTOCOMMIT, reinterpret_cast<SQLPOINTER>(SQL_AUTOCOMMIT_ON), 0)))
    {
        m_usable = false;
        throw std::runtime_error("Cannot restore DB autocommit");
    }
}

void CDataBase::SaveAppearance(const std::string& _id, const ModelCustomize& _model)
{
    // 토큰은 DB의 최신값을 읽어 보존한다. 종료 snapshot으로 잔액을 되돌리지 않는다.
    Transaction([&] {
        PlayerInfo info{};
        SQLHSTMT statement = nullptr;
        if (!AllocStatementHandle(statement))
            throw std::runtime_error("appearance statement failed");

        const auto prepared = SQLPrepareW(statement, reinterpret_cast<SQLWCHAR*>(const_cast<wchar_t*>(L"SELECT Token FROM dbo.user_data WITH (UPDLOCK) WHERE Player_ID = ?")), SQL_NTS);
        if (!SQL_SUCCEEDED(prepared) || !BindParameter(statement, 1, _id) || !SQL_SUCCEEDED(SQLExecute(statement)))
        {
            FreeStatementHandle(statement);
            throw std::runtime_error("appearance token query failed");
        }

        CheckResult(SQLBindCol(statement, 1, SQL_C_LONG, &info.m_tokenNum, sizeof(info.m_tokenNum), nullptr), statement);
        const bool found = FetchRow(statement);
        FreeStatementHandle(statement);
        if (!found)
            throw std::runtime_error("appearance account missing");

        info.m_model = _model;
        if (!SavePlayerInfo(_id, info))
            throw std::runtime_error("appearance persistence failed");
    });
}

void CDataBase::ResetRequest()
{
    while (!m_statements.empty())
        FreeStatementHandle(m_statements.back());

    m_failed = false;
}

bool CDataBase::FetchRow(SQLHSTMT _statement)
{
    const auto result = SQLFetch(_statement);
    if (!SQL_SUCCEEDED(result) && result != SQL_NO_DATA)
        Show_Error(_statement, SQL_HANDLE_STMT, result);

    return SQL_SUCCEEDED(result);
}

void CDataBase::CheckResult(SQLRETURN _result, SQLHSTMT _statement)
{
    if (!SQL_SUCCEEDED(_result))
    {
        Show_Error(_statement, SQL_HANDLE_STMT, _result);
        throw std::runtime_error("DB result binding or conversion failed");
    }
}

void CDataBase::Show_Error(SQLHANDLE _handle, SQLSMALLINT _type, RETCODE _code)
{
    m_failed = true;
    SQLWCHAR state[6]{};
    SQLINTEGER native = 0;
    SQLWCHAR ignored[2]{};
    SQLSMALLINT length = 0;
    SQLGetDiagRecW(_type, _handle, 1, state, &native, ignored, 2, &length);
    std::string code;
    for (int i = 0; i < 5 && state[i]; ++i)
        code += static_cast<char>(state[i]);

    LogPrinter::PrintMsg("DB error SQLSTATE=" + code + " native=" + std::to_string(native));
}

void CDataBase::ThrowIfFailed()
{
    if (std::exchange(m_failed, false))
        throw std::runtime_error("DB operation failed");
}

bool CDataBase::AllocStatementHandle(SQLHSTMT& _hstmt)
{
    if (!m_usable)
        throw std::runtime_error("DB connection state is uncertain; restart required");

    SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_STMT, m_hdbc, &_hstmt);
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO)
    {
        m_failed = true;
        LogPrinter::PrintMsg("Failed To Alloc Statement Handle");
        return false;
    }
    m_statements.push_back(_hstmt);
    if (!SQL_SUCCEEDED(SQLSetStmtAttr(_hstmt, SQL_ATTR_QUERY_TIMEOUT, reinterpret_cast<SQLPOINTER>(5), 0)))
    {
        FreeStatementHandle(_hstmt);
        throw std::runtime_error("Cannot set DB query timeout");
    }

    return true;
}

void CDataBase::FreeStatementHandle(SQLHSTMT _hstmt)
{
    if (_hstmt != NULL)
    {
        std::erase(m_statements, _hstmt);
        SQLCloseCursor(_hstmt);
        SQLFreeHandle(SQL_HANDLE_STMT, _hstmt);
    }
}
}
