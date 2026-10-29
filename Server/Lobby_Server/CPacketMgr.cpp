#include "pch.h"
#include "CPacketMgr.h"
#include "CMatchMgr.h"
#include "CNetworkMgr.h"

namespace wod_server
{
std::unique_ptr<CPacketMgr> CPacketMgr::m_instance;

bool CPacketMgr::Initialize()
{
    try
    {
        m_packetfunc.insert({CS_LOGIN, [this](BASE_PACKET* _p, CClient* _c) {
                                 LoginPacket(_p, _c);
                             }});
        m_packetfunc.insert({CS_MOVE, [this](BASE_PACKET* _p, CClient* _c) {
                                 MovePacket(_p, _c);
                             }});
        m_packetfunc.insert({CS_MATCH, [this](BASE_PACKET* _p, CClient* _c) {
                                 MatchPacket(_p, _c);
                             }});
        m_packetfunc.insert({CS_CHAT, [this](BASE_PACKET* _p, CClient* _c) {
                                 ChatPacket(_p, _c);
                             }});
        m_packetfunc.insert({CS_ROTATE, [this](BASE_PACKET* _p, CClient* _c) {
                                 RotatePacket(_p, _c);
                             }});
        m_packetfunc.insert({CS_CUSTOMIZE, [this](BASE_PACKET* _p, CClient* _c) {
                                 CustomizePacket(_p, _c);
                             }});
        m_packetfunc.insert({CS_SHOP, [this](BASE_PACKET* _p, CClient* _c) {
                                 ShopPacket(_p, _c);
                             }});
        m_packetfunc.insert({CS_SIGN_UP, [this](BASE_PACKET* _p, CClient* _c) {
                                 SignUpPacket(_p, _c);
                             }});
        m_packetfunc.insert({CS_CHANGE_CHANNEL, [this](BASE_PACKET* _p, CClient* _c) {
                                 ChangeChannelPacket(_p, _c);
                             }});
        m_packetfunc.insert({CS_PORT_NUM, [this](BASE_PACKET* _p, CClient* _c) {
                                 PortNumPacket(_p, _c);
                             }});
        m_packetfunc.insert({CS_CHANGE_NODE, [this](BASE_PACKET* _p, CClient* _c) {
                                 ChangeNodePacket(_p, _c);
                             }});
        m_packetfunc.insert({CS_STAKE_TOKEN, [this](BASE_PACKET* _p, CClient* _c) {
                                 StakeTokenPacket(_p, _c);
                             }});
        m_packetfunc.insert({CS_DUMMY_CLIENT, [this](BASE_PACKET* _p, CClient* _c) {
                                 DummyClientPacket(_p, _c);
                             }});
        m_packetfunc.insert({CS_CREATE_TRANSACTION, [this](BASE_PACKET* _p, CClient* _c) {
                                 CreateTransactionPacket(_p, _c);
                             }});
        m_packetfunc.insert({CS_OPEN_AUCTION, [this](BASE_PACKET* _p, CClient* _c) {
                                 OpenAuctionPacket(_p, _c);
                             }});
        m_packetfunc.insert({CS_OPEN_CUSTOMIZE, [this](BASE_PACKET* _p, CClient* _c) {
                                 OpenCustomizePacket(_p, _c);
                             }});
        m_packetfunc.insert({CS_OPEN_BLOCKCHAIN, [this](BASE_PACKET* _p, CClient* _c) {
                                 OpenBlockChainPacket(_p, _c);
                             }});
        m_packetfunc.insert({CS_GET_AUCTION_INFO, [this](BASE_PACKET* _p, CClient* _c) {
                                 GetAuctionInfoPacket(_p, _c);
                             }});
        m_packetfunc.insert({CS_REGISTER_AUCTION, [this](BASE_PACKET* _p, CClient* _c) {
                                 RegisterAuctionPacket(_p, _c);
                             }});
        m_packetfunc.insert({CS_BUY_AUCTION, [this](BASE_PACKET* _p, CClient* _c) {
                                 BuyAuctionPacket(_p, _c);
                             }});
        m_packetfunc.insert({CS_RTT, [this](BASE_PACKET* _p, CClient* _c) {
                                 RTTPacket(_p, _c);
                             }});

        //For Test
        m_packetfunc.insert({CS_TEST_CHANGE_SERVER, [this](BASE_PACKET* _p, CClient* _c) {
                                 ChangeServerPacket(_p, _c);
                             }});

        m_randomEngine = std::mt19937(std::random_device{}());

        std::fstream in("Resource/CustomizeNumber.txt");
        if (!in)
            throw std::runtime_error("missing Resource/CustomizeNumber.txt");
        std::string temp;
        for (int i = 0; i < m_shopMaxNums.size(); ++i)
        {
            if (!(in >> temp))
                throw std::runtime_error("incomplete CustomizeNumber.txt");
            if (temp == "#" || temp == "")
            {
                if (!(in >> temp))
                    throw std::runtime_error("incomplete CustomizeNumber.txt");
                --i;
                continue;
            }
            m_shopMaxNums[i] = std::stoi(temp) - 1;
        }

        return true;
    }
    catch (std::exception ex)
    {
        LogPrinter::PrintMsg(ex.what());
        return false;
    }
}

bool CPacketMgr::Release()
{
    return true;
}

bool CPacketMgr::RequiresGlobal(const BASE_PACKET* _packet)
{
    switch (_packet->type)
    {
    case CS_LOGIN:
    case CS_MOVE:
    case CS_ROTATE:
    case CS_CUSTOMIZE:
    case CS_RTT:
        return false;
    case CS_CHAT:
        return reinterpret_cast<const CS_CHAT_PACKET*>(_packet)->chatType != 1;
    default:
        return true;
    }
}

void CPacketMgr::Packet_Exec(BASE_PACKET* _packet, CClient* _client)
{

    if (_packet->type != CS_LOGIN && _packet->type != CS_SIGN_UP && _packet->type != CS_RTT && _packet->type != CS_DUMMY_CLIENT && _packet->type != CS_TEST_CHANGE_SERVER && _client->GetState() != CL_STATE::ST_LOBBY &&
        _client->GetState() != CL_STATE::ST_MATCHING)
        return;

    if (!network::GetInstance()->GetDataBaseThread()->IsEnabled())
    {
        switch (_packet->type)
        {
        case CS_SIGN_UP:
            _client->GetPacketSender()->SendLoginFailPacket(3);
            return;
        case CS_SHOP:
        case CS_CHANGE_NODE:
        case CS_STAKE_TOKEN:
        case CS_CREATE_TRANSACTION:
        case CS_OPEN_AUCTION:
        case CS_OPEN_BLOCKCHAIN:
        case CS_OPEN_CUSTOMIZE:
        case CS_GET_AUCTION_INFO:
        case CS_REGISTER_AUCTION:
        case CS_BUY_AUCTION:
            return;
        }
    }

    const auto handler = std::as_const(m_packetfunc).find(_packet->type);
    if (handler != m_packetfunc.cend())
    {
        handler->second(_packet, _client);
        wod::core::JobMetrics::Packet();
    }
    else
    {
        LogPrinter::PrintMsg(static_cast<int>(_packet->type) + ": Undefined Packet Type");
    }
}

void CPacketMgr::LoginPacket(BASE_PACKET* _packet, CClient* _client)
{
    if (_client->GetState() != CL_STATE::ST_ALLOC)
        return;

    if (network::GetInstance()->GetDataBaseThread()->IsEnabled())
    {
        Authenticate(_packet, _client, false);
        return;
    }

    const auto* request = reinterpret_cast<const CS_LOGIN_PACKET*>(_packet);
    _client->SetName(request->name);
    _client->GetPacketSender()->SendTokenNumPacket(2000);
    CompleteLogin(_client);
}

void CPacketMgr::MovePacket(BASE_PACKET* _packet, CClient* _client)
{
    CS_MOVE_PACKET* p = reinterpret_cast<CS_MOVE_PACKET*>(_packet);

    wod::core::JobMetrics::Move(p->direction == 0);
    _client->ApplyMovementInput(p->direction);
}

void CPacketMgr::MatchPacket(BASE_PACKET* _packet, CClient* _client)
{
    auto* packet = reinterpret_cast<CS_MATCH_PACKET*>(_packet);
    const int socket = _client->GetSocketID();
    if (!packet->match)
    {
        match::GetInstance()->Cancel(socket);
        if (_client->GetState() == CL_STATE::ST_MATCHING)
            _client->SetState(CL_STATE::ST_LOBBY);

        return;
    }

    if (_client->GetState() != CL_STATE::ST_LOBBY && _client->GetState() != CL_STATE::ST_MATCHING)
        return;

    _client->SetState(CL_STATE::ST_MATCHING);
    match::GetInstance()->RegisterToQue(socket, packet->character, packet->match_time);
    std::array<int, 4> players;
    if (!match::GetInstance()->TryMakeMatch(players))
        return;

    LG_MATCH_START_PACKET start = {sizeof(LG_MATCH_START_PACKET), LG_MATCH_START, static_cast<short>(m_matchNum)};
    network::GetInstance()->GetGameServer()->Send(&start);
    for (int i = 0; i < 4; ++i)
    {
        auto* client = CUserMgr::GetInstance()->GetClient(players[i]);
        LG_MATCH_PACKET info = {sizeof(LG_MATCH_PACKET), LG_MATCH_PLAYER, "", static_cast<short>(m_matchNum), i, client->GetModelCustomize()};
        memcpy_s(info.name, NAME_SIZE, client->GetName(), NAME_SIZE);
        network::GetInstance()->GetGameServer()->Send(&info);
        client->SetState(CL_STATE::ST_INGAME);
        client->GetPacketSender()->SendMatchPacket(i);
    }

    ++m_matchNum;
}

void CPacketMgr::ChatPacket(BASE_PACKET* _packet, CClient* _client)
{
    CS_CHAT_PACKET* p = reinterpret_cast<CS_CHAT_PACKET*>(_packet);

    SC_CHAT_PACKET scp;
    scp.size = sizeof(SC_CHAT_PACKET);
    scp.type = SC_CHAT;
    scp.chatType = p->chatType;
    wcscpy_s(scp.chat, p->chat);
    memcpy_s(scp.name, NAME_SIZE, _client->GetName(), NAME_SIZE);

    switch (p->chatType)
    {
    case 0: {
        //broadcast to all clients
        for (const auto& [socketNum, channelClient] : CUserMgr::GetInstance()->GetAllClient())
        {
            if (nullptr == channelClient || channelClient->IsDisconnected() || (channelClient->GetState() != CL_STATE::ST_LOBBY && channelClient->GetState() != CL_STATE::ST_MATCHING))
            {
                continue;
            }
            channelClient->GetPacketSender()->GetSession()->Send(&scp);
        }
        break;
    }
    case 1: {
        // channel
        int channel = _client->GetChannel();
        for (const auto& channelClient : CUserMgr::GetInstance()->GetChannelClients(channel))
        {
            if (nullptr == channelClient || channelClient->IsDisconnected() || (channelClient->GetState() != CL_STATE::ST_LOBBY && channelClient->GetState() != CL_STATE::ST_MATCHING))
            {
                continue;
            }
            channelClient->GetPacketSender()->GetSession()->Send(&scp);
        }
    }
    break;
    case 2:
        // party
        break;
    default:
        LogPrinter::PrintMsg("Wrong chat type");
        break;
    }

#ifdef TEST
    std::cout << "Client " << p->id << " Sended : ";
    std::wcout << p->chat << std::endl;
#endif // TEST
}

void CPacketMgr::RotatePacket(BASE_PACKET* _packet, CClient* _client)
{
    CS_ROTATE_PACKET* p = reinterpret_cast<CS_ROTATE_PACKET*>(_packet);
    _client->GetTransform()->SetLook(vec3(p->lookX, p->lookY, p->lookZ));
    _client->GetTransform()->SetRight(vec3(p->rightX, p->rightY, p->rightZ));

    for (int nearClientID : _client->GetViewList()->GetView())
    {
        CUserMgr::GetInstance()->GetClient(nearClientID)->GetPacketSender()->SendRotatePacket(_client->GetID(), _client->GetTransform()->GetLook(), _client->GetTransform()->GetRight());
    }
}

void CPacketMgr::CustomizePacket(BASE_PACKET* _packet, CClient* _client)
{
    CS_CUSTOMIZE_PACKET* p = reinterpret_cast<CS_CUSTOMIZE_PACKET*>(_packet);

    if (network::GetInstance()->GetDataBaseThread()->IsEnabled())
    {
        const auto model = p->model;
        const std::string name(_client->GetName());
        _client->RunDatabase([name, model](CDataBase& _database) {
            _database.SaveAppearance(name, model);

            return [model](CClient* _client) {
                _client->SetModelCustomize(model);
                _client->GetPacketSender()->SendModelCustomizePacket(_client->GetID(), model);
                for (int id : _client->GetViewList()->GetView())
                    CUserMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendModelCustomizePacket(_client->GetID(), model);
            };
        });
        return;
    }

    _client->SetModelCustomize(p->model);
    _client->GetPacketSender()->SendModelCustomizePacket(_client->GetID(), _client->GetModelCustomize());

    for (int nearClientID : _client->GetViewList()->GetView())
    {
        CUserMgr::GetInstance()->GetClient(nearClientID)->GetPacketSender()->SendModelCustomizePacket(_client->GetID(), _client->GetModelCustomize());
    }
}

void CPacketMgr::ShopPacket(BASE_PACKET* _packet, CClient* _client)
{
    const auto request = *reinterpret_cast<const CS_SHOP_PACKET*>(_packet);
    if (request.shopType < 0 || request.shopType >= static_cast<int>(m_shopMaxNums.size()))
        return;

    if (_client->GetPlayerInfo().m_tokenNum < SHOP_BUY_TOKEN_NUM)
    {
        _client->GetPacketSender()->SendShopPacket(-1, -1);
        return;
    }

    const int part = request.shopType;
    const int num = std::uniform_int_distribution<>(0, m_shopMaxNums[part])(m_randomEngine);
    const std::string name(_client->GetName());
    _client->RunDatabase([name, part, num](CDataBase& _database) {
        PlayerInfo balance{};
        _database.Transaction([&] {
            if (!_database.SaveShopBuyInfo(name, part, num) || !_database.SaveToken(name, -SHOP_BUY_TOKEN_NUM))
                throw std::runtime_error("shop persistence failed");

            balance = _database.ReadBalance(name);
        });

        return [part, num, balance](CClient* _client) {
            _client->ApplyBalance(balance);
            _client->GetPacketSender()->SendShopPacket(part, num);
            _client->GetPacketSender()->SendTokenNumPacket(_client->GetPlayerInfo().m_tokenNum);
            CPacketMgr::GetInstance()->RecordTransaction(_client, -SHOP_BUY_TOKEN_NUM);
        };
    });
}

void CPacketMgr::SignUpPacket(BASE_PACKET* _packet, CClient* _client)
{
    if (_client->GetState() == CL_STATE::ST_ALLOC)
        Authenticate(_packet, _client, true);
}

void CPacketMgr::RegisterAuctionPacket(BASE_PACKET* _packet, CClient* _client)
{
    const auto request = *reinterpret_cast<const CS_REGISTER_AUCTION_PACKET*>(_packet);
    const std::string name(_client->GetName());
    _client->RunDatabase([name, request](CDataBase& _database) {
        const auto future = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now() + std::chrono::hours(24));
        std::tm local{};
        localtime_s(&local, &future);
        std::ostringstream deadline;
        deadline << std::put_time(&local, "%Y-%m-%d %H:%M:%S");
        if (!_database.RegisterAuction(name, request.customizeType, request.customizeNum, request.buyPrice, deadline.str()))
            throw std::runtime_error("auction registration failed");

        return [](CClient*) {
        };
    });
}

