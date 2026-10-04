#include "stdafx.h"
#include "NetworkManager.h"
#include "OverlapEx.h"
#include "SocketUtil.h"
#include "Scene.h"
#include "Player.h"
#include "Util.h"
#include "PeerManager.h"
#include "SoundManager.h"
#include "SceneManager.h"

std::unique_ptr<NetworkManager> NetworkManager::m_instance;

//Set Animation Speed by Walk Speed Function

float AniSpeed(JOB herojob, int AniNum)
{
    switch (herojob)
    {
    case JOB::ARCHER:
        switch (AniNum)
        {
        case 1://Forward
            return 1.5f;
        case 2://Back
            return 1.5f;
        case 3://Left
            return 1.5f;
        case 4://Right
            return 1.5f;
        default:
            cout << "Other Ani Number have been entered" << endl;
            return 0.f;
        }
        return 0.f;
    case JOB::FIGHTER:
        switch (AniNum)
        {
        case 1:
            return 1.f;
        case 2:
            return 1.f;
        case 3:
            return 1.f;
        case 4:
            return 1.f;
        default:
            cout << "Other Ani Number have been entered" << endl;
            return 0.f;
        }
        return 0.f;
    case JOB::SWORDMAN:
        switch (AniNum)
        {
        case 1:
            return 1.5f;
        case 2:
            return 2.f;
        case 3:
            return 2.f;
        case 4:
            return 1.5f;
        default:
            cout << "Other Ani Number have been entered" << endl;
            return 0.f;
        }
        return 0.f;
    case JOB::WIZARD:
        switch (AniNum)
        {
        case 1:
            return 1.5f;
        case 2:
            return 2.f;
        case 3:
            return 1.8f;
        case 4:
            return 2.0f;
        default:
            cout << "Other Ani Number have been entered" << endl;
            return 0.f;
        }
        return 0.f;
    case JOB::NONE:
        cout << "Error because job number is none" << endl;
        return 0.f;
    default:
        cout << "Error because job number is default" << endl;
        return 0.f;
    }
}

float AniSpeed(BOSSJOB bossjob, int AniNum)
{
    switch (bossjob)
    {
    case BOSSJOB::OGRE:
        switch (AniNum)
        {
        case 1:
            return 2.5f;
        case 2:
            return 2.5f;
        case 3:
            return 1.5f;
        case 4:
            return 2.0f;
        default:
            cout << "Other Ani Number have been entered" << endl;
            return 0.f;
        }
        return 0.f;
    case BOSSJOB::PROGRAMMER:
        cout << "Error because BossJob is Programmer" << endl;
        return 0.f;
    case BOSSJOB::NONE:
        cout << "Error because job number is none" << endl;
        return 0.f;
    default:
        cout << "Error because job number is default" << endl;
        return 0.f;
    }
}

void NetworkManager::Initialize(string ip)
{
    myInfo = new ObjectInfo;
    otherClientsInfo = new ObjectInfo[LOBBY_MAX_CLIENT];
    npcInfo = new NpcInfo[MAX_MINION];    
    monsterInfo = new NpcInfo[MONSTER_NUM];
    readySceneInfo = new ReadySceneInfo;
    gameSceneInfo = new GameSceneInfo;
    lobbySceneInfo = new LobbySceneInfo;
    structureInfo = new StructureInfo[5];
    structureInfo[4].active = true;

    for (int i = 0; i < 7; ++i) {
        lobbySceneInfo->auctionPageInfos.push_back({});
    }

    myInfo->Initialize();
    myInfo->show = true;
    for (int i = 0; i < LOBBY_MAX_CLIENT; ++i) {
        otherClientsInfo[i].Initialize();
#ifdef WITH_DATABASE
        m_ArrayOtherClientCustom[i] = ModelCustomize{ 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        0 , -1, 1 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 };

        m_ArrayInGameClientsCustom[i] = ModelCustomize{ 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
0 , -1, 1 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 };
#else
        m_ArrayOtherClientCustom[i] = ModelCustomize{ 0, 1, 0, 0, 1, 1, 3, 1, 0, 1, 0, 0, 1, 1, 1,
        1 , 0, 1 , 3 , 2 , 2 , 2 , 2 , 1 , 1 , 1 , 1 , 1 };
#endif
    }
    for (int i = 0; i < MAX_MINION; ++i) {
        npcInfo[i].Initialize();
    }
    for (int i = 0; i < MONSTER_NUM; ++i) {
        monsterInfo[i].Initialize();
    }
    readySceneInfo->Initialize();
    gameSceneInfo->Initialize();

    //Skill ObjectInfo Initialize
    for (int i = 0; i < MAX_SKILL_OBJECT; ++i) {
        skillObjectInfos[SKILL_TYPE::WIZARD_ATTACK].emplace_back();
        skillObjectInfos[SKILL_TYPE::WIZARD_MAGIC_MISSILE].emplace_back();
        skillObjectInfos[SKILL_TYPE::WIZARD_ENERGY_BALL].emplace_back();
        skillObjectInfos[SKILL_TYPE::WIZARD_BIGBANG].emplace_back();
        skillObjectInfos[SKILL_TYPE::WIZARD_DARKNESS_RAY].emplace_back();
        skillObjectInfos[SKILL_TYPE::WIZARD_BIGBANG_CONTINUE].emplace_back();
        skillObjectInfos[SKILL_TYPE::SWORDMAN_AURA_BLADE].emplace_back();
        skillObjectInfos[SKILL_TYPE::SWORDMAN_JUDGEMENT_SWORD].emplace_back();
        skillObjectInfos[SKILL_TYPE::SWORDMAN_PROTECTED_AREA].emplace_back();
        skillObjectInfos[SKILL_TYPE::ARCHER_ATTACK].emplace_back();
        skillObjectInfos[SKILL_TYPE::ARCHER_PHOENIX_ARROW].emplace_back();
        skillObjectInfos[SKILL_TYPE::ARCHER_PENETRAITING_SHOT].emplace_back();
        skillObjectInfos[SKILL_TYPE::ARCHER_STICKY_ARROW].emplace_back();
        skillObjectInfos[SKILL_TYPE::ARCHER_STROM_ARROW].emplace_back();
        skillObjectInfos[SKILL_TYPE::ARCHER_ARROW_RAIN].emplace_back();
        skillObjectInfos[SKILL_TYPE::FIGHTER_FIREBALL].emplace_back();
        skillObjectInfos[SKILL_TYPE::PRO_RETURN_ZERO].emplace_back();
        skillObjectInfos[SKILL_TYPE::PRO_SCL].emplace_back();
        skillObjectInfos[SKILL_TYPE::PRO_ATTACK].emplace_back();
        skillObjectInfos[SKILL_TYPE::PRO_HELLO_WORLD].emplace_back();
        skillObjectInfos[SKILL_TYPE::OGRE_ROCK_THROW].emplace_back();
        skillObjectInfos[SKILL_TYPE::OGRE_DIMENSION_CRUSH].emplace_back();
        skillObjectInfos[SKILL_TYPE::OGRE_DIMENSION_PUNCH].emplace_back();
    }

    for (int i = 0; i < MAX_SKILL_OBJECT * 5; ++i) {
        skillObjectInfos[SKILL_TYPE::ARCHER_MULTIPLE_SHOT].emplace_back();
    }


    SocketUtil::Startup();
    m_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
    if (INVALID_SOCKET == m_socket) {
        SocketUtil::PrintError("Socket Initialize");
    }
    GUID op = WSAID_DISCONNECTEX;
    DWORD bytes = 0;
    WSAIoctl(m_socket, SIO_GET_EXTENSION_FUNCTION_POINTER, &op, sizeof(op), &SocketUtil::DisconnectEx, sizeof(SocketUtil::DisconnectEx), &bytes, NULL, NULL);

    GUID op2 = WSAID_CONNECTEX;
    WSAIoctl(m_socket, SIO_GET_EXTENSION_FUNCTION_POINTER, &op2, sizeof(op2), &SocketUtil::ConnectEx, sizeof(SocketUtil::ConnectEx), &bytes, NULL, NULL);

    m_iocpfunc.insert({ OP_TYPE::OP_RECV, [this](int id, int bytes, OverlapEx* over_ex) {NetworkManager::Recv(id, bytes, over_ex); } });
    m_iocpfunc.insert({ OP_TYPE::OP_SEND, [this](int id, int bytes, OverlapEx* over_ex) {NetworkManager::Send(id, bytes, over_ex); } });
    m_iocpfunc.insert({ OP_TYPE::OP_DISCONNECT,[this](int id, int bytes, OverlapEx* over_ex) {NetworkManager::Disconnect(id, bytes, over_ex); } });
    m_iocpfunc.insert({ OP_TYPE::OP_CONNECT,[this](int id, int bytes, OverlapEx* over_ex) {NetworkManager::Connect(id, bytes, over_ex); } });

    m_packetfunc.insert({ SC_LOGIN_INFO, [this](BASE_PACKET* packet, int id) {NetworkManager::LoginInfoPacket(id, packet); } });
    m_packetfunc.insert({ SC_LOGIN_FAIL, [this](BASE_PACKET* packet, int id) {NetworkManager::LoginFailPacket(id, packet); } });
    m_packetfunc.insert({ SC_ADD_PLAYER, [this](BASE_PACKET* packet, int id) {NetworkManager::AddPlayerPacket(id, packet); } });
    m_packetfunc.insert({ SC_REMOVE_PLAYER, [this](BASE_PACKET* packet, int id) {NetworkManager::RemovePlayerPacket(id, packet); } });
    m_packetfunc.insert({ SC_MOVE_PLAYER, [this](BASE_PACKET* packet, int id) {NetworkManager::MovePacket(id, packet); } });
    m_packetfunc.insert({ SC_MATCH_PLAYER, [this](BASE_PACKET* packet, int id) {NetworkManager::MatchPacket(id, packet); } });
    m_packetfunc.insert({ SC_MATCH_END, [this](BASE_PACKET* packet, int id) {NetworkManager::MatchEndPacket(id, packet); } });
    m_packetfunc.insert({ SC_CHAT, [this](BASE_PACKET* packet, int id) {NetworkManager::ChatPacket(id, packet); } });
    m_packetfunc.insert({ SC_ROTATE_PLAYER, [this](BASE_PACKET* packet, int id) {NetworkManager::RotatePacket(id, packet); } });
    m_packetfunc.insert({ SC_SKILL, [this](BASE_PACKET* packet, int id) {NetworkManager::SkillPacket(id, packet); } });
    m_packetfunc.insert({ SC_COOLTIME, [this](BASE_PACKET* packet, int id) {NetworkManager::CoolTimePacket(id, packet); } });
    m_packetfunc.insert({ SC_ADD_NPC, [this](BASE_PACKET* packet, int id) {NetworkManager::AddNpcPacket(id, packet); } });
    m_packetfunc.insert({ SC_SKILL_SELECT, [this](BASE_PACKET* packet, int id) {NetworkManager::SkillSelectPacket(id, packet); } });
    m_packetfunc.insert({ SC_GAME_TIME, [this](BASE_PACKET* packet, int id) {NetworkManager::GameTimePacket(id, packet); } });
    m_packetfunc.insert({ SC_READY, [this](BASE_PACKET* packet, int id) {NetworkManager::ReadyPacket(id, packet); } });
    m_packetfunc.insert({ SC_JOB_SELECT, [this](BASE_PACKET* packet, int id) {NetworkManager::JobSelectPacket(id, packet); } });
    m_packetfunc.insert({ SC_GAME_START, [this](BASE_PACKET* packet, int id) {NetworkManager::GameStartPacket(id, packet); } });
    m_packetfunc.insert({ SC_MODEL_CUSTOMIZE, [this](BASE_PACKET* packet, int id) {NetworkManager::CustomizePacket(id, packet); } });
    m_packetfunc.insert({ SC_TEST_CHANGE_SERVER, [this](BASE_PACKET* packet, int id) {NetworkManager::ChangeServerPacket(id, packet); } });
    m_packetfunc.insert({ SC_MOVE_NPC, [this](BASE_PACKET* packet, int id) {NetworkManager::MoveNpcPacket(id, packet); } });
    m_packetfunc.insert({ SC_TELEPORT, [this](BASE_PACKET* packet, int id) {NetworkManager::TeleportPacket(id, packet); } });
    m_packetfunc.insert({ SC_TOWER_ATTACK, [this](BASE_PACKET* packet, int id) {NetworkManager::TowerAttackPacket(id, packet); } });
    m_packetfunc.insert({ SC_TOWER_ATTACK_REMOVE, [this](BASE_PACKET* packet, int id) {NetworkManager::TowerAttackRemovePacket(id, packet); } });
    m_packetfunc.insert({ SC_TOWER_ATTACK_ADD, [this](BASE_PACKET* packet, int id) {NetworkManager::TowerAttackAddPacket(id, packet); } });
    m_packetfunc.insert({ SC_REMOVE_NPC, [this](BASE_PACKET* packet, int id) {NetworkManager::RemoveNpcPacket(id, packet); } });
    m_packetfunc.insert({ SC_NPC_STAT_CHANGE, [this](BASE_PACKET* packet, int id) {NetworkManager::NpcStatChangePacket(id, packet); } });
    m_packetfunc.insert({ SC_ADD_SKILL_OBJECT, [this](BASE_PACKET* packet, int id) {NetworkManager::AddSkillObjectPacket(id, packet); } });
    m_packetfunc.insert({ SC_UPDATE_SKILL_OBJECT, [this](BASE_PACKET* packet, int id) {NetworkManager::UpdateSkillObjectPacket(id, packet); } });
    m_packetfunc.insert({ SC_REMOVE_SKILL_OBJECT, [this](BASE_PACKET* packet, int id) {NetworkManager::RemoveSkillObjectPacket(id, packet); } });
    m_packetfunc.insert({ SC_NPC_ATTACK, [this](BASE_PACKET* packet, int id) {NetworkManager::NpcAttackPacket(id, packet); } });
    m_packetfunc.insert({ SC_GIVE_GOLD, [this](BASE_PACKET* packet, int id) {NetworkManager::GiveGoldPacket(id, packet); } });
    m_packetfunc.insert({ SC_PLAYER_STAT_CHANGE, [this](BASE_PACKET* packet, int id) {NetworkManager::PlayerStatChangePacket(id, packet); } });
    m_packetfunc.insert({ SC_PLAYER_STATUS_CHANGE, [this](BASE_PACKET* packet, int id) {NetworkManager::PlayerStatusChangePacket(id, packet); } });
    m_packetfunc.insert({ SC_SHOP, [this](BASE_PACKET* packet, int id) {NetworkManager::ShopPacket(id, packet); } });
    m_packetfunc.insert({ SC_AUCTION_INFO, [this](BASE_PACKET* packet, int id) {NetworkManager::AuctionInfoPacket(id, packet); } });
    m_packetfunc.insert({ SC_HEALTH_MANA, [this](BASE_PACKET* packet, int id) {NetworkManager::HealthManaPacket(id, packet); } });
    m_packetfunc.insert({ SC_SKILL_FINISH, [this](BASE_PACKET* packet, int id) {NetworkManager::SkillFinishPacket(id, packet); } });
    m_packetfunc.insert({ SC_PLAYER_RESPWAN, [this](BASE_PACKET* packet, int id) {NetworkManager::PlayerRespawnPacket(id, packet); } });
    m_packetfunc.insert({ SC_STRUCTURE_STAT_CHANGE, [this](BASE_PACKET* packet, int id) {NetworkManager::StructureStatChangePacket(id, packet); } });
    m_packetfunc.insert({ SC_STRUCTURE_STATUS_CHANGE, [this](BASE_PACKET* packet, int id) {NetworkManager::StructureStatusChangePacket(id, packet); } });
    m_packetfunc.insert({ SC_CHANGE_CHANNEL, [this](BASE_PACKET* packet, int id) {NetworkManager::ChangeChannelPacket(id, packet); } });
    m_packetfunc.insert({ SC_TOKEN_NUM, [this](BASE_PACKET* packet, int id) {NetworkManager::TokenNumPacket(id, packet); } });
    m_packetfunc.insert({ SC_CHANGE_NODE, [this](BASE_PACKET* packet, int id) {NetworkManager::ChangeNodePacket(id, packet); } });
    m_packetfunc.insert({ SC_STAKE_TOKEN, [this](BASE_PACKET* packet, int id) {NetworkManager::StakeTokenPacket(id, packet); } });
    m_packetfunc.insert({ SC_TRANSACTION, [this](BASE_PACKET* packet, int id) {NetworkManager::StakeTokenPacket(id, packet); } });
    m_packetfunc.insert({ SC_MAGIC_EYE_POS, [this](BASE_PACKET* packet, int id) {NetworkManager::MagicEyePacket(id, packet); } });
    m_packetfunc.insert({ SC_BLOCK_HEADER, [this](BASE_PACKET* packet, int id) {NetworkManager::BlockHeaderPacket(id, packet); } });
    m_packetfunc.insert({ SC_BLOCK_BODY, [this](BASE_PACKET* packet, int id) {NetworkManager::BlockBodyPacket(id, packet); } });
    m_packetfunc.insert({ SC_FULLNODE, [this](BASE_PACKET* packet, int id) {NetworkManager::FullNodePacket(id, packet); } });
    m_packetfunc.insert({ SC_PEER_INFO, [this](BASE_PACKET* packet, int id) {NetworkManager::PeerInfoPacket(id, packet); } });  
    m_packetfunc.insert({ SC_TELEPORT_ACTIVE, [this](BASE_PACKET* packet, int id) {NetworkManager::TeleportActivePacket(id, packet); } });
    m_packetfunc.insert({ SC_GAME_OVER, [this](BASE_PACKET* packet, int id) {NetworkManager::GameOverPacket(id, packet); } });
    m_packetfunc.insert({ SC_MONSTER_KILL_BUFF, [this](BASE_PACKET* packet, int id) {NetworkManager::MonsterKillBuffPacket(id, packet); } });
    m_packetfunc.insert({ SC_JUMP_FINISH, [this](BASE_PACKET* packet, int id) {NetworkManager::JumpFinishPacket(id, packet); } });
    m_packetfunc.insert({ SC_CUSTOMIZE_PARTS, [this](BASE_PACKET* packet, int id) {NetworkManager::CustomizePartsPacket(id, packet); } });
    m_packetfunc.insert({ SC_AUCTION_PARTS_NUM, [this](BASE_PACKET* packet, int id) {NetworkManager::AuctionPartsNumPacket(id, packet); } });    
    m_packetfunc.insert({ SC_STAKE_TOKEN_INFO, [this](BASE_PACKET* packet, int id) {NetworkManager::StakedTokenInfoPacket(id, packet); } });
    m_packetfunc.insert({ SC_RTT, [this](BASE_PACKET* packet, int id) {NetworkManager::RTTPacket(id, packet); } });

    m_over = new OverlapEx();

    SOCKADDR_IN server_addr;
    ZeroMemory(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(LOBBY_PORT);
    inet_pton(AF_INET, ip.c_str(), &server_addr.sin_addr);

    SOCKADDR_IN cl;
    memset(&cl, 0, sizeof(cl));
    cl.sin_family = AF_INET;
    cl.sin_port = 0;
    cl.sin_addr.S_un.S_addr = INADDR_ANY;

    ::bind(m_socket, reinterpret_cast<LPSOCKADDR>(&cl), sizeof(cl));

    int connect = WSAConnect(m_socket, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr), NULL, NULL, NULL, NULL);
    if (connect != 0) {
        cout << "Server Connect Fail" << endl;
    }
    else {
        cout << "Server Connected" << endl;
    }
    m_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, NULL, 0);
    CreateIoCompletionPort(reinterpret_cast<HANDLE>(m_socket), m_iocp, 0, 0);
    
    playerScene = SCENEKIND::TITLE;

    m_portNum = Util::GenerateRandomInt(49152, 65535);

    PeerManager::GetInstance()->Initialize(m_portNum);
}

