#pragma once
#include <Protocol/protocol.h>
#include <span>
#include <cstring>
#include <cmath>
namespace wod::protocol {
enum class Endpoint { GameClient, LobbyClient, LobbyToGame, GameToLobby };
inline bool ValidateShape(std::span<const char> bytes, Endpoint endpoint) {
    if (bytes.size() < 2 || static_cast<unsigned char>(bytes[0]) != bytes.size()) return false;
    const unsigned char type = static_cast<unsigned char>(bytes[1]);
    switch(endpoint) {
    case Endpoint::GameClient:
        switch(type) {
        case CS_BUY_ITEM: return bytes.size() == sizeof(CS_BUY_ITEM_PACKET);
        case CS_BUY_STAT: return bytes.size() == sizeof(CS_BUY_STAT_PACKET);
        case CS_CHAT: return bytes.size() == sizeof(CS_CHAT_PACKET);
        case CS_DEBUG_GOLD: return bytes.size() == sizeof(CS_DEBUG_GOLD_PACKET);
        case CS_DUMMY_CLIENT: return bytes.size() == sizeof(CS_DUMMY_CLIENT_PACKET);
        case CS_JOB_SELECT: return bytes.size() == sizeof(CS_JOB_SELECT_PACKET);
        case CS_JUMP: return bytes.size() == sizeof(CS_JUMP_PACKET);
        case CS_LOAD_COMPLETE: return bytes.size() == sizeof(CS_LOAD_COMPLETE_PACKET);
        case CS_LOGIN: return bytes.size() == sizeof(CS_LOGIN_PACKET);
        case CS_MINION_PATH: return bytes.size() == sizeof(CS_MINION_PATH_PACKET);
        case CS_MOVE: return bytes.size() == sizeof(CS_MOVE_PACKET);
        case CS_NPC_ATTACK_FINISH: return bytes.size() == sizeof(CS_NPC_ATTACK_FINISH_PACKET);
        case CS_READY: return bytes.size() == sizeof(CS_READY_PACKET);
        case CS_ROTATE: return bytes.size() == sizeof(CS_ROTATE_PACKET);
        case CS_RTT: return bytes.size() == sizeof(CS_RTT_PACKET);
        case CS_SKILL: return bytes.size() == sizeof(CS_SKILL_PACKET);
        case CS_SKILL_FINISH: return bytes.size() == sizeof(CS_SKILL_FINISH_PACKET);
        case CS_SKILL_SELECT: return bytes.size() == sizeof(CS_SKILL_SELECT_PACKET);
        case CS_STAT_SELECT: return bytes.size() == sizeof(CS_STAT_SELECT_PACKET);
        case CS_TELEPORT: return bytes.size() == sizeof(CS_TELEPORT_PACKET);
        case CS_TEST_INGAME: return bytes.size() == sizeof(CS_TEST_INGAME_PACKET);
        case CS_TEST_INGAME2: return bytes.size() == sizeof(CS_TEST_INGAME_PACKET2);
        case CS_TOWER_ACTIVATE: return bytes.size() == sizeof(CS_TOWER_ACTIVATE_PACKET);
        case CS_USE_ITEM: return bytes.size() == sizeof(CS_USE_ITEM_PACKET);
        default: return false;
        }
    case Endpoint::LobbyClient:
        switch(type) {
        case CS_BUY_AUCTION: return bytes.size() == sizeof(CS_BUY_AUCTION_PACKET);
        case CS_CHANGE_CHANNEL: return bytes.size() == sizeof(CS_CHANGE_CHANNEL_PACKET);
        case CS_CHANGE_NODE: return bytes.size() == sizeof(CS_CHANGE_NODE_PACKET);
        case CS_CHAT: return bytes.size() == sizeof(CS_CHAT_PACKET);
        case CS_CREATE_TRANSACTION: return bytes.size() == sizeof(CS_CREATE_TRANSACTION_PACKET);
        case CS_CUSTOMIZE: return bytes.size() == sizeof(CS_CUSTOMIZE_PACKET);
        case CS_DUMMY_CLIENT: return bytes.size() == sizeof(CS_DUMMY_CLIENT_PACKET);
        case CS_GET_AUCTION_INFO: return bytes.size() == sizeof(CS_GET_AUCTION_INFO_PACKET);
        case CS_LOGIN: return bytes.size() == sizeof(CS_LOGIN_PACKET);
        case CS_MATCH: return bytes.size() == sizeof(CS_MATCH_PACKET);
        case CS_MOVE: return bytes.size() == sizeof(CS_MOVE_PACKET);
        case CS_OPEN_AUCTION: return bytes.size() == sizeof(CS_OPEN_AUCTION_PACKET);
        case CS_OPEN_BLOCKCHAIN: return bytes.size() == sizeof(CS_OPEN_BLOCKCHAIN_PACKET);
        case CS_OPEN_CUSTOMIZE: return bytes.size() == sizeof(CS_OPEN_CUSTOMIZE_PACKET);
        case CS_PORT_NUM: return bytes.size() == sizeof(CS_PORT_NUM_PACKET);
        case CS_REGISTER_AUCTION: return bytes.size() == sizeof(CS_REGISTER_AUCTION_PACKET);
        case CS_ROTATE: return bytes.size() == sizeof(CS_ROTATE_PACKET);
        case CS_RTT: return bytes.size() == sizeof(CS_RTT_PACKET);
        case CS_SHOP: return bytes.size() == sizeof(CS_SHOP_PACKET);
        case CS_SIGN_UP: return bytes.size() == sizeof(CS_SIGN_UP_PACKET);
        case CS_STAKE_TOKEN: return bytes.size() == sizeof(CS_STAKE_TOKEN_PACKET);
        case CS_TEST_CHANGE_SERVER: return bytes.size() == sizeof(CS_TEST_CHANGE_SERVER_PACKET);
        default: return false;
        }
    case Endpoint::LobbyToGame:
        switch(type) {
        case LG_MATCH_START: return bytes.size() == sizeof(LG_MATCH_START_PACKET);
        case LG_MATCH_PLAYER: return bytes.size() == sizeof(LG_MATCH_PACKET);
        default: return false;
        }
    case Endpoint::GameToLobby:
        switch(type) {
        case GL_TRANSACTIONS: return bytes.size() == sizeof(GL_TRANSACTIONS_PACKET);
        default: return false;
        }
    }
    return false;
}

template<class T> T Read(std::span<const char> bytes) { T packet{}; std::memcpy(&packet,bytes.data(),sizeof(T)); return packet; }
template<size_t N> bool Terminated(const char (&text)[N]) { return std::memchr(text,0,N) != nullptr; }
inline bool Validate(std::span<const char> bytes, Endpoint endpoint) {
    if (!ValidateShape(bytes,endpoint)) return false;
    const auto type=static_cast<unsigned char>(bytes[1]);
    if (endpoint==Endpoint::LobbyToGame) {
        if (type==LG_MATCH_PLAYER) {
            auto p=Read<LG_MATCH_PACKET>(bytes);
            return Terminated(p.name) && p.match_num>=0 && p.id>=0 && p.id<4;
        }
        auto p=Read<LG_MATCH_START_PACKET>(bytes);
        return p.match_num>=0;
    }
    if (endpoint==Endpoint::GameToLobby) return Terminated(Read<GL_TRANSACTIONS_PACKET>(bytes).name);
    switch(type) {
    case CS_LOGIN: { auto p=Read<CS_LOGIN_PACKET>(bytes); return Terminated(p.name) && Terminated(p.password); }
    case CS_SIGN_UP: { auto p=Read<CS_SIGN_UP_PACKET>(bytes); return Terminated(p.name) && Terminated(p.password); }
    case CS_JOB_SELECT: { auto p=Read<CS_JOB_SELECT_PACKET>(bytes); return p.id>=0 && p.id<4 && p.job>=0 && p.job<6; }
    case CS_READY: { auto p=Read<CS_READY_PACKET>(bytes); return p.id>=0 && p.id<4; }
    case CS_LOAD_COMPLETE: { auto p=Read<CS_LOAD_COMPLETE_PACKET>(bytes); return p.id>=0 && p.id<4; }
    case CS_SKILL_SELECT: { auto p=Read<CS_SKILL_SELECT_PACKET>(bytes); return p.id>=0 && p.id<4 && p.storage>=0 && p.storage<MAX_SKILL && p.skill>=0; }
    case CS_SKILL: { auto p=Read<CS_SKILL_PACKET>(bytes); return p.id>=0 && p.id<4 && p.skillType>=1 && p.skillType<=MAX_SKILL+1; }
    case CS_USE_ITEM: { auto p=Read<CS_USE_ITEM_PACKET>(bytes); return p.itemNum>=0 && p.itemNum<ITEM_NUM; }
    case CS_BUY_ITEM: { auto p=Read<CS_BUY_ITEM_PACKET>(bytes); return p.itemType>=0 && p.itemType<static_cast<int>(ITEMKIND::NONE); }
    case CS_BUY_STAT: { auto p=Read<CS_BUY_STAT_PACKET>(bytes); return p.statType>=static_cast<int>(ITEMKIND::HP) && p.statType<=static_cast<int>(ITEMKIND::CRITICAL); }
    case CS_TOWER_ACTIVATE: { auto p=Read<CS_TOWER_ACTIVATE_PACKET>(bytes); return p.num>=0 && p.num<PATH_NUM; }
    case CS_MINION_PATH: { auto p=Read<CS_MINION_PATH_PACKET>(bytes); return p.path>=0 && p.path<PATH_NUM; }
    case CS_NPC_ATTACK_FINISH: { auto p=Read<CS_NPC_ATTACK_FINISH_PACKET>(bytes); return p.id>=0 && p.id<MAX_MINION+MONSTER_NUM; }
    case CS_ROTATE: { auto p=Read<CS_ROTATE_PACKET>(bytes); return std::isfinite(p.lookX) && std::isfinite(p.lookY) && std::isfinite(p.lookZ) && std::isfinite(p.rightX) && std::isfinite(p.rightY) && std::isfinite(p.rightZ); }
    default: return true;
    }
}
}