void CPacketMgr::GetAuctionInfoPacket(BASE_PACKET* _packet, CClient* _client)
{
    const int page = reinterpret_cast<const CS_GET_AUCTION_INFO_PACKET*>(_packet)->pageNum;
    _client->RunDatabase([page](CDataBase& _database) {
        auto rows = _database.GetAuctionInfo(page);
        _database.ThrowIfFailed();

        return [rows = std::move(rows)](CClient* _client) {
            _client->GetPacketSender()->SendAuctionInfoPacket(rows);
        };
    });
}

void CPacketMgr::ChangeChannelPacket(BASE_PACKET* _packet, CClient* _client)
{
    CS_CHANGE_CHANNEL_PACKET* p = reinterpret_cast<CS_CHANGE_CHANNEL_PACKET*>(_packet);

    if (CUserMgr::GetInstance()->ChangeChannel(_client->GetChannel(), p->channel, _client))
    {
        _client->GetPacketSender()->SendChangeChannelPacket(false, p->channel);
        _client->GetPacketSender()->SendLoginInfoPacket(_client->GetID(), _client->GetTransform()->GetPos(), _client->GetModelCustomize());
        _client->Move(); // 기존 이동 예약과 legacy dummy의 전송 정책을 유지하며 시야만 갱신한다.
    }
    else
    {
        _client->GetPacketSender()->SendChangeChannelPacket(true, _client->GetChannel());
    }
}

