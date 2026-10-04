#include "pch.h"
#include "CPacketSender.h"
#include "CUserMgr.h"
#include "CNetworkMgr.h"

namespace wod_server {

	CPacketSender::CPacketSender()
	{
		m_session = std::make_shared<Session>();
	}

	void CPacketSender::Initailize(const SOCKET& socket)
	{
		m_session->SetRemainData(0);
		m_session->SetSocket(socket);
	}

	void CPacketSender::Reset()
	{
		m_session->SetRemainData(0);
	}

	void CPacketSender::SendLoginInfoPacket(int id, const vec3& pos, const ModelCustomize& modelCustomize) const
	{
		SC_LOGIN_INFO_PACKET p;
		p.size = sizeof(p);
		p.type = SC_LOGIN_INFO;
		p.id = id % LOBBY_MAX_CLIENT;
		p.x = pos.x; p.y = pos.y; p.z = pos.z;
		p.model = modelCustomize;
		m_session->Send(&p);
	}

	void CPacketSender::SendLoginFailPacket(char reason) const
	{
		SC_LOGIN_FAIL_PACKET p;
		p.size = sizeof(p);
		p.type = SC_LOGIN_FAIL;
		p.reason = reason;
		m_session->Send(&p);
	}

	void CPacketSender::SendMovePacket(int id, const vec3& pos, char dir) const
	{
		SC_MOVE_PLAYER_PACKET p;
		p.size = sizeof(p);
		p.type = SC_MOVE_PLAYER;
		p.x = pos.x;
		p.y = pos.y;
		p.z = pos.z;
		p.id = id % LOBBY_MAX_CLIENT;
		p.direction = dir;
		p.move_time = std::chrono::high_resolution_clock::now().time_since_epoch().count() + m_clientTimeDifference;
		m_session->Send(&p);
	}

	void CPacketSender::SendRotatePacket(int id, const vec3& look, const vec3& right) const
	{
		SC_ROTATE_PLAYER_PACKET p;
		p.size = sizeof(SC_ROTATE_PLAYER_PACKET);
		p.type = SC_ROTATE_PLAYER;
		p.id = id % LOBBY_MAX_CLIENT;
		p.lookX = look.x; p.lookY = look.y; p.lookZ = look.z;
		p.rightX = right.x; p.rightY = right.y; p.rightZ = right.z;
		m_session->Send(&p);
	}

	void CPacketSender::SendAddPlayerPacket(int id, int socketNum, const vec3& look, const vec3& right, const ModelCustomize& model)
	{
		SC_ADD_PLAYER_PACKET p;
		p.size = sizeof(SC_ADD_PLAYER_PACKET);
		p.type = SC_ADD_PLAYER;
		p.id = id % LOBBY_MAX_CLIENT;
		p.lookX = look.x; p.lookY = look.y; p.lookZ = look.z;
		p.rightX = right.x; p.rightY = right.y; p.rightZ = right.z;
		p.model = model;

		m_session->Send(&p);
	}

	void CPacketSender::SendRemovePlayerPacket(int id, int socketNum)
	{
		SC_REMOVE_PLAYER_PACKET p;
		p.size = sizeof(SC_REMOVE_PLAYER_PACKET);
		p.type = SC_REMOVE_PLAYER;
		p.id = id % LOBBY_MAX_CLIENT;

		m_session->Send(&p);
	}

	void CPacketSender::SendMatchPacket(int id)
	{
		SC_MATCH_PACKET p;
		p.size = sizeof(SC_MATCH_PACKET);
		p.type = SC_MATCH_PLAYER;
		p.id = id;
		memset(p.gameip, 0, INET_ADDRSTRLEN);
		memcpy_s(p.gameip, INET_ADDRSTRLEN, network::GetInstance()->gameIP.c_str(), INET_ADDRSTRLEN);
		p.gameport = GAME_PORT;
		m_session->Send(&p);
	}

	void CPacketSender::SendModelCustomizePacket(int id, const ModelCustomize& model) const
	{
		SC_MODEL_CUSTOMIZE_PACKET p;
		p.size = sizeof(SC_MODEL_CUSTOMIZE_PACKET);
		p.type = SC_MODEL_CUSTOMIZE;
		p.id = id % LOBBY_MAX_CLIENT;
		p.model = model;
		m_session->Send(&p);
	}