void NetworkManager::Release()
{
    delete m_over;
    delete myInfo;
    
    if (npcInfo) {
        delete[] npcInfo;
        npcInfo = nullptr;
    }
    delete[] monsterInfo;
    delete gameSceneInfo;
    if (readySceneInfo) {
        delete readySceneInfo;
        readySceneInfo = nullptr;
    }
    delete lobbySceneInfo;
    delete[] structureInfo;
    //delete[] otherClientsInfo;

    PeerManager::GetInstance()->Release();
}

void NetworkManager::Disconnect()
{
    closesocket(m_socket);
    SocketUtil::Cleanup();
    Release();
}

void NetworkManager::Reset()
{
    readySceneInfo->Initialize();
    channelNum = 0;
    gameSceneInfo->Initialize();
    for (int i = 0; i < MAX_MINION; ++i) {
        npcInfo[i].Initialize();
    }
    for (int i = 0; i < MONSTER_NUM; ++i) {
        monsterInfo[i].Initialize();
    }
    skillUsed = false;

    for (int i = 0; i < skillCoolTime.size(); ++i) {
        skillCoolTime[i] = 0;
    }
    playerSkill = SKILLKIND::NONE;

    for (int i = 0; i < PATH_NUM; ++i) {
        structureInfo[i].active = false;
        structureInfo[i].broken = false;
    }
    structureInfo[3].active = true;
    structureInfo[4].active = true;
    myInfo->Initialize();
    myInfo->show = true;

    for (int i = 0; i < LOBBY_MAX_CLIENT; ++i) {
        otherClientsInfo[i].Initialize();
#ifdef WITH_DATABASE
        m_ArrayOtherClientCustom[i] = ModelCustomize{ 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        0 , -1, 1 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 };

        m_ArrayInGameClientsCustom[i] = ModelCustomize{ 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
0 , -1, 1 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 };
#endif
    }

    for (int i = 0; i < MAX_SKILL_OBJECT; ++i) {
        skillObjectInfos[SKILL_TYPE::WIZARD_ATTACK][i].show = false;
        skillObjectInfos[SKILL_TYPE::WIZARD_MAGIC_MISSILE][i].show = false;
        skillObjectInfos[SKILL_TYPE::WIZARD_ENERGY_BALL][i].show = false;
        skillObjectInfos[SKILL_TYPE::WIZARD_BIGBANG][i].show = false;
        skillObjectInfos[SKILL_TYPE::WIZARD_DARKNESS_RAY][i].show = false;
        skillObjectInfos[SKILL_TYPE::WIZARD_BIGBANG_CONTINUE][i].show = false;
        skillObjectInfos[SKILL_TYPE::SWORDMAN_AURA_BLADE][i].show = false;
        skillObjectInfos[SKILL_TYPE::SWORDMAN_JUDGEMENT_SWORD][i].show = false;
        skillObjectInfos[SKILL_TYPE::SWORDMAN_PROTECTED_AREA][i].show = false;
        skillObjectInfos[SKILL_TYPE::ARCHER_ATTACK][i].show = false;
        skillObjectInfos[SKILL_TYPE::ARCHER_PHOENIX_ARROW][i].show = false;
        skillObjectInfos[SKILL_TYPE::ARCHER_PENETRAITING_SHOT][i].show = false;
        skillObjectInfos[SKILL_TYPE::ARCHER_STICKY_ARROW][i].show = false;
        skillObjectInfos[SKILL_TYPE::ARCHER_STROM_ARROW][i].show = false;
        skillObjectInfos[SKILL_TYPE::ARCHER_ARROW_RAIN][i].show = false;
        skillObjectInfos[SKILL_TYPE::FIGHTER_FIREBALL][i].show = false;
        skillObjectInfos[SKILL_TYPE::PRO_RETURN_ZERO][i].show = false;
        skillObjectInfos[SKILL_TYPE::PRO_SCL][i].show = false;
        skillObjectInfos[SKILL_TYPE::PRO_ATTACK][i].show = false;
        skillObjectInfos[SKILL_TYPE::PRO_HELLO_WORLD][i].show = false;
        skillObjectInfos[SKILL_TYPE::OGRE_ROCK_THROW][i].show = false;
        skillObjectInfos[SKILL_TYPE::OGRE_DIMENSION_CRUSH][i].show = false;
        skillObjectInfos[SKILL_TYPE::OGRE_DIMENSION_PUNCH][i].show = false;
    }

    for (int i = 0; i < MAX_SKILL_OBJECT * 5; ++i) {
        skillObjectInfos[SKILL_TYPE::ARCHER_MULTIPLE_SHOT][i].show = false;
    }

}