void CPacketMgr::PortNumPacket(BASE_PACKET* _packet, CClient* _client)
{
    CS_PORT_NUM_PACKET* p = reinterpret_cast<CS_PORT_NUM_PACKET*>(_packet);

    network::GetInstance()->GetP2PNetwork()->RegisterPortNum(_client->GetIP(), p->portNum);

    std::unordered_map<std::string, int> peerInfos = network::GetInstance()->GetP2PNetwork()->GetPeerInfos();

    if (_client->IsFullNode())
    {
        for (const auto& info : peerInfos)
        {
            if (info.first != _client->GetIP())
                _client->GetPacketSender()->SendPeerInfoPacket(info.first, info.second);
        }
    }
    else
    {
        if (peerInfos.size() > 1)
        {
            for (auto iter = peerInfos.end(); iter != peerInfos.begin();)
            {
                --iter;
                if (iter->first != _client->GetIP())
                {
                    _client->GetPacketSender()->SendPeerInfoPacket(iter->first, iter->second);
                    return;
                }
            }
        }
    }
}

void CPacketMgr::StakeTokenPacket(BASE_PACKET* _packet, CClient* _client)
{
    const auto request = *reinterpret_cast<const CS_STAKE_TOKEN_PACKET*>(_packet);
    if (request.stakeDays < MIN_STAKE_DAYS || request.stakeNum < MIN_STAKE_TOKEN || request.stakeNum > _client->GetPlayerInfo().m_tokenNum)
    {
        _client->GetPacketSender()->SendStakeTokenPacket(true, request.stakeDays < MIN_STAKE_DAYS ? 0 : 1);
        return;
    }

    const std::string name(_client->GetName());
    _client->RunDatabase([name, request](CDataBase& _database) {
        int token = 0, days = 0, count = 0;
        PlayerInfo balance{};
        _database.Transaction([&] {
            if (!_database.StakeTokens(name, request.stakeNum, request.stakeDays) || !_database.GetStakedTokenData(name, token, days, count))
                throw std::runtime_error("staking failed");

            balance = _database.ReadBalance(name);
        });

        return [request, token, days, count, balance](CClient* _client) {
            _client->ApplyBalance(balance);
            _client->GetPacketSender()->SendStakeTokenPacket(false, 0);
            _client->GetPacketSender()->SendTokenNumPacket(_client->GetPlayerInfo().m_tokenNum);
            _client->GetPacketSender()->SendStakedTokenInfoPacket(token, days - count);
            CPacketMgr::GetInstance()->RecordTransaction(_client, -request.stakeNum);
        };
    });
}