	void CPacketSender::SendShopPacket(int customizePart, int customizeDetail) const
	{
		SC_SHOP_PACKET p;
		p.size = sizeof(SC_SHOP_PACKET);
		p.type = SC_SHOP;
		p.customizePart = customizePart;
		p.customizeDetail = customizeDetail;
		m_session->Send(&p);
	}

	void CPacketSender::SendAuctionInfoPacket(const std::vector<AuctionInfo> auctionInfos)
	{
		SC_AUCTION_INFO_PACKET p;
		p.size = sizeof(SC_AUCTION_INFO_PACKET);
		p.type = SC_AUCTION_INFO;

		for (int i = 0; i < auctionInfos.size(); ++i) {
			p.auctionInfos[i] = auctionInfos[i];
			for (int j = 0; j < NAME_SIZE; ++j) {
				if (p.auctionInfos[i].playerName[j] == ' ') {
					p.auctionInfos[i].playerName[j] = '\0';
				}
			}
		}

		if (auctionInfos.size() != 7) {
			for (int i = static_cast<int>(auctionInfos.size()); i < 7; ++i) {
				p.auctionInfos[i].customizeNum = 0;
				p.auctionInfos[i].customizeType = 0;
				p.auctionInfos[i].buyPrice = 0;
				memset(p.auctionInfos[i].deadLine, 0, TIME_SIZE);
				memset(p.auctionInfos[i].playerName, 0, NAME_SIZE);
			}
		}

		m_session->Send(&p);
	}

	void CPacketSender::SendStakeTokenPacket(bool fail, char reason)
	{
		SC_STAKE_TOKEN_PACKET p;
		p.size = sizeof(p);
		p.type = SC_STAKE_TOKEN;
		p.fail = fail;
		p.reason = reason;
		m_session->Send(&p);
	}

	void CPacketSender::SendTokenNumPacket(int tokenNum)
	{
		SC_TOKEN_NUM_PACKET p;
		p.size = sizeof(p);
		p.type = SC_TOKEN_NUM;
		p.tokenNum = tokenNum;
		m_session->Send(&p);
	}

	void CPacketSender::SendCustomizePartsPacket(short partType[CUSTOMIZE_PART_NUM_FROM_SERVER], short customizeNum[CUSTOMIZE_PART_NUM_FROM_SERVER], short count[CUSTOMIZE_PART_NUM_FROM_SERVER]) const
	{
		SC_CUSTOMIZE_PARTS_PACKET p;
		p.size = sizeof(p);
		p.type = SC_CUSTOMIZE_PARTS;
		for (int i = 0; i < CUSTOMIZE_PART_NUM_FROM_SERVER; ++i) {
			p.partType[i] = static_cast<unsigned char>(partType[i]);
			p.customizeNum[i] = static_cast<unsigned char>(customizeNum[i]);
			p.count[i] = static_cast<unsigned char>(count[i]);
		}
		m_session->Send(&p);
	}

	void CPacketSender::SendAuctionPartsNumPacket(int num)
	{
		SC_AUCTION_PARTS_NUM_PACKET p;
		p.size = sizeof(p);
		p.type = SC_AUCTION_PARTS_NUM;
		p.num = num;
		m_session->Send(&p);
	}

	void CPacketSender::SendStakedTokenInfoPacket(int stakedToken, int remainingDay) const
	{
		SC_STAKE_TOKEN_INFO_PACKET p;
		p.size = sizeof(p);
		p.type = SC_STAKE_TOKEN_INFO;
		p.stakedToken = stakedToken;
		p.remainingDay = remainingDay;
		m_session->Send(&p);
	}

	void CPacketSender::SendChangeChannelPacket(bool fail, int channel)
	{
		SC_CHANGE_CHANNEL_PACKET p;
		p.size = sizeof(SC_CHANGE_CHANNEL_PACKET);
		p.type = SC_CHANGE_CHANNEL;
		p.fail = fail;
		p.channel = channel;
		m_session->Send(&p);
	}