void NetworkManager::SendPacket(BASE_PACKET* packet) const
{
    OverlapEx* over = new OverlapEx(packet);
    WSASend(m_socket, &over->GetWSA(), 1, 0, 0, &over->GetOver(), 0);
    delete packet;
}

void NetworkManager::RecvPacket()
{
    DWORD recv_flag = 0;
    memset(&m_over->GetOver(), 0, sizeof(m_over->GetOver()));
    m_over->GetWSA().len = BUF_SIZE - m_remainData;
    m_over->GetWSA().buf = m_over->GetSendBuf() + m_remainData;
    int byte = WSARecv(m_socket, &m_over->GetWSA(), 1, 0, &recv_flag, &m_over->GetOver(), 0);
}

void NetworkManager::WorkerThread()
{
    RecvPacket();
    while (true) {
        DWORD bytes;
        ULONG_PTR key;
        WSAOVERLAPPED* over = nullptr;
        int err = GetQueuedCompletionStatus(m_iocp, &bytes, &key, &over, INFINITE);
        OverlapEx* over_ex = reinterpret_cast<OverlapEx*>(over);
        int id = static_cast<int>(key);

        if (false == err) {
            cout << "GQCS err" << endl;
            if (OP_TYPE::OP_SEND == over_ex->GetOP())
                delete over_ex;
            continue;
        }

        auto iter = m_iocpfunc.find(over_ex->GetOP());
        if (iter != m_iocpfunc.end()) {
            iter->second(static_cast<int>(key), bytes, over_ex);
        }
        else
            cout << "Wrong Key Value For IOCP Function" << endl;
    }
}

void NetworkManager::TestReady(bool type)
{
    for (int i = 0; i < INGAME_PLAYER; ++i) {
        otherClientsInfo[i].Initialize();
    }

    for (int i = 0; i < INGAME_PLAYER; ++i) {
        otherClientsInfo[i].show = true;
    }

    if (type) {
        //Client Test
        m_id = 0;
    }
    else {
        //Boss Test
        m_id = 3;
    }
    for (int i = 0; i < INGAME_PLAYER; ++i) {
        if (3 == m_id)
        {
            myInfo->x = READY_SCENE_PLAYER_POS_BY_BOSS[m_id].x;
            myInfo->y = READY_SCENE_PLAYER_POS_BY_BOSS[m_id].y;
            myInfo->z = READY_SCENE_PLAYER_POS_BY_BOSS[m_id].z;
            //if (3 != i)
            {
                otherClientsInfo[i].x = READY_SCENE_PLAYER_POS_BY_BOSS[i].x;
                otherClientsInfo[i].y = READY_SCENE_PLAYER_POS_BY_BOSS[i].y;
                otherClientsInfo[i].z = READY_SCENE_PLAYER_POS_BY_BOSS[i].z;
            }
        }
        else if (i == m_id) {
            otherClientsInfo[i].show = false;
            myInfo->x = READY_SCENE_PLAYER_POS[m_id].x;
            myInfo->y = READY_SCENE_PLAYER_POS[m_id].y;
            myInfo->z = READY_SCENE_PLAYER_POS[m_id].z;
        }
        else {
            otherClientsInfo[i].show = true;
            otherClientsInfo[i].x = READY_SCENE_PLAYER_POS[i].x;
            otherClientsInfo[i].y = READY_SCENE_PLAYER_POS[i].y;
            otherClientsInfo[i].z = READY_SCENE_PLAYER_POS[i].z;
        }
        //m_ArrayInGameClientsCustom[i] = { 0, 1, 0, 0, 1, 1, 3, 1, 0, 1, 0, 0, 1, 1, 1, 1, 0, 1, 3, 2, 2, 2, 2, 1, 1, 1, 1, 1 };
    }

    playerScene = SCENEKIND::READY;
}

void NetworkManager::SendJumpPacket() const
{
    CS_JUMP_PACKET* p = new CS_JUMP_PACKET;
    p->size = sizeof(CS_JUMP_PACKET);
    p->type = CS_JUMP;
    SendPacket(p);
}

void NetworkManager::SendTeleportPacket() const
{
    CS_TELEPORT_PACKET* p = new CS_TELEPORT_PACKET;
    p->size = sizeof(CS_TELEPORT_PACKET);
    p->type = CS_TELEPORT;
    SendPacket(p);
}

const void NetworkManager::SendTestIngamePacket(char name[NAME_SIZE]) const
{
    CS_TEST_INGAME_PACKET* p = new CS_TEST_INGAME_PACKET;
    p->size = sizeof(CS_TEST_INGAME_PACKET);
    p->type = CS_TEST_INGAME;
    memcpy_s(p->name, NAME_SIZE, name, NAME_SIZE);    
    SendPacket(p);
}

const void NetworkManager::SendTestIngamePacket() const
{
    CS_TEST_INGAME_PACKET2* p = new CS_TEST_INGAME_PACKET2;
    p->size = sizeof(CS_TEST_INGAME_PACKET2);
    p->type = CS_TEST_INGAME2;    
    SendPacket(p);
}

void NetworkManager::Recv(int id, int bytes, OverlapEx* over_ex)
{
    int remainData = bytes + m_remainData;
    char* packet = over_ex->GetSendBuf();

    while (remainData > 0) {
        BASE_PACKET* p = reinterpret_cast<BASE_PACKET*>(packet);

        if (p->size <= remainData) {
            m_packetfunc[p->type](p, id);
            packet += p->size;
            remainData -= p->size;
        }
        else 
            break;
    }

    m_remainData = remainData;
    if (remainData > 0)
        memmove(over_ex->GetSendBuf(), packet, remainData);
    
    RecvPacket();
}

void NetworkManager::Send(int id, int bytes, OverlapEx* over_ex)
{
    delete over_ex;
}