void CPacketMgr::ChangeNodePacket(BASE_PACKET* _packet, CClient* _client)
{
    const bool full = reinterpret_cast<const CS_CHANGE_NODE_PACKET*>(_packet)->fullNode;
    if (_client->GetPlayerInfo().m_tokenNum < MIN_STAKE_TOKEN * 100)
    {
        _client->GetPacketSender()->SendChangeNodePacket(true, 0);
        return;
    }

    const std::string name(_client->GetName());
    _client->RunDatabase([name, full](CDataBase& _database) {
        if (!_database.ChangeNodeType(name, full))
            throw std::runtime_error("node persistence failed");

        return [full](CClient* _client) {
            _client->SetFullNode(full);
            _client->GetPacketSender()->SendChangeNodePacket(false, 0);
        };
    });
}

void CPacketMgr::DummyClientPacket(BASE_PACKET* _packet, CClient* _client)
{
    CS_DUMMY_CLIENT_PACKET* p = reinterpret_cast<CS_DUMMY_CLIENT_PACKET*>(_packet);

    std::string dummyName = "Dummy" + std::to_string(p->id);

    _client->m_stateLock.lock();
    _client->SetState(CL_STATE::ST_LOBBY);
    _client->m_stateLock.unlock();

    _client->SetName(dummyName.c_str());
    _client->GetTransform()->SetPos({static_cast<float>(rand() % 300), 5.f, static_cast<float>(rand() % 300)});

    _client->GetPacketSender()->SendDummyLoginInfo(_client->GetID(), _client->GetTransform()->GetPos());

    //Initialize client position
    _client->ProcessUpdate(true);
}