	void CPacketSender::SendPeerInfoPacket(std::string ip, int portNum)
	{
		SC_PEER_INFO_PACKET p;
		p.size = sizeof(p);
		p.type = SC_PEER_INFO;
		memcpy_s(p.peerIP, INET_ADDRSTRLEN, ip.c_str(), INET_ADDRSTRLEN);
		p.peerPortNum = portNum;
		m_session->Send(&p);
	}

	void CPacketSender::SendChangeNodePacket(bool fail, char reason)
	{
		SC_CHANGE_NODE_PACKET p;
		p.size = sizeof(p);
		p.type = SC_CHANGE_NODE;
		p.fail = fail;
		p.reason = reason;
		m_session->Send(&p);
	}

	void CPacketSender::SendTransactionPacket(std::vector<TransactionData> transactions)
	{
		SC_TRANSACTION_PACKET p;
		p.size = sizeof(p);
		p.type = SC_TRANSACTION;

		for (int i = 0; i < transactions.size(); ++i) {
			p.transactions[i] = transactions[i];
		}
		m_session->Send(&p);
	}

	void CPacketSender::SendBlockHeaderPacket(const std::string& hash, int version, const std::string& timeStamp, const std::string& prevHash, const std::string& merkleRoot, int validatorID)
	{
		SC_BLOCK_HEADER_PACKET p;
		p.size = sizeof(p);
		p.type = SC_BLOCK_HEADER;
		memcpy_s(p.hash, 65, hash.c_str(), hash.size());
		p.version = version;
		memcpy_s(p.timeStamp, 21, timeStamp.c_str(), timeStamp.size());
		memcpy_s(p.prevHash, 65, prevHash.c_str(), prevHash.size());
		memcpy_s(p.merkleRoot, 65, merkleRoot.c_str(), merkleRoot.size());
		p.validatorID = validatorID;
		m_session->Send(&p);
	}

	void CPacketSender::SendBlockBodyPacket(const std::vector<std::string>& transactions, int index)
	{
		SC_BLOCK_BODY_PACKET p;
		p.size = sizeof(p);
		p.type = SC_BLOCK_BODY;
		memcpy_s(p.transaction, 65, transactions[index].c_str(), transactions[index].size());
		m_session->Send(&p);
	}

	void CPacketSender::SendFullNodePacket(bool isFullNode)
	{
		SC_FULLNODE_PACKET p;
		p.size = sizeof(p);
		p.type = SC_FULLNODE;
		p.fullNode = isFullNode;
		m_session->Send(&p);
	}

	void CPacketSender::SendRTTPacket() const
	{
		SC_RTT_PACKET p;
		p.size = sizeof(p);
		p.type = SC_RTT;
		p.time = std::chrono::high_resolution_clock::now().time_since_epoch().count();
		m_session->Send(&p);
	}

	void CPacketSender::SendServerChangePacket(int id)
	{
		SC_TEST_CHANGE_SERVER_PACKET p;
		p.size = sizeof(SC_TEST_CHANGE_SERVER_PACKET);
		p.type = SC_TEST_CHANGE_SERVER;
		p.id = id;
		memset(p.gameip, 0, INET_ADDRSTRLEN);
		memcpy_s(p.gameip, INET_ADDRSTRLEN, network::GetInstance()->gameIP.c_str(), INET_ADDRSTRLEN);
		p.gameport = GAME_PORT;
		m_session->Send(&p);
	}

	void CPacketSender::SendDummyLoginInfo(int id, const vec3& pos)
	{
		SC_DUMMY_LOGIN_INFO_PACKET p;
		p.size = sizeof(p);
		p.type = SC_DUMMY_LOGIN_INFO;
		p.id = id;
		p.x = pos.x; p.y = pos.y; p.z = pos.z;
		m_session->Send(&p);
	}

	void CPacketSender::SendDummyMovePacket(int id, const vec3& pos, char dir)
	{
		SC_MOVE_PLAYER_PACKET p;
		p.size = sizeof(p);
		p.type = SC_MOVE_PLAYER;
		p.x = pos.x; p.y = pos.y; p.z = pos.z;
		p.id = id;
		p.direction = dir;
		p.move_time = static_cast<unsigned int>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count());
		m_session->Send(&p);
	}

}