void NetworkManager::Disconnect(int id, int bytes, OverlapEx* over_ex)
{
    SOCKADDR_IN server_addr;
    ZeroMemory(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(over_ex->GetInfo1());
    inet_pton(AF_INET, over_ex->GetInfo2(), &server_addr.sin_addr);

    cout << "Port: " << over_ex->GetInfo1() << endl;
    cout << "IP: " << over_ex->GetInfo2() << endl;

    OverlapEx* over = new OverlapEx();
    over->ResetOver();
    over->SetOP(OP_TYPE::OP_CONNECT);
    if (!SocketUtil::ConnectEx(m_socket, reinterpret_cast<LPSOCKADDR>(&server_addr), sizeof(server_addr), NULL, 0, NULL, &over->GetOver()) && WSA_IO_PENDING != WSAGetLastError() && ERROR_IO_PENDING != WSAGetLastError()) {
        SocketUtil::PrintError("Connect Error");
    }
    cout << "Disconnect" << endl;
    delete over_ex;
}

void NetworkManager::Connect(int id, int bytes, OverlapEx* over_ex)
{
    //Connect to Game Server
    cout << "Connect" << endl;
    RecvPacket();
    delete over_ex;
}

void NetworkManager::LoginInfoPacket(int id, BASE_PACKET* packet)
{
    SC_LOGIN_INFO_PACKET* p = reinterpret_cast<SC_LOGIN_INFO_PACKET*>(packet);

    if (playerScene != SCENEKIND::READY) {
        m_id = p->id;
        myInfo->x = p->x;
        myInfo->y = p->y;
        myInfo->z = p->z;
#ifdef WITH_DATABASE
        m_ArrayOtherClientCustom[p->id] = p->model;
#endif
    }
    else {
        for (int i = 0; i < INGAME_PLAYER; ++i) {
            if (3 == m_id)
            {
                myInfo->x = READY_SCENE_PLAYER_POS_BY_BOSS[m_id].x;
                myInfo->y = READY_SCENE_PLAYER_POS_BY_BOSS[m_id].y;
                myInfo->z = READY_SCENE_PLAYER_POS_BY_BOSS[m_id].z;
                //if (3 != i)
                {
                    otherClientsInfo[i].x = READY_SCENE_PLAYER_POS_BY_BOSS[i].x;
                    otherClientsInfo[i].y = READY_SCENE_PLAYER_POS_BY_BOSS[i].y;
                    otherClientsInfo[i].z = READY_SCENE_PLAYER_POS_BY_BOSS[i].z;
                }
            }
            else if (i == m_id) {
                otherClientsInfo[i].show = false;
                myInfo->x = READY_SCENE_PLAYER_POS[m_id].x;
                myInfo->y = READY_SCENE_PLAYER_POS[m_id].y;
                myInfo->z = READY_SCENE_PLAYER_POS[m_id].z;
            }
            else {
                otherClientsInfo[i].show = true;
                otherClientsInfo[i].x = READY_SCENE_PLAYER_POS[i].x;
                otherClientsInfo[i].y = READY_SCENE_PLAYER_POS[i].y;
                otherClientsInfo[i].z = READY_SCENE_PLAYER_POS[i].z;
            }
        }
    }

#ifdef WITH_DATABASE
    if(playerScene == SCENEKIND::TITLE)
        playerScene = SCENEKIND::LOBBY;
#endif
}

void NetworkManager::LoginFailPacket(int id, BASE_PACKET* packet)
{
    SC_LOGIN_FAIL_PACKET* p = reinterpret_cast<SC_LOGIN_FAIL_PACKET*>(packet);
    
    switch (p->reason) {
    case 0:
        cout << "No ID" << endl;
        break;
    case 1:
        cout << "Wrong Password" << endl;
        break;
    case 2:
        cout << "ID Exist" << endl;
        break;
    case 3:
        cout << "Server Err" << endl;
        break;
    default:
        break;
    }
}

void NetworkManager::AddPlayerPacket(int id, BASE_PACKET* packet)
{
    SC_ADD_PLAYER_PACKET* p = reinterpret_cast<SC_ADD_PLAYER_PACKET*>(packet);
    cout << "Add Player " << p->id << endl;
    if (m_id == p->id) {
        cout << m_id << endl;
        return;
    }
    
    otherClientsInfo[p->id].show = true;
    otherClientsInfo[p->id].lookX = p->lookX; otherClientsInfo[p->id].lookY = p->lookY; otherClientsInfo[p->id].lookZ = p->lookZ;
    otherClientsInfo[p->id].rightX = p->rightX; otherClientsInfo[p->id].rightY = p->rightY; otherClientsInfo[p->id].rightZ = p->rightZ;

    if (playerScene == SCENEKIND::LOBBY) {
        if (OtherClients[p->id]) {
            m_ArrayOtherClientCustom[p->id] = p->model;
        }
    }
    else {
        if (OtherClients[p->id]) {
            m_ArrayInGameClientsCustom[p->id] = p->model;
        }
    }
}

void NetworkManager::RemovePlayerPacket(int id, BASE_PACKET* packet)
{
    SC_REMOVE_PLAYER_PACKET* p = reinterpret_cast<SC_REMOVE_PLAYER_PACKET*>(packet);
    cout << "Remove Player " << p->id << endl;

    otherClientsInfo[p->id].show = false;
}

void NetworkManager::MovePacket(int id, BASE_PACKET* packet)
{
    SC_MOVE_PLAYER_PACKET* p = reinterpret_cast<SC_MOVE_PLAYER_PACKET*>(packet);

    if (m_id == p->id) {
        myInfo->prevX = myInfo->x; myInfo->prevY = myInfo->y; myInfo->prevZ = myInfo->z;

        myInfo->x = p->x;
        myInfo->y = p->y;
        myInfo->z = p->z;
        myInfo->lastPacketTime = p->move_time;
        myInfo->recvPacketTime = std::chrono::high_resolution_clock::now().time_since_epoch().count();

    }
    else {
        //other client
        otherClientsInfo[p->id].prevX = otherClientsInfo[p->id].x; otherClientsInfo[p->id].prevY = otherClientsInfo[p->id].y; otherClientsInfo[p->id].prevZ = otherClientsInfo[p->id].z;

        otherClientsInfo[p->id].x = p->x;
        otherClientsInfo[p->id].y = p->y;
        otherClientsInfo[p->id].z = p->z;
        otherClientsInfo[p->id].lastPacketTime = p->move_time;
        otherClientsInfo[p->id].recvPacketTime = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        //0:Stop 1:Forward 2:Backward 4:Left 5:Forward-Left 6:Backward-Left 8:Right 9:Forward-Right 10:Backward-Right
        switch (p->direction) {
        case 0:
            otherClientsInfo[p->id].animation = p->direction;
            break;
        case 1:
            otherClientsInfo[p->id].animation = p->direction;
            break;
        case 2:
            otherClientsInfo[p->id].animation = p->direction;
            break;
        case 4:
        case 5:
        case 6:
            otherClientsInfo[p->id].animation = 3;
            break;
        case 8:
        case 9:
        case 10:
            otherClientsInfo[p->id].animation = 4;
            break;
        }

    }

}

void NetworkManager::MatchPacket(int id, BASE_PACKET* packet)
{
    SC_MATCH_PACKET* p = reinterpret_cast<SC_MATCH_PACKET*>(packet);
  
    m_id = p->id;

    OverlapEx* over = new OverlapEx();
    over->ResetOver();
    over->SetOP(OP_TYPE::OP_DISCONNECT);
    over->SetInfo1(p->gameport);
    over->SetInfo2(p->gameip);
    if (!SocketUtil::DisconnectEx(m_socket, &over->GetOver(), TF_REUSE_SOCKET, NULL) && WSA_IO_PENDING != WSAGetLastError() && ERROR_IO_PENDING != WSAGetLastError()) {
        SocketUtil::PrintError("Disconnect");
        delete over;
    }

    for (int i = 0; i < INGAME_PLAYER; ++i) {
        otherClientsInfo[i].show = true;
    }

    for (int i = INGAME_PLAYER; i < LOBBY_MAX_CLIENT; ++i) {
        otherClientsInfo[i].show = false;
    }

    playerScene = SCENEKIND::READY;
    ListChating.clear();
}

void NetworkManager::MatchEndPacket(int id, BASE_PACKET* packet)
{
    Reset();                                   //Reset NetworkManager
    SceneManager::GetInstance()->Reset();      //Reset SceneManager
    CTextureShader::GetInstance()->Reset();    //Reset TextureShader
    UILayer::GetInstance()->Reset();           //Reset UILayer
    SC_MATCH_END_PACKET* p = reinterpret_cast<SC_MATCH_END_PACKET*>(packet);

    OverlapEx* over2 = new OverlapEx();
    over2->ResetOver();
    over2->SetOP(OP_TYPE::OP_DISCONNECT);
    over2->SetInfo1(p->lobbyport);
    over2->SetInfo2(p->lobbyip);
    if (!SocketUtil::DisconnectEx(m_socket, &over2->GetOver(), TF_REUSE_SOCKET, NULL) && WSA_IO_PENDING != WSAGetLastError() && ERROR_IO_PENDING != WSAGetLastError()) {
        SocketUtil::PrintError("Disconnect Error");
        delete over2;
    }

    playerScene = SCENEKIND::LOBBY;
    ListChating.clear();
}

void NetworkManager::ChatPacket(int id, BASE_PACKET* packet)
{
    SC_CHAT_PACKET* p = reinterpret_cast<SC_CHAT_PACKET*>(packet);
    
    SERVERCHAT newOp;
    newOp.chating = new WCHAR[256];

    switch (p->chatType)
    {
    case 0:
        wcscpy(newOp.chating, L"[전체] ");
        newOp.op = CHAT::ALL;
        break;
    case 1:
        wcscpy(newOp.chating, L"[채널] ");
        newOp.op = CHAT::CHANNEL;
        break;
    case 2:
        break;
    case 3:
        wcscpy(newOp.chating, L"");
        newOp.op = CHAT::ALL;
        break;
    default:
        wcscpy(newOp.chating, L"[채널] ");
        newOp.op = CHAT::CHANNEL;
        break;
    }
    
    WCHAR wcharArray[20];
    mbstowcs(wcharArray, p->name, NAME_SIZE);
    wcscat(newOp.chating, wcharArray);
    wcscat(newOp.chating, L": ");

    wcscat(newOp.chating, p->chat); 
    
    if (ListChating.size() >= 7)
    {
        WCHAR* temp = ListChating.front().chating;
        ListChating.pop_front();
        ListChating.push_back(newOp);
        delete[] temp;
    }
    else {
        ListChating.push_back(newOp);
    }
}

void NetworkManager::RotatePacket(int id, BASE_PACKET* packet)
{
    // Rotate Packet for other clients
    SC_ROTATE_PLAYER_PACKET* p = reinterpret_cast<SC_ROTATE_PLAYER_PACKET*>(packet);
    otherClientsInfo[p->id].lookX = p->lookX; otherClientsInfo[p->id].lookY = p->lookY; otherClientsInfo[p->id].lookZ = p->lookZ;
    otherClientsInfo[p->id].rightX = p->rightX; otherClientsInfo[p->id].rightY = p->rightY; otherClientsInfo[p->id].rightZ = p->rightZ;
}

void NetworkManager::SkillPacket(int id, BASE_PACKET* packet)
{
    if (playerScene != SCENEKIND::INGAME)
        return;
    SC_SKILL_PACKET* p = reinterpret_cast<SC_SKILL_PACKET*>(packet);

    //onOff:true  ->  onOff Skill turn off
    //onOff:false ->  Skills which are not onOff used
    if (p->onOff) {
        // Other Client Animation Process For On/Off Skills
        OtherClients[p->id]->m_pSkinnedAnimationController->m_pAnimationTracks[static_cast<int>(static_cast<SKILLKIND>(p->skillType)) + 4].m_bOnOffToggle = false;
        for (auto q : SceneManager::GetInstance()->m_ParticleInfo[static_cast<PARTICLE_SITUATION>(p->id)][static_cast<int>(static_cast<SKILLKIND>(p->skillType)) - 1])
        {
            if (q->show)
            {
                q->show = false;
            }
        }
        cout << "Otherclient finish Toggle Skill" << endl;
    }
    else {
        if (m_id == p->id) {
            playerSkill = static_cast<SKILLKIND>(p->skillType);
            skillUsed = true;
            myInfo->lastSkillTime[p->skillType - 1] = chrono::system_clock::now();
            UILayer::GetInstance()->m_skillCoolRemainingTime[p->skillType - 1] = reinterpret_cast<CGamePlayer*>(myClient)->GetSkillCoolTime(p->skillType - 1);
        }
        else {
            otherClientsInfo[p->id].playerSkill = static_cast<SKILLKIND>(p->skillType);
            otherClientsInfo[p->id].skillUsed = true;
            otherClientsInfo[p->id].lastSkillTime[p->skillType - 1] = chrono::system_clock::now();
        }
    }
}

void NetworkManager::CoolTimePacket(int id, BASE_PACKET* packet)
{    
    SC_COOLTIME_PACKET* p = reinterpret_cast<SC_COOLTIME_PACKET*>(packet);
    skillCoolTime[0] = p->skill1;
    skillCoolTime[1] = p->skill2;
    skillCoolTime[2] = p->skill3;
    skillCoolTime[3] = p->skill4;
    skillCoolTime[4] = p->skill5;
    reinterpret_cast<CGamePlayer*>(myClient)->LoadCoolTime();
}

void NetworkManager::AddNpcPacket(int id, BASE_PACKET* packet)
{
    SC_ADD_NPC_PACKET* p = reinterpret_cast<SC_ADD_NPC_PACKET*>(packet);
    if (p->id < MAX_MINION) {
        npcInfo[p->id].dissolve = false;
        npcInfo[p->id].x = p->x; npcInfo[p->id].y = p->y; npcInfo[p->id].z = p->z;
        npcInfo[p->id].lookX = p->lookX; npcInfo[p->id].lookY = p->lookY; npcInfo[p->id].lookZ = p->lookZ;
        npcInfo[p->id].rightX = p->rightX; npcInfo[p->id].rightY = p->rightY; npcInfo[p->id].rightZ = p->rightZ;
        minions[p->id]->m_Ani = ANI_ON_SERVER::IDLE; //walk
        //minions[p->id]->SetDissolveState(0);
        minions[p->id]->m_nObjectDissolveState = 0;
        minions[p->id]->m_fDissolveTime = 0;

        npcInfo[p->id].show = true;  //Make sure to move at the end
    }
    else {
        int monsterIndex = p->id - MAX_MINION;
        monsterInfo[monsterIndex].dissolve = false;
        monsterInfo[monsterIndex].x = p->x; monsterInfo[monsterIndex].y = p->y; monsterInfo[monsterIndex].z = p->z;
        monsterInfo[monsterIndex].lookX = p->lookX; monsterInfo[monsterIndex].lookY = p->lookY; monsterInfo[monsterIndex].lookZ = p->lookZ;
        monsterInfo[monsterIndex].rightX = p->rightX; monsterInfo[monsterIndex].rightY = p->rightY; monsterInfo[monsterIndex].rightZ = p->rightZ;
        monsters[monsterIndex]->m_Ani = ANI_ON_SERVER::IDLE;
        //monsters[monsterIndex]->SetDissolveState(0);
        monsters[monsterIndex]->m_nObjectDissolveState = 0;
        monsters[monsterIndex]->m_fDissolveTime = 0;

        monsterInfo[monsterIndex].show = true;// Make sure to move at the end
    }
}

void NetworkManager::SkillSelectPacket(int id, BASE_PACKET* packet)
{
    SC_SKILL_SELECT_PACKET* p = reinterpret_cast<SC_SKILL_SELECT_PACKET*>(packet);

    readySceneInfo->selectSkills[p->id][p->storage] = p->skill;
}

void NetworkManager::GameTimePacket(int id, BASE_PACKET* packet)
{
    SC_GAME_TIME_PACKET* p = reinterpret_cast<SC_GAME_TIME_PACKET*>(packet);

    if (p->timeType == 0) {
        readySceneInfo->time = p->time;
    }
    else {
        gameSceneInfo->time = p->time;

        if (IsEqual(floorf(p->time), 180.f) && magneticFenceActive) {
            magneticFenceActive = false;
            SoundManager::GetInstance()->Play_Sound(L"MagneticFenceDeactive.wav", CHANNELID::EFFECT);
        }
    }
}

void NetworkManager::ReadyPacket(int id, BASE_PACKET* packet)
{
    SC_READY_PACKET* p = reinterpret_cast<SC_READY_PACKET*>(packet);

    readySceneInfo->playerReadys[p->id] = p->ready;
}

void NetworkManager::JobSelectPacket(int id, BASE_PACKET* packet)
{
    SC_JOB_SELECT_PACKET* p = reinterpret_cast<SC_JOB_SELECT_PACKET*>(packet);

    readySceneInfo->playerJobs[p->id] = p->job;
}

void NetworkManager::GameStartPacket(int id, BASE_PACKET* packet)
{
    otherClientsInfo[m_id].show = false;
    playerScene = SCENEKIND::INGAME;
    ListChating.clear();

    SendStatSelectPacket(myClient->GetAdditionalStats());
}

void NetworkManager::CustomizePacket(int id, BASE_PACKET* packet)
{
    SC_MODEL_CUSTOMIZE_PACKET* p = reinterpret_cast<SC_MODEL_CUSTOMIZE_PACKET*>(packet);
    if (playerScene == SCENEKIND::LOBBY) {
        m_ArrayOtherClientCustom[p->id] = p->model;
    }
    else {
        m_ArrayInGameClientsCustom[p->id] = p->model;
    }
}

void NetworkManager::MoveNpcPacket(int id, BASE_PACKET* packet)
{
    SC_MOVE_NPC_PACKET* p = reinterpret_cast<SC_MOVE_NPC_PACKET*>(packet);

    if (p->id < MAX_MINION) {
        npcInfo[p->id].x = p->x;
        npcInfo[p->id].y = p->y;
        npcInfo[p->id].z = p->z;
        npcInfo[p->id].lookX = p->lookX; npcInfo[p->id].lookY = p->lookY; npcInfo[p->id].lookZ = p->lookZ;
        npcInfo[p->id].rightX = p->rightX; npcInfo[p->id].rightY = p->rightY; npcInfo[p->id].rightZ = p->rightZ;

        minions[p->id]->SetAnimation(static_cast<int>(MINION_ANIM::WALK));
    }
    else {
        int monsterIndex = p->id - MAX_MINION;
        monsterInfo[monsterIndex].x = p->x;
        monsterInfo[monsterIndex].y = p->y;
        monsterInfo[monsterIndex].z = p->z;
        monsterInfo[monsterIndex].lookX = p->lookX; monsterInfo[monsterIndex].lookY = p->lookY; monsterInfo[monsterIndex].lookZ = p->lookZ;
        monsterInfo[monsterIndex].rightX = p->rightX; monsterInfo[monsterIndex].rightY = p->rightY; monsterInfo[monsterIndex].rightZ = p->rightZ;

        if (p->idle) {
            monsters[monsterIndex]->SetAnimation(0);
            return;
        }

        switch (monsterIndex) {
        case 0:
            //monsters[monsterIndex]->SetAnimation(monsters[monsterIndex]->GetRunAnim()); //Unique Red
            break;
        case 1:
            monsters[monsterIndex]->SetAnimation(monsters[monsterIndex]->GetRunAnim());   //Rare Green
            break;
        case 2:
            monsters[monsterIndex]->SetAnimation(monsters[monsterIndex]->GetRunAnim());   //Rare Golem
            break;
        case 3:
            monsters[monsterIndex]->SetAnimation(monsters[monsterIndex]->GetRunAnim());   //Normal Bear
            break;
        case 4:
            monsters[monsterIndex]->SetAnimation(monsters[monsterIndex]->GetRunAnim());   //Normal Minotaur
            break;
        case 5:
        case 7:
            monsters[monsterIndex]->SetAnimation(monsters[monsterIndex]->GetRunAnim());   // Normal Chest
            break;
        case 6:
        case 8:
            monsters[monsterIndex]->SetAnimation(monsters[monsterIndex]->GetRunAnim());   // Normal Beholder
            break;
        }
    }
}

void NetworkManager::SendTowerActivatePacket(int num) const
{
    CS_TOWER_ACTIVATE_PACKET* p = new CS_TOWER_ACTIVATE_PACKET;
    p->size = sizeof(CS_TOWER_ACTIVATE_PACKET);
    p->type = CS_TOWER_ACTIVATE;
    p->num = num;
    SendPacket(p);
}

void NetworkManager::SendMinionPathPacket(int path) const
{
    CS_MINION_PATH_PACKET* p = new CS_MINION_PATH_PACKET;
    p->size = sizeof(CS_MINION_PATH_PACKET);
    p->type = CS_MINION_PATH;
    p->path = path;
    SendPacket(p);
}

void NetworkManager::SendNpcAttackFinishPacket(int id) const
{
    CS_NPC_ATTACK_FINISH_PACKET* p = new CS_NPC_ATTACK_FINISH_PACKET;
    p->size = sizeof(CS_NPC_ATTACK_FINISH_PACKET);
    p->type = CS_NPC_ATTACK_FINISH;
    p->id = id;
    SendPacket(p);
}

void NetworkManager::SendShopPacket() const
{
    CS_SHOP_PACKET* p = new CS_SHOP_PACKET;
    p->size = sizeof(CS_SHOP_PACKET);
    p->type = CS_SHOP;
    p->shopType = CTextureShader::GetInstance()->GetCurParts();
    SendPacket(p);
}

void NetworkManager::SendSignUpPacket(char name[NAME_SIZE], char password[NAME_SIZE]) const
{
    CS_SIGN_UP_PACKET* p = new CS_SIGN_UP_PACKET;
    p->size = sizeof(CS_SIGN_UP_PACKET);
    p->type = CS_SIGN_UP;
    memcpy_s(p->name, NAME_SIZE, name, NAME_SIZE);
    memcpy_s(p->password, NAME_SIZE, password, NAME_SIZE);
    SendPacket(p);
}

void NetworkManager::SendRegisterAuctionPacket(char name[NAME_SIZE], SHOP_TYPE type, short customizeNum, int buyTokenNum) const
{
    CS_REGISTER_AUCTION_PACKET* p = new CS_REGISTER_AUCTION_PACKET;
    p->size = sizeof(CS_REGISTER_AUCTION_PACKET);
    p->type = CS_REGISTER_AUCTION;
    memcpy_s(p->name, NAME_SIZE, name, NAME_SIZE);
    p->customizeType = static_cast<short>(type);
    p->customizeNum = customizeNum;
    p->buyPrice = buyTokenNum;
    SendPacket(p);
}

void NetworkManager::SendChangeChannelPacket(int channel)
{
    CS_CHANGE_CHANNEL_PACKET* p = new CS_CHANGE_CHANNEL_PACKET;
    p->size = sizeof(CS_CHANGE_CHANNEL_PACKET);
    p->type = CS_CHANGE_CHANNEL;
    p->channel = channel;
    SendPacket(p);

    for (int i = 0; i < LOBBY_MAX_CLIENT; ++i) {
        otherClientsInfo[i].show = false;
    }
}

void NetworkManager::SendPortNumPacket()
{
    CS_PORT_NUM_PACKET* p = new CS_PORT_NUM_PACKET;
    p->size = sizeof(CS_PORT_NUM_PACKET);
    p->type = CS_PORT_NUM;
    p->portNum = static_cast<unsigned short>(m_portNum);
    SendPacket(p);
}

void NetworkManager::SendStakeTokenPacket(int tokenNum, unsigned short stakeDays)
{
    CS_STAKE_TOKEN_PACKET* p = new CS_STAKE_TOKEN_PACKET;
    p->size = sizeof(CS_STAKE_TOKEN_PACKET);
    p->type = CS_STAKE_TOKEN;
    p->stakeNum = tokenNum;
    p->stakeDays = stakeDays;
    SendPacket(p);
}

void NetworkManager::SendChangeNodePacket(bool fullNode)
{
    CS_CHANGE_NODE_PACKET* p = new CS_CHANGE_NODE_PACKET;
    p->size = sizeof(CS_CHANGE_NODE_PACKET);
    p->type = CS_CHANGE_NODE;
    p->fullNode = fullNode;
    SendPacket(p);
}

void NetworkManager::SendStatSelectPacket(const Additional_Stats& stats)
{
    CS_STAT_SELECT_PACKET* p = new CS_STAT_SELECT_PACKET;
    p->size = sizeof(CS_STAT_SELECT_PACKET);
    p->type = CS_STAT_SELECT;

    p->hp = stats.hp;
    p->mp = stats.mp;
    p->attack = stats.attack;
    p->magic_attack = stats.magic_attack;
    p->defense = stats.defense;
    p->magic_defense = stats.magic_defense;
    p->speed = stats.speed;
    p->tenacity = stats.tenacity;
    p->critical = stats.critical;

    if (stats.point != 0) {
        p->hp += stats.point;
    }

    SendPacket(p);
}

void NetworkManager::SendBuyItemPacket(ITEMKIND type)
{
    CS_BUY_ITEM_PACKET* p = new CS_BUY_ITEM_PACKET;
    p->size = sizeof(CS_BUY_ITEM_PACKET);
    p->type = CS_BUY_ITEM;
    p->itemType = static_cast<char>(type);
    SendPacket(p);
}

void NetworkManager::SendBuyStatPacket(ITEMKIND type)
{
    CS_BUY_STAT_PACKET* p = new CS_BUY_STAT_PACKET;
    p->size = sizeof(CS_BUY_STAT_PACKET);
    p->type = CS_BUY_STAT;
    p->statType = static_cast<char>(type);
    SendPacket(p);
}

void NetworkManager::SendUseItemPacket(int num)
{
    CS_USE_ITEM_PACKET* p = new CS_USE_ITEM_PACKET;
    p->size = sizeof(CS_USE_ITEM_PACKET);
    p->type = CS_USE_ITEM;
    p->itemNum = static_cast<short>(num);
    SendPacket(p);
}

void NetworkManager::SendSkillSelectPacket(int storage, int skill) const
{
    CS_SKILL_SELECT_PACKET* p = new CS_SKILL_SELECT_PACKET;
    p->size = sizeof(CS_SKILL_SELECT_PACKET);
    p->type = CS_SKILL_SELECT;
    p->id = m_id;
    p->storage = storage;
    p->skill = skill;
    SendPacket(p);
}

void NetworkManager::SendMatchPacket(char role, bool match) const
{
    CS_MATCH_PACKET* p = new CS_MATCH_PACKET;
    p->size = sizeof(CS_MATCH_PACKET);
    p->type = CS_MATCH;
    p->match_time = static_cast<unsigned int>(chrono::duration_cast<chrono::milliseconds>(chrono::high_resolution_clock::now().time_since_epoch()).count());
    p->character = role;
    p->match = true;
    SendPacket(p);
}

void NetworkManager::SendReadyPacket(bool ready) const
{
    CS_READY_PACKET* p = new CS_READY_PACKET;
    p->size = sizeof(CS_READY_PACKET);
    p->type = CS_READY;
    p->id = m_id;
    p->ready = ready;
    SendPacket(p);
}

void NetworkManager::SendJobSelectPacket(int job) const
{
    CS_JOB_SELECT_PACKET* p = new CS_JOB_SELECT_PACKET;
    p->size = sizeof(CS_JOB_SELECT_PACKET);
    p->type = CS_JOB_SELECT;
    p->id = m_id;
    p->job = static_cast<short>(job);
    SendPacket(p);
}

void NetworkManager::SendChatPacket(int id, WCHAR chatBuf[256], char name[NAME_SIZE], CHAT option) const
{
    CS_CHAT_PACKET* p = new CS_CHAT_PACKET;
    p->size = sizeof(CS_CHAT_PACKET);
    p->type = CS_CHAT;
    wcscpy_s(p->chat, chatBuf);
    p->id = id;
    memset(p->name, 0, NAME_SIZE);
    memcpy_s(p->name, NAME_SIZE, name, NAME_SIZE);

    switch (option) {
    case CHAT::ALL:
        p->chatType = 0;
        break;
    case CHAT::CHANNEL:
        p->chatType = 1;
        break;
    }

    SendPacket(p);
}

void NetworkManager::SendLoginPacket(char name[NAME_SIZE], char password[NAME_SIZE])
{
    CS_LOGIN_PACKET* p = new CS_LOGIN_PACKET;
    p->size = sizeof(CS_LOGIN_PACKET);
    p->type = CS_LOGIN;
    memcpy_s(p->name, NAME_SIZE, name, NAME_SIZE);

#ifdef WITH_DATABASE
    if (playerScene == SCENEKIND::TITLE || playerScene == SCENEKIND::LOBBY) {
        memcpy_s(p->password, NAME_SIZE, password, NAME_SIZE);
        memcpy_s(clientPassword, NAME_SIZE, password, NAME_SIZE);
    }
#endif
    SendPacket(p);
}

void NetworkManager::SendRotatePacket(const XMFLOAT3& look, const XMFLOAT3& right) const
{
    CS_ROTATE_PACKET* p = new CS_ROTATE_PACKET;
    p->size = sizeof(CS_ROTATE_PACKET);
    p->type = CS_ROTATE;
    p->lookX = look.x; p->lookY = look.y; p->lookZ = look.z;
    p->rightX = right.x; p->rightY = right.y; p->rightZ = right.z;
    SendPacket(p);
}

void NetworkManager::SendSkillFinishPacket() const
{
    CS_SKILL_FINISH_PACKET* p = new CS_SKILL_FINISH_PACKET;
    p->size = sizeof(CS_SKILL_FINISH_PACKET);
    p->type = CS_SKILL_FINISH;
    SendPacket(p);
}

void NetworkManager::SendModelCustomizePacket(const ModelCustomize& model) const
{
    CS_CUSTOMIZE_PACKET* p = new CS_CUSTOMIZE_PACKET;
    p->size = sizeof(CS_CUSTOMIZE_PACKET);
    p->type = CS_CUSTOMIZE;
    p->model = model;
    SendPacket(p);
}

void NetworkManager::SendSkillPacket(SKILLKIND skill, bool onOff) const
{
    CS_SKILL_PACKET* p = new CS_SKILL_PACKET;
    p->size = sizeof(CS_SKILL_PACKET);
    p->type = CS_SKILL;
    p->skillType = static_cast<char>(skill);
    p->id = m_id;
    p->onOff = onOff;
    SendPacket(p);
}

void NetworkManager::SendLoadCompletePacket() const
{
    CS_LOAD_COMPLETE_PACKET* p = new CS_LOAD_COMPLETE_PACKET;
    p->size = sizeof(CS_LOAD_COMPLETE_PACKET);
    p->type = CS_LOAD_COMPLETE;
    p->id = m_id;
    SendPacket(p);
}

void NetworkManager::SendCreateTransactionPacket() const
{
    CS_CREATE_TRANSACTION_PACKET* p = new CS_CREATE_TRANSACTION_PACKET;
    p->size = sizeof(CS_CREATE_TRANSACTION_PACKET);
    p->type = CS_CREATE_TRANSACTION;
    SendPacket(p);
}

void NetworkManager::SendOpenAuctionPacket() const
{
    CS_OPEN_AUCTION_PACKET* p = new CS_OPEN_AUCTION_PACKET;
    p->size = sizeof(CS_OPEN_AUCTION_PACKET);
    p->type = CS_OPEN_AUCTION;
    SendPacket(p);
}

void NetworkManager::SendOpenBlockChainPacket() const
{
    CS_OPEN_BLOCKCHAIN_PACKET* p = new CS_OPEN_BLOCKCHAIN_PACKET;
    p->size = sizeof(CS_OPEN_BLOCKCHAIN_PACKET);
    p->type = CS_OPEN_BLOCKCHAIN;
    SendPacket(p);
}

void NetworkManager::SendOpenCustomizePacket() const
{
    CS_OPEN_CUSTOMIZE_PACKET* p = new CS_OPEN_CUSTOMIZE_PACKET;
    p->size = sizeof(CS_OPEN_CUSTOMIZE_PACKET);
    p->type = CS_OPEN_CUSTOMIZE;
    SendPacket(p);
}

void NetworkManager::SendGetAuctionInfoPacket(int pageNum) const
{
    CS_GET_AUCTION_INFO_PACKET* p = new CS_GET_AUCTION_INFO_PACKET;
    p->size = sizeof(CS_GET_AUCTION_INFO_PACKET);
    p->type = CS_GET_AUCTION_INFO;
    p->pageNum = pageNum;
    SendPacket(p);
}

void NetworkManager::SendBuyAuctionPacket(char sellerName[NAME_SIZE], short customizeType, short customizeNum, unsigned short buyPrice) const
{
    if (m_tokenNum < buyPrice || !strcmp(sellerName, "")) {
        SoundManager::GetInstance()->Play_Sound(L"AuctionBuyFail.wav", CHANNELID::EFFECT, 0.8f);
        return;
    }

    CS_BUY_AUCTION_PACKET* p = new CS_BUY_AUCTION_PACKET;
    p->size = sizeof(CS_BUY_AUCTION_PACKET);
    p->type = CS_BUY_AUCTION;
    memcpy_s(p->sellerName, NAME_SIZE, sellerName, NAME_SIZE);
    p->customizeType = customizeType;
    p->customizeNum = customizeNum;
    p->buyPrice = buyPrice;
    SendPacket(p);
    SoundManager::GetInstance()->Play_Sound(L"AuctionBuy.wav", CHANNELID::EFFECT, 0.8f);
}

void NetworkManager::SendDebugGoldPacket()
{
    CS_DEBUG_GOLD_PACKET* p = new CS_DEBUG_GOLD_PACKET;
    p->size = sizeof(CS_DEBUG_GOLD_PACKET);
    p->type = CS_DEBUG_GOLD;
    SendPacket(p);
}

void NetworkManager::SendLoginCompletePacket()
{
    CS_LOGIN_COMPLETE_PACKET* p = new CS_LOGIN_COMPLETE_PACKET;
    p->size = sizeof(CS_LOGIN_COMPLETE_PACKET);
    p->type = CS_LOAD_COMPLETE;
    SendPacket(p);
}

void NetworkManager::ChangeServerPacket(int id, BASE_PACKET* packet)
{
    SC_TEST_CHANGE_SERVER_PACKET* p = reinterpret_cast<SC_TEST_CHANGE_SERVER_PACKET*>(packet);

    m_id = p->id;

    OverlapEx* over = new OverlapEx();
    over->ResetOver();
    over->SetOP(OP_TYPE::OP_DISCONNECT);
    over->SetInfo1(p->gameport);
    over->SetInfo2(p->gameip);
    if (false == SocketUtil::DisconnectEx(m_socket, &over->GetOver(), TF_REUSE_SOCKET, NULL) && WSA_IO_PENDING != WSAGetLastError() && ERROR_IO_PENDING != WSAGetLastError()) {
        SocketUtil::PrintError("Disconnect");
        delete over;
    }

    delete otherClientsInfo;
    otherClientsInfo = new ObjectInfo[INGAME_PLAYER];
    for (int i = 0; i < INGAME_PLAYER; ++i) {
        otherClientsInfo[i].Initialize();
    }

    for (int i = 0; i < INGAME_PLAYER; ++i) {
        otherClientsInfo[i].show = false;
    }

    for (int i = INGAME_PLAYER; i < LOBBY_MAX_CLIENT; ++i) {
        otherClientsInfo[i].show = false;
    }

    playerScene = SCENEKIND::INGAME;
    ListChating.clear();
}

void NetworkManager::TeleportPacket(int id, BASE_PACKET* packet)
{
    SC_TELEPORT_PACKET* p = reinterpret_cast<SC_TELEPORT_PACKET*>(packet);

    if (p->id == m_id) {
        myInfo->show = p->finish;
        if (!p->finish) {
            SoundManager::GetInstance()->Play_Sound(L"Teleport.wav", CHANNELID::EFFECT);
        }
    }
    else {
        otherClientsInfo[p->id].show = p->finish;
    }
}

void NetworkManager::TowerAttackPacket(int id, BASE_PACKET* packet)
{
    SC_TOWER_ATTACK_PACKET* p = reinterpret_cast<SC_TOWER_ATTACK_PACKET*>(packet);
    
    towerAttacks[p->id]->SetPosition(p->x, p->y, p->z);
}

void NetworkManager::TowerAttackRemovePacket(int id, BASE_PACKET* packet)
{
    SC_TOWER_ATTACK_REMOVE_PACKET* p = reinterpret_cast<SC_TOWER_ATTACK_REMOVE_PACKET*>(packet);
    towerAttacks[p->id]->SetShow(false);
}

void NetworkManager::TowerAttackAddPacket(int id, BASE_PACKET* packet)
{    
    SC_TOWER_ATTACK_ADD_PACKET* p = reinterpret_cast<SC_TOWER_ATTACK_ADD_PACKET*>(packet);
    towerAttacks[p->id]->SetShow(true);
    towerAttacks[p->id]->SetPosition(p->x, p->y, p->z);

    if (Util::DistanceXZ(towerAttacks[p->id]->GetPosition(), myClient->GetPosition()) < OBJECT_EFFECT_SOUND_DISTANCE)
        SoundManager::GetInstance()->Play_Sound(L"TowerAttack.wav", CHANNELID::EFFECT);
}

void NetworkManager::RemoveNpcPacket(int id, BASE_PACKET* packet)
{
    SC_REMOVE_NPC_PACKET* p = reinterpret_cast<SC_REMOVE_NPC_PACKET*>(packet);
    if (p->id < MAX_MINION) {
        npcInfo[p->id].dissolve = true;
        minions[p->id]->SetCurHp(0);
        minions[p->id]->m_Ani = ANI_ON_SERVER::DEAD;
        
        if (Util::DistanceXZ(minions[p->id]->GetPosition(), myClient->GetPosition()) < OBJECT_EFFECT_SOUND_DISTANCE)
            SoundManager::GetInstance()->Play_Sound(L"MinionDeath.wav", CHANNELID::NPC, 0.7f);


    }
    else {        
        int monsterIndex = p->id - MAX_MINION;
        monsterInfo[monsterIndex].dissolve = true;
        monsters[monsterIndex]->SetCurHp(0);
        monsters[monsterIndex]->m_Ani = ANI_ON_SERVER::DEAD;

        SceneManager::GetInstance()->m_ParticleInfo[PARTICLE_SITUATION::COINBYDEATH][0][monsterIndex]->pos = XMFLOAT3(monsterInfo[monsterIndex].x, monsterInfo[monsterIndex].y, monsterInfo[monsterIndex].z);
        SceneManager::GetInstance()->m_ParticleInfo[PARTICLE_SITUATION::COINBYDEATH][0][monsterIndex]->show = true;



        if (Util::DistanceXZ(monsters[monsterIndex]->GetPosition(), myClient->GetPosition()) < OBJECT_EFFECT_SOUND_DISTANCE)
        {
            SoundManager::GetInstance()->Play_Sound(monsters[monsterIndex]->GetDeathSoundString(), CHANNELID::NPC);
            SoundManager::GetInstance()->Play_Sound(L"ShopBuy.wav", CHANNELID::EFFECT, 0.7f);
        }
            
    }
}

void NetworkManager::AddSkillObjectPacket(int id, BASE_PACKET* packet)
{
    SC_ADD_SKILL_OBJECT_PACKET* p = reinterpret_cast<SC_ADD_SKILL_OBJECT_PACKET*>(packet);

    skillObjectInfos[static_cast<SKILL_TYPE>(p->objectType)][p->id].pos = XMFLOAT3(p->x, p->y, p->z);
    skillObjectInfos[static_cast<SKILL_TYPE>(p->objectType)][p->id].look = XMFLOAT3(p->lookX, p->lookY, p->lookZ);
    skillObjectInfos[static_cast<SKILL_TYPE>(p->objectType)][p->id].show = true;

    if (Util::DistanceXZ(XMFLOAT3(myInfo->x, myInfo->y, myInfo->z), XMFLOAT3(p->x, p->y, p->z)) < SOUND_DISTANCE)
        SoundManager::GetInstance()->Play_Sound(static_cast<SKILL_TYPE>(p->objectType), CHANNELID::SKILL);
}

void NetworkManager::UpdateSkillObjectPacket(int id, BASE_PACKET* packet)
{
    SC_UPDATE_SKILL_OBJECT_PACKET* p = reinterpret_cast<SC_UPDATE_SKILL_OBJECT_PACKET*>(packet);

    //skillObjectInfos[static_cast<SKILL_TYPE>(p->objectType)][p->id].show = true;
    skillObjectInfos[static_cast<SKILL_TYPE>(p->objectType)][p->id].pos = XMFLOAT3(p->x, p->y, p->z);
}

void NetworkManager::RemoveSkillObjectPacket(int id, BASE_PACKET* packet)
{
    SC_REMOVE_SKILL_OBJECT_PACKET* p = reinterpret_cast<SC_REMOVE_SKILL_OBJECT_PACKET*>(packet);

    skillObjectInfos[static_cast<SKILL_TYPE>(p->objectType)][p->id].show = false;
    cout << "RemoveSkillObjectPacket Type : "<< static_cast<int>(p->objectType)<<" id : " << p->id << endl;

    if (sqrtf(powf(myInfo->x - skillObjectInfos[static_cast<SKILL_TYPE>(p->objectType)][p->id].pos.x, 2.f) + powf(myInfo->z - skillObjectInfos[static_cast<SKILL_TYPE>(p->objectType)][p->id].pos.z, 2.f)) < SOUND_DISTANCE)
        SoundManager::GetInstance()->Play_RemoveSound(static_cast<SKILL_TYPE>(p->objectType), CHANNELID::SKILL);
}

void NetworkManager::NpcAttackPacket(int id, BASE_PACKET* packet)
{
    SC_NPC_ATTACK_PACKET* p = reinterpret_cast<SC_NPC_ATTACK_PACKET*>(packet);

    if (p->id < MAX_MINION) {
        minions[p->id]->SetAnimation(static_cast<int>(MINION_ANIM::ATTACK1));

        if(Util::DistanceXZ(minions[p->id]->GetPosition(), myClient->GetPosition()) < OBJECT_EFFECT_SOUND_DISTANCE)
            SoundManager::GetInstance()->Play_Sound(L"MinionAttack.wav", CHANNELID::NPC, 0.7f);
    }
    else {
        int monsterIndex = p->id - MAX_MINION;
        monsters[monsterIndex]->SetAnimation(monsters[monsterIndex]->GetAttackAnim());

        if (Util::DistanceXZ(monsters[monsterIndex]->GetPosition(), myClient->GetPosition()) < OBJECT_EFFECT_SOUND_DISTANCE)
            SoundManager::GetInstance()->Play_Sound(monsters[monsterIndex]->GetAttackSoundString(), CHANNELID::NPC);
    }
}

void NetworkManager::NpcStatChangePacket(int id, BASE_PACKET* packet)
{
    SC_NPC_STAT_CHANGE_PACKET* p = reinterpret_cast<SC_NPC_STAT_CHANGE_PACKET*>(packet);

    if (p->id < MAX_MINION) {
        minions[p->id]->SetMaxHp(p->maxHp);
        minions[p->id]->SetCurHp(p->curHp);
    }
    else {
        int monsterIndex = p->id - MAX_MINION;
        monsters[monsterIndex]->SetMaxHp(p->maxHp);
        monsters[monsterIndex]->SetCurHp(p->curHp);
    }
}

void NetworkManager::GiveGoldPacket(int id, BASE_PACKET* packet)
{
    SC_GIVE_GOLD_PACKET* p = reinterpret_cast<SC_GIVE_GOLD_PACKET*>(packet);
    reinterpret_cast<CGamePlayer*>(myClient)->SetGold(p->gold);
}

void NetworkManager::PlayerStatChangePacket(int id, BASE_PACKET* packet)
{
    SC_PLAYER_STAT_CHANGE_PACKET* p = reinterpret_cast<SC_PLAYER_STAT_CHANGE_PACKET*>(packet);

    if (p->id == m_id) {
        Player_Info info = reinterpret_cast<CGamePlayer*>(myClient)->GetPlayerInfo();
        info.Speed = p->speed;
        reinterpret_cast<CGamePlayer*>(myClient)->SetPlayerInfo(info);
        float speed = 0.f;
        if (m_id != 3)//hero
        {
            for (int i = 1; i < 5; ++i)
            {
                speed = info.Speed * AniSpeed(static_cast<JOB>(readySceneInfo->playerJobs[m_id]), i);
                myClient->m_pSkinnedAnimationController->m_pAnimationTracks[i].SetSpeed(speed);
            }
        }
        else//boss
        {
            if(static_cast<BOSSJOB>(readySceneInfo->playerJobs[m_id]) == BOSSJOB::OGRE)
                for (int i = 1; i < 5; ++i)
                {
                    speed = info.Speed * AniSpeed(static_cast<BOSSJOB>(readySceneInfo->playerJobs[m_id]), i);
                    myClient->m_pSkinnedAnimationController->m_pAnimationTracks[i].SetSpeed(speed);
                }
        }
    }
    else {
        Player_Info info = reinterpret_cast<COtherClientPlayer*>(OtherClients[p->id])->GetPlayerInfo();
        info.Speed = p->speed;
        reinterpret_cast<COtherClientPlayer*>(OtherClients[p->id])->SetPlayerInfo(info);
        float speed = 0.f;
        if (p->id != 3)//hero
        {
            for (int i = 1; i < 5; ++i)
            {
                speed = info.Speed * AniSpeed(static_cast<JOB>(readySceneInfo->playerJobs[p->id]), i);
                OtherClients[p->id]->m_pSkinnedAnimationController->m_pAnimationTracks[i].SetSpeed(speed);
            }
        }
        else//boss
        {
            if (static_cast<BOSSJOB>(readySceneInfo->playerJobs[p->id]) == BOSSJOB::OGRE)
                for (int i = 1; i < 5; ++i)
                {
                    speed = info.Speed * AniSpeed(static_cast<BOSSJOB>(readySceneInfo->playerJobs[p->id]), i);
                    OtherClients[p->id]->m_pSkinnedAnimationController->m_pAnimationTracks[i].SetSpeed(speed);
                }
        }
    }
}

void NetworkManager::PlayerStatusChangePacket(int id, BASE_PACKET* packet)
{
    SC_PLAYER_STATUS_CHANGE_PACKET* p = reinterpret_cast<SC_PLAYER_STATUS_CHANGE_PACKET*>(packet);
    
    switch (p->statusType) {
    case 0:
        //SKILL_BUFF
        if (p->id == m_id) {
            reinterpret_cast<CPlayerObject*>(myClient)->SetSkillBuff(static_cast<SKILL_BUFF>(p->statusNum));
        }
        else {
            reinterpret_cast<CPlayerObject*>(OtherClients[p->id])->SetSkillBuff(static_cast<SKILL_BUFF>(p->statusNum));
        }
        break;
    default:
        break;
    }
}

void NetworkManager::HealthManaPacket(int id, BASE_PACKET* packet)
{
    SC_HEALTH_MANA_PACKET* p = reinterpret_cast<SC_HEALTH_MANA_PACKET*>(packet);

    if (p->id == m_id) {
        if (myInfo->dissolve || !myInfo->show)
            return;
        Player_Info info = reinterpret_cast<CGamePlayer*>(myClient)->GetPlayerInfo();
        if (info.CurHp > p->curHp) CTextureShader::GetInstance()->GetDamage();
        info.MaxHp = p->maxHp;
        info.CurHp = p->curHp;
        info.MaxMp = p->maxMp;
        info.CurMp = p->curMp;
        reinterpret_cast<CGamePlayer*>(myClient)->SetPlayerInfo(info);
        //cout << "Player Stat Change: " << reinterpret_cast<CGamePlayer*>(myClient)->GetPlayerInfo().CurHp << endl;
    }
    else {
        if (otherClientsInfo[p->id].dissolve || !otherClientsInfo[p->id].show)
            return;
        Player_Info info = reinterpret_cast<COtherClientPlayer*>(OtherClients[p->id])->GetPlayerInfo();
        info.MaxHp = p->maxHp;
        info.CurHp = p->curHp;
        info.MaxMp = p->maxMp;
        info.CurMp = p->curMp;
        reinterpret_cast<COtherClientPlayer*>(OtherClients[p->id])->SetPlayerInfo(info);
    }
}

void NetworkManager::SkillFinishPacket(int id, BASE_PACKET* packet)
{
    //Skill Finish Packet For Stopping Animation
    SC_SKILL_FINISH_PACKET* p = reinterpret_cast<SC_SKILL_FINISH_PACKET*>(packet);
    cout << "SkillFinishPacket" << endl;
    if (m_id == p->id)
    {
        myClient->m_Ani = ANI_ON_SERVER::IDLE;
        reinterpret_cast<CPlayer*>(myClient)->SetUsingSkill(false);
    }
    else
    {
        OtherClients[p->id]->m_Ani = ANI_ON_SERVER::IDLE;
    }
        
}

void NetworkManager::ShopPacket(int id, BASE_PACKET* packet)
{
    SC_SHOP_PACKET* p = reinterpret_cast<SC_SHOP_PACKET*>(packet);
    //EyeBrow: 1~7 Man, 8~17 Woman
    if (p->customizePart == -1)
        cout << "Not Enough Token" << endl;
    else {
        cout << "ShopType: " << p->customizePart << endl;
        cout << "Num: " << p->customizeDetail << endl;
        lobbySceneInfo->partNum = p->customizeDetail;
        lobbySceneInfo->shopType = p->customizePart;
    }
}

void NetworkManager::AuctionInfoPacket(int id, BASE_PACKET* packet)
{
    cout << "Auction Info Packet" << endl;
    SC_AUCTION_INFO_PACKET* p = reinterpret_cast<SC_AUCTION_INFO_PACKET*>(packet);

    for (int i = 0; i < AUCTION_DATA_NUM; ++i) {
        if (p->auctionInfos[i].buyPrice == 0) {
            AuctionInfo temp;
            memset(temp.playerName, 0, NAME_SIZE);
            temp.customizeNum = 0;
            temp.customizeNum = 0;
            temp.buyPrice = 0;
            memset(temp.deadLine, 0, TIME_SIZE);
            lobbySceneInfo->auctionPageInfos[i] = temp;
        }
        else {
            AuctionInfo temp;
            memcpy_s(temp.playerName, NAME_SIZE, p->auctionInfos[i].playerName, NAME_SIZE);
            temp.customizeNum = p->auctionInfos[i].customizeType;
            temp.customizeNum = p->auctionInfos[i].customizeNum;
            temp.buyPrice = p->auctionInfos[i].buyPrice;
            memcpy_s(temp.deadLine, TIME_SIZE, p->auctionInfos[i].deadLine, TIME_SIZE);
            lobbySceneInfo->auctionPageInfos[i] = temp;

            memcpy_s(CTextureShader::GetInstance()->m_auctionInfo->username[i], NAME_SIZE, p->auctionInfos[i].playerName, NAME_SIZE);
            CTextureShader::GetInstance()->m_auctionInfo->productParts[i] = p->auctionInfos[i].customizeType;
            CTextureShader::GetInstance()->m_auctionInfo->productNum[i] = p->auctionInfos[i].customizeNum;
            CTextureShader::GetInstance()->m_auctionInfo->productPrice[i] = p->auctionInfos[i].buyPrice;

            cout << "Name: " << p->auctionInfos[i].playerName << ", Customize Type: " << p->auctionInfos[i].customizeType <<
                ", Customize Num: " << p->auctionInfos[i].customizeNum << ", BuyPrice: " << p->auctionInfos[i].buyPrice << ", DeadLine: " << p->auctionInfos[i].deadLine << endl;
        }
    }
}

void NetworkManager::PlayerRespawnPacket(int id, BASE_PACKET* packet)
{
    SC_PLAYER_RESPAWN_PACKET* p = reinterpret_cast<SC_PLAYER_RESPAWN_PACKET*>(packet);

    // respawn:respawn complete. true:waiting respawn
    if (p->id == m_id) {
        if (!p->respawn)
        {
            myInfo->show = !p->respawn;
            myInfo->dissolve = false;
            myClient->m_Ani = ANI_ON_SERVER::IDLE;
            myClient->m_nObjectDissolveState = 0;
            myClient->m_fDissolveTime = 0;
            dynamic_cast<CPlayer*>(myClient)->SetUsingSkill(false);
            Player_Info info = reinterpret_cast<CGamePlayer*>(myClient)->GetPlayerInfo();
            info.CurHp = info.MaxHp;
            info.CurMp = info.MaxMp;
            reinterpret_cast<CGamePlayer*>(myClient)->SetPlayerInfo(info);
        }       
        if (p->respawn)
        {
            myInfo->dissolve = true;
            myClient->m_Ani = ANI_ON_SERVER::DEAD;
            switch (readySceneInfo->playerJobs[m_id]) {
            case static_cast<int>(JOB::ARCHER):
                SoundManager::GetInstance()->Play_Sound(L"Archer_Die.wav", CHANNELID::PLAYER);
                break;
            case static_cast<int>(JOB::FIGHTER):
                SoundManager::GetInstance()->Play_Sound(L"Fighter_Die.wav", CHANNELID::PLAYER);
                break;
            case static_cast<int>(JOB::SWORDMAN):
                SoundManager::GetInstance()->Play_Sound(L"Swordman_Die.wav", CHANNELID::PLAYER);
                break;
            case static_cast<int>(JOB::WIZARD):
                SoundManager::GetInstance()->Play_Sound(L"Wizzard_Die.wav", CHANNELID::PLAYER);
                break;
            }
            Player_Info info = reinterpret_cast<CGamePlayer*>(myClient)->GetPlayerInfo();
            info.CurHp = 0;
            reinterpret_cast<CGamePlayer*>(myClient)->SetPlayerInfo(info);
        }
            
    }
    else {
        if (!p->respawn)
        {
            otherClientsInfo[p->id].show = !p->respawn;
            otherClientsInfo[p->id].dissolve = false;
            OtherClients[p->id]->m_Ani = ANI_ON_SERVER::IDLE;
            OtherClients[p->id]->m_nObjectDissolveState = 0;
            OtherClients[p->id]->m_fDissolveTime = 0;       

            Player_Info info = reinterpret_cast<COtherClientPlayer*>(OtherClients[p->id])->GetPlayerInfo();
            info.CurHp = info.MaxHp;
            info.CurMp = info.MaxMp;
            reinterpret_cast<COtherClientPlayer*>(OtherClients[p->id])->SetPlayerInfo(info);
        }         
        if (p->respawn)
        {
            otherClientsInfo[p->id].dissolve = true;
            OtherClients[p->id]->m_Ani = ANI_ON_SERVER::DEAD;
            otherClientsInfo[p->id].animation;

            Player_Info info = reinterpret_cast<COtherClientPlayer*>(OtherClients[p->id])->GetPlayerInfo();
            info.CurHp = 0;
            reinterpret_cast<COtherClientPlayer*>(OtherClients[p->id])->SetPlayerInfo(info);

        }
            
    }
}

void NetworkManager::StructureStatChangePacket(int id, BASE_PACKET* packet)
{
    SC_STRUCTURE_STAT_CHANGE_PACKET* p = reinterpret_cast<SC_STRUCTURE_STAT_CHANGE_PACKET*>(packet);
    
    //p->id: 0~3:Tower, 4:Nexus (From Boss Respawn Left Tower is 0, Right Tower is 3)
    structureInfo[p->id].maxHp = p->maxHp;
    structureInfo[p->id].curHp = p->curHp;

    cout << "ID: " << p->id << ", HP: " << p->curHp << endl;
}

void NetworkManager::StructureStatusChangePacket(int id, BASE_PACKET* packet)
{
    SC_STRUCTURE_STATUS_CHANGE_PACKET* p = reinterpret_cast<SC_STRUCTURE_STATUS_CHANGE_PACKET*>(packet);

    if (p->broken) {
        structureInfo[p->id].broken = true;
        SoundManager::GetInstance()->Play_Sound(L"TowerDestroy.wav", CHANNELID::EFFECT);
    }
    else {
        for (int i = 0; i < PATH_NUM; ++i) {
            if (i == p->id) {
                structureInfo[i].active = true;
                if (Util::DistanceXZ(TOWER_POS[i], myClient->GetPosition()) < OBJECT_EFFECT_SOUND_DISTANCE)
                    SoundManager::GetInstance()->Play_Sound(L"TowerActive.wav", CHANNELID::EFFECT);
            }
            else {
                structureInfo[i].active = false;
            }
        }
    }
}

void NetworkManager::ChangeChannelPacket(int id, BASE_PACKET* packet)
{
    SC_CHANGE_CHANNEL_PACKET* p = reinterpret_cast<SC_CHANGE_CHANNEL_PACKET*>(packet);

    if (p->fail) {
        SoundManager::GetInstance()->Play_Sound(L"ChannelChangeFail.wav", CHANNELID::EFFECT);
    }
    else {
        m_channel = p->channel;
        SoundManager::GetInstance()->Play_Sound(L"ChannelChange.wav", CHANNELID::EFFECT);
        CTextureShader::GetInstance()->ChannelSwitch();
    }
}

void NetworkManager::PeerInfoPacket(int id, BASE_PACKET* packet)
{
    SC_PEER_INFO_PACKET* p = reinterpret_cast<SC_PEER_INFO_PACKET*>(packet);

    //Send Data To Other Peers for Register
    if (!PeerManager::GetInstance()->RegisterPeer(std::string(p->peerIP), p->peerPortNum)) {
        cout << "Failed To Connect To P2P Network" << endl;
    }
}

void NetworkManager::TokenNumPacket(int id, BASE_PACKET* packet)
{
    SC_TOKEN_NUM_PACKET* p = reinterpret_cast<SC_TOKEN_NUM_PACKET*>(packet);
    m_tokenNum = p->tokenNum;
}

void NetworkManager::ChangeNodePacket(int id, BASE_PACKET* packet)
{
    SC_CHANGE_NODE_PACKET* p = reinterpret_cast<SC_CHANGE_NODE_PACKET*>(packet);
    
    if (p->fail) {
        switch (p->reason) {
        case 0:
            cout << "Not Enough Token" << endl;
            break;
        case 1:
            cout << "Server Error" << endl;
            break;
        }
    }
    else {
        cout << "Change Node Success" << endl;
    }
}

void NetworkManager::StakeTokenPacket(int id, BASE_PACKET* packet)
{
    SC_STAKE_TOKEN_PACKET* p = reinterpret_cast<SC_STAKE_TOKEN_PACKET*>(packet);
    
    if (p->fail) {
        switch (p->reason) {
        case 0:
            cout << "Not Enough Stake Days" << endl;
            break;
        case 1:
            cout << "Not Enough Staking Token, Min is 10" << endl;
            break;
        case 2:
            cout << "Server Error" << endl;
            break;
        }
    }
    else {
        cout << "Stake Token Success" << endl;
    }
}

void NetworkManager::TransactionPacket(int id, BASE_PACKET* packet)
{
    SC_TRANSACTION_PACKET* p = reinterpret_cast<SC_TRANSACTION_PACKET*>(packet);
    
    PeerManager::GetInstance()->CreateBlock(p->transactions);
}

void NetworkManager::MagicEyePacket(int id, BASE_PACKET* packet)
{
    SC_MAGIC_EYE_POS_PACKET* p = reinterpret_cast<SC_MAGIC_EYE_POS_PACKET*>(packet);

    if (p->show) {
        //Show boss pos in minimap
    }
    else {
        //don't show boss pos in minimap
    }
}

void NetworkManager::BlockHeaderPacket(int id, BASE_PACKET* packet)
{
    SC_BLOCK_HEADER_PACKET* p = reinterpret_cast<SC_BLOCK_HEADER_PACKET*>(packet);
    m_blockHeader += string(p->hash, SHA256::BLOCKSIZE);
    m_blockHeader += to_string(p->version);
    m_blockHeader += string(p->timeStamp, 20);
    m_blockHeader += string(p->prevHash, SHA256::BLOCKSIZE);
    m_blockHeader += string(p->merkleRoot, SHA256::BLOCKSIZE);
    m_blockHeader += to_string(p->validatorID);

    if (m_blockBody.size() == SHA256::BLOCKSIZE * 7) {
        m_blockHeader += m_blockBody;

        PeerManager::GetInstance()->CreateBlock(m_blockHeader);

        m_blockBody.clear();
        m_blockHeader.clear();
    }
}

void NetworkManager::BlockBodyPacket(int id, BASE_PACKET* packet)
{
    SC_BLOCK_BODY_PACKET* p = reinterpret_cast<SC_BLOCK_BODY_PACKET*>(packet);

    m_blockBody += string(p->transaction, SHA256::BLOCKSIZE);

    if (m_blockHeader.size() > 0 && m_blockBody.size() == SHA256::BLOCKSIZE * 7) {
        m_blockHeader += m_blockBody;
        PeerManager::GetInstance()->CreateBlock(m_blockHeader);

        m_blockBody.clear();
        m_blockHeader.clear();
    }
}

void NetworkManager::FullNodePacket(int id, BASE_PACKET* packet)
{
    SC_FULLNODE_PACKET* p = reinterpret_cast<SC_FULLNODE_PACKET*>(packet);

    PeerManager::GetInstance()->SetFullNode(p->fullNode);
}

void NetworkManager::TeleportActivePacket(int id, BASE_PACKET* packet)
{
    SC_TELEPORT_ACTIVE_PACKET* p = reinterpret_cast<SC_TELEPORT_ACTIVE_PACKET*>(packet);

    teleportActive = p->active;

    if (p->active) {
        SoundManager::GetInstance()->Play_Sound(L"TeleportActive.wav", CHANNELID::EFFECT, 1.2f);
    }
}

void NetworkManager::GameOverPacket(int id, BASE_PACKET* packet)
{
    SC_GAME_OVER_PACKET* p = reinterpret_cast<SC_GAME_OVER_PACKET*>(packet);

    if (p->nexusDestroy) {
        if (m_id == 3) {
            CTextureShader::GetInstance()->Win();
        }
        else {
            CTextureShader::GetInstance()->Defeat();
        }
        //Boss Win
        SoundManager::GetInstance()->Play_Sound(L"NexusDestroy.wav", CHANNELID::EFFECT);
    }
    else {
        if (m_id != 3) {
            CTextureShader::GetInstance()->Win();
        }
        else {
            CTextureShader::GetInstance()->Defeat();
        }
        //Hero Win
        if (readySceneInfo->playerJobs[3] - MAX_JOB == static_cast<int>(BOSSJOB::OGRE)) {
            SoundManager::GetInstance()->Play_Sound(L"GameOverOgre.wav", CHANNELID::EFFECT);
        }
        else {
            SoundManager::GetInstance()->Play_Sound(L"GameOverProgrammer.wav", CHANNELID::EFFECT);
        }

        if (m_id == 3)
        {
            myClient->m_Ani = ANI_ON_SERVER::DEAD;
            myInfo->dissolve = true;
        }
        else
        {
            OtherClients[3]->m_Ani = ANI_ON_SERVER::DEAD;
            otherClientsInfo[3].dissolve = true;
        }
    }
}

void NetworkManager::MonsterKillBuffPacket(int id, BASE_PACKET* packet)
{
    SC_MONSTER_KILL_BUFF_PACKET* p = reinterpret_cast<SC_MONSTER_KILL_BUFF_PACKET*>(packet);

    if (p->monsterType == 0) {
        //Unique(Red) all stat buff
        if (p->id == m_id) {
            
        }
        else {

        }
    }
    else if (p->monsterType == 1) {
        //Green attack, util(endure,critical) buff
        if (p->id == m_id) {

        }
        else {

        }
    }
    else {
        //Golem defense, util(endure,critical) buff
        if (p->id == m_id) {

        }
        else {

        }
    }
}

void NetworkManager::JumpFinishPacket(int id, BASE_PACKET* packet)
{
    reinterpret_cast<CGamePlayer*>(myClient)->SetJumping(false);
}

void NetworkManager::CustomizePartsPacket(int id, BASE_PACKET* packet)
{
    SC_CUSTOMIZE_PARTS_PACKET* p = reinterpret_cast<SC_CUSTOMIZE_PARTS_PACKET*>(packet);

    for (int i = 0; i < lobbySceneInfo->customizeDatas.size(); ++i) {
        lobbySceneInfo->customizeDatas[i].clear();
    }

    for (int i = 0; i < CUSTOMIZE_PART_NUM_FROM_SERVER; ++i) {
        if (static_cast<int>(p->partType[i]) != 200) {
            lobbySceneInfo->customizeDatas[static_cast<int>(p->partType[i])].push_back({ static_cast<int>(p->customizeNum[i]), static_cast<int>(p->count[i]) });
        }
    }

    for (int i = 0; i < lobbySceneInfo->customizeDatas.size(); ++i) {
        sort(lobbySceneInfo->customizeDatas[i].begin(), lobbySceneInfo->customizeDatas[i].end(), [](const pair<int, int>& p1, const pair<int, int>& p2) {
            return p1.first < p2.first;
            });
    }
}

void NetworkManager::AuctionPartsNumPacket(int id, BASE_PACKET* packet)
{
    SC_AUCTION_PARTS_NUM_PACKET* p = reinterpret_cast<SC_AUCTION_PARTS_NUM_PACKET*>(packet);
    lobbySceneInfo->auctionPartNum = p->num;

    CTextureShader::GetInstance()->m_auctionInfo->maxPageNum = ((p->num - 1) / 7) + 1;
}

void NetworkManager::StakedTokenInfoPacket(int id, BASE_PACKET* packet)
{
    SC_STAKE_TOKEN_INFO_PACKET* p = reinterpret_cast<SC_STAKE_TOKEN_INFO_PACKET*>(packet);

    CTextureShader::GetInstance()->m_blockChainInfo->stakingTokens = p->stakedToken;
    CTextureShader::GetInstance()->m_blockChainInfo->unstakingDeadline = p->remainingDay;
}

void NetworkManager::RTTPacket(int id, BASE_PACKET* packet)
{
    SC_RTT_PACKET* p = reinterpret_cast<SC_RTT_PACKET*>(packet);

    CS_RTT_PACKET* clientPacket = new CS_RTT_PACKET;
    clientPacket->size = sizeof(CS_RTT_PACKET);
    clientPacket->type = CS_RTT;
    clientPacket->serverTime = p->time;
    clientPacket->time = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    SendPacket(clientPacket);
}