void CPacketMgr::CreateTransactionPacket(BASE_PACKET* _packet, CClient* _client)
{
    const int token = GenerateRandomNumber(100, 1000);
    const std::string name(_client->GetName());
    _client->RunDatabase([name, token](CDataBase& _database) {
        if (!_database.SaveToken(name, token))
            throw std::runtime_error("token persistence failed");

        const auto balance = _database.ReadBalance(name);

        return [token, balance](CClient* _client) {
            _client->ApplyBalance(balance);
            _client->GetPacketSender()->SendTokenNumPacket(_client->GetPlayerInfo().m_tokenNum);
            CPacketMgr::GetInstance()->RecordTransaction(_client, token);
        };
    });
}

void CPacketMgr::OpenCustomizePacket(BASE_PACKET* _packet, CClient* _client)
{
    const std::string name(_client->GetName());
    _client->RunDatabase([name](CDataBase& _database) {
        std::vector<std::tuple<short, short, short>> rows;
        if (!_database.GetPlayerCustomizeParts(name, rows))
            throw std::runtime_error("customization query failed");

        _database.ThrowIfFailed();

        return [rows = std::move(rows)](CClient* _client) {
            short types[CUSTOMIZE_PART_NUM_FROM_SERVER], nums[CUSTOMIZE_PART_NUM_FROM_SERVER], counts[CUSTOMIZE_PART_NUM_FROM_SERVER];
            for (size_t i = 0; i < CUSTOMIZE_PART_NUM_FROM_SERVER; ++i)
            {
                types[i] = i < rows.size() ? std::get<0>(rows[i]) : 200;
                nums[i] = i < rows.size() ? std::get<1>(rows[i]) : 200;
                counts[i] = i < rows.size() ? std::get<2>(rows[i]) : 200;
            }

            _client->GetPacketSender()->SendCustomizePartsPacket(types, nums, counts);
        };
    });
}

void CPacketMgr::OpenAuctionPacket(BASE_PACKET* _packet, CClient* _client)
{
    _client->RunDatabase([](CDataBase& _database) {
        const int count = _database.GetAuctionPartsNum();
        if (count < 0)
            throw std::runtime_error("auction count failed");

        return [count](CClient* _client) {
            _client->GetPacketSender()->SendAuctionPartsNumPacket(count);
        };
    });
}

void CPacketMgr::OpenBlockChainPacket(BASE_PACKET* _packet, CClient* _client)
{
    const std::string name(_client->GetName());
    _client->RunDatabase([name](CDataBase& _database) {
        int token = 0, days = 0, count = 0;
        if (!_database.GetStakedTokenData(name, token, days, count))
            throw std::runtime_error("staking query failed");

        return [token, days, count](CClient* _client) {
            _client->GetPacketSender()->SendStakedTokenInfoPacket(token, days - count);
        };
    });
}

void CPacketMgr::BuyAuctionPacket(BASE_PACKET* _packet, CClient* _client)
{
    const auto request = *reinterpret_cast<const CS_BUY_AUCTION_PACKET*>(_packet);
    if (!request.buyPrice || _client->GetPlayerInfo().m_tokenNum < request.buyPrice)
        return;

    const std::string name(_client->GetName());
    _client->RunDatabase([name, request](CDataBase& _database) {
        PlayerInfo buyerBalance{}, sellerBalance{};
        _database.Transaction([&] {
            if (!_database.BuyAuction(request.sellerName, name, request.customizeType, request.customizeNum, request.buyPrice) || !_database.SaveToken(name, -static_cast<int>(request.buyPrice)) ||
                !_database.SaveToken(request.sellerName, request.buyPrice))
                throw std::runtime_error("auction purchase failed");

            buyerBalance = _database.ReadBalance(name);
            sellerBalance = _database.ReadBalance(request.sellerName);
        });

        return [request, buyerBalance, sellerBalance](CClient* _client) {
            _client->ApplyBalance(buyerBalance);
            _client->GetPacketSender()->SendTokenNumPacket(_client->GetPlayerInfo().m_tokenNum);
            CUserMgr::GetInstance()->PublishBalance(request.sellerName, sellerBalance);

            CPacketMgr::GetInstance()->RecordTransaction(_client, -static_cast<int>(request.buyPrice));
        };
    });
}

void CPacketMgr::LoginCompletePacket(BASE_PACKET* _packet, CClient* _client)
{
    for (int id : _client->GetViewList()->GetView())
    {
        CUserMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendModelCustomizePacket(_client->GetID(), _client->GetModelCustomize());
    }
}

void CPacketMgr::RTTPacket(BASE_PACKET* _packet, CClient* _client)
{
    CS_RTT_PACKET* p = reinterpret_cast<CS_RTT_PACKET*>(_packet);

    long long currentTime = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    long long packetDelayTime = (currentTime - p->serverTime) / 2;
    long long timeDifference = p->time - (p->serverTime + packetDelayTime);
    _client->GetPacketSender()->SetTimeDifference(timeDifference);
}

void CPacketMgr::ChangeServerPacket(BASE_PACKET* _packet, CClient* _client)
{
    CS_TEST_CHANGE_SERVER_PACKET* p = reinterpret_cast<CS_TEST_CHANGE_SERVER_PACKET*>(_packet);
    if (p->boss)
    {
        m_matchId = 3;
    }
    LG_MATCH_PACKET lgp = {sizeof(LG_MATCH_PACKET), LG_MATCH_PLAYER, "", static_cast<short>(m_matchNum), m_matchId};
    memcpy_s(lgp.name, NAME_SIZE, _client->GetName(), NAME_SIZE);
    network::GetInstance()->GetGameServer()->Send(&lgp);
    _client->GetPacketSender()->SendServerChangePacket(m_matchId);
    _client->Disconnect();
}

void CPacketMgr::Authenticate(BASE_PACKET* _packet, CClient* _client, bool _signUp)
{
    const auto* login = reinterpret_cast<const CS_LOGIN_PACKET*>(_packet);
    const auto* signup = reinterpret_cast<const CS_SIGN_UP_PACKET*>(_packet);
    const std::string name(_signUp ? signup->name : login->name);
    const std::string password(_signUp ? signup->password : login->password);
    if (!CUserMgr::GetInstance()->ClaimAccount(name, _client))
    {
        _client->GetPacketSender()->SendLoginFailPacket(_signUp ? 2 : 3);
        return;
    }

    const auto initial = _client->GetPlayerInfo();
    _client->RunDatabase([name, password, initial, signUp = _signUp](CDataBase& _database) {
        auto info = initial;
        bool full = false;
        int error = -1;
        const bool exists = _database.CheckIdExists(name);
        _database.ThrowIfFailed();
        if (signUp && exists)
        {
            error = 2;
        }
        else if (!signUp && !exists)
        {
            error = 0;
        }
        else if (signUp && !_database.SignUp(name, password))
        {
            error = 3;
        }
        else if (!_database.GetPlayerInfo(name, password, info))
        {
            error = signUp ? 3 : 1;
        }
        else if (!_database.GetFullNodeInfo(name, full))
        {
            error = 3;
        }

        _database.ThrowIfFailed();

        return [name, info, full, error](CClient* _client) {
            if (_client->IsDisconnected())
                return;

            if (error >= 0)
            {
                CUserMgr::GetInstance()->ReleaseAccount(_client);
                _client->GetPacketSender()->SendLoginFailPacket(error);
                return;
            }

            _client->SetName(name.c_str());
            _client->SetPlayerInfo(info);
            _client->SetFullNode(full);
            _client->GetPacketSender()->SendTokenNumPacket(_client->GetPlayerInfo().m_tokenNum);
            _client->GetPacketSender()->SendFullNodePacket(full);
            CPacketMgr::GetInstance()->CompleteLogin(_client);
        };
    });
}

void CPacketMgr::CompleteLogin(CClient* _client)
{
    _client->m_stateLock.lock();
    _client->SetState(CL_STATE::ST_LOBBY);
    _client->m_stateLock.unlock();

    _client->GetPacketSender()->SendLoginInfoPacket(_client->GetID(), _client->GetTransform()->GetPos(), _client->GetModelCustomize());

    //Initialize client position
    _client->ProcessUpdate();
    _client->GetPacketSender()->SendMovePacket(_client->GetID(), _client->GetTransform()->GetPos(), _client->GetTransform()->GetDir());

    for (CClient* otherClient : CUserMgr::GetInstance()->GetChannelClients(_client->GetChannel()))
    {
        if (otherClient == nullptr)
            continue;
        if (otherClient->GetID() == _client->GetID())
            continue;
        _client->GetPacketSender()->SendModelCustomizePacket(otherClient->GetID(), otherClient->GetModelCustomize());
    }
}

void CPacketMgr::RecordTransaction(CClient* _client, int _token)
{
#ifdef WITH_DATABASE
    // 기존 블록체인 명시 활성화 경계. DB 연결 자체는 블록체인을 활성화하지 않는다.
    TransactionData transaction{};
    std::memcpy(transaction.name, _client->GetName(), NAME_SIZE);
    transaction.token = _token;
    const auto now = std::time(nullptr);
    std::tm time{};
    gmtime_s(&time, &now);
    strftime(transaction.time, sizeof(transaction.time), "%Y-%m-%d %H:%M:%S", &time);
    network::GetInstance()->GetP2PNetwork()->InsertTransaction(transaction);
#endif
}

} // namespace wod_server
