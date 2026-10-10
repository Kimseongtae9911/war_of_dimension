#include "pch.h"
#include "CPacketSender.h"
#include "CUserMgr.h"
#include "CNetworkMgr.h"

namespace wod_server {

	CPacketSender::CPacketSender()
	{
		m_session = std::make_shared<Session>();
	}

	void CPacketSender::Initailize(const SOCKET& _socket)
	{
		m_session->SetRemainData(0);
		m_session->SetSocket(_socket);
	}

	void CPacketSender::Reset()
	{
		m_session->SetRemainData(0);
	}

	void CPacketSender::SendLoginInfoPacket(int _id, const vec3& _pos, const ModelCustomize& _modelCustomize) const
	{
		SC_LOGIN_INFO_PACKET p;
		p.size = sizeof(p);
		p.type = SC_LOGIN_INFO;
		p.id = _id % LOBBY_MAX_CLIENT;
		p.x = _pos.m_x; p.y = _pos.m_y; p.z = _pos.m_z;
		p.model = _modelCustomize;
		m_session->Send(&p);
	}

	void CPacketSender::SendLoginFailPacket(char _reason) const
	{
		SC_LOGIN_FAIL_PACKET p;
		p.size = sizeof(p);
		p.type = SC_LOGIN_FAIL;
		p.reason = _reason;
		m_session->Send(&p);
	}

	void CPacketSender::SendMovePacket(int _id, const vec3& _pos, char _dir) const
	{
		SC_MOVE_PLAYER_PACKET p;
		p.size = sizeof(p);
		p.type = SC_MOVE_PLAYER;
		p.x = _pos.m_x;
		p.y = _pos.m_y;
		p.z = _pos.m_z;
		p.id = _id % LOBBY_MAX_CLIENT;
		p.direction = _dir;
		p.move_time = std::chrono::high_resolution_clock::now().time_since_epoch().count() + m_clientTimeDifference;
		m_session->Send(&p);
	}

	void CPacketSender::SendRotatePacket(int _id, const vec3& _look, const vec3& _right) const
	{
		SC_ROTATE_PLAYER_PACKET p;
		p.size = sizeof(SC_ROTATE_PLAYER_PACKET);
		p.type = SC_ROTATE_PLAYER;
		p.id = _id % LOBBY_MAX_CLIENT;
		p.lookX = _look.m_x; p.lookY = _look.m_y; p.lookZ = _look.m_z;
		p.rightX = _right.m_x; p.rightY = _right.m_y; p.rightZ = _right.m_z;
		m_session->Send(&p);
	}

	void CPacketSender::SendAddPlayerPacket(int _id, int _socketNum, const vec3& _look, const vec3& _right, const ModelCustomize& _model)
	{
		SC_ADD_PLAYER_PACKET p;
		p.size = sizeof(SC_ADD_PLAYER_PACKET);
		p.type = SC_ADD_PLAYER;
		p.id = _id % LOBBY_MAX_CLIENT;
		p.lookX = _look.m_x; p.lookY = _look.m_y; p.lookZ = _look.m_z;
		p.rightX = _right.m_x; p.rightY = _right.m_y; p.rightZ = _right.m_z;
		p.model = _model;

		m_session->Send(&p);
	}

	void CPacketSender::SendRemovePlayerPacket(int _id, int _socketNum)
	{
		SC_REMOVE_PLAYER_PACKET p;
		p.size = sizeof(SC_REMOVE_PLAYER_PACKET);
		p.type = SC_REMOVE_PLAYER;
		p.id = _id % LOBBY_MAX_CLIENT;

		m_session->Send(&p);
	}

	void CPacketSender::SendMatchPacket(int _id)
	{
		SC_MATCH_PACKET p;
		p.size = sizeof(SC_MATCH_PACKET);
		p.type = SC_MATCH_PLAYER;
		p.id = _id;
		memset(p.gameip, 0, INET_ADDRSTRLEN);
		memcpy_s(p.gameip, INET_ADDRSTRLEN, network::GetInstance()->m_gameIP.c_str(), INET_ADDRSTRLEN);
		p.gameport = GAME_PORT;
		m_session->Send(&p);
	}

	void CPacketSender::SendModelCustomizePacket(int _id, const ModelCustomize& _model) const
	{
		SC_MODEL_CUSTOMIZE_PACKET p;
		p.size = sizeof(SC_MODEL_CUSTOMIZE_PACKET);
		p.type = SC_MODEL_CUSTOMIZE;
		p.id = _id % LOBBY_MAX_CLIENT;
		p.model = _model;
		m_session->Send(&p);
	}

	void CPacketSender::SendShopPacket(int _customizePart, int _customizeDetail) const
	{
		SC_SHOP_PACKET p;
		p.size = sizeof(SC_SHOP_PACKET);
		p.type = SC_SHOP;
		p.customizePart = _customizePart;
		p.customizeDetail = _customizeDetail;
		m_session->Send(&p);
	}

	void CPacketSender::SendAuctionInfoPacket(const std::vector<AuctionInfo> _auctionInfos)
	{
		SC_AUCTION_INFO_PACKET p;
		p.size = sizeof(SC_AUCTION_INFO_PACKET);
		p.type = SC_AUCTION_INFO;

		for (int i = 0; i < _auctionInfos.size(); ++i) {
			p.auctionInfos[i] = _auctionInfos[i];
			for (int j = 0; j < NAME_SIZE; ++j) {
				if (p.auctionInfos[i].playerName[j] == ' ') {
					p.auctionInfos[i].playerName[j] = '\0';
				}
			}
		}

		if (_auctionInfos.size() != 7) {
			for (int i = static_cast<int>(_auctionInfos.size()); i < 7; ++i) {
				p.auctionInfos[i].customizeNum = 0;
				p.auctionInfos[i].customizeType = 0;
				p.auctionInfos[i].buyPrice = 0;
				memset(p.auctionInfos[i].deadLine, 0, TIME_SIZE);
				memset(p.auctionInfos[i].playerName, 0, NAME_SIZE);
			}
		}

		m_session->Send(&p);
	}

	void CPacketSender::SendStakeTokenPacket(bool _fail, char _reason)
	{
		SC_STAKE_TOKEN_PACKET p;
		p.size = sizeof(p);
		p.type = SC_STAKE_TOKEN;
		p.fail = _fail;
		p.reason = _reason;
		m_session->Send(&p);
	}

	void CPacketSender::SendTokenNumPacket(int _tokenNum)
	{
		SC_TOKEN_NUM_PACKET p;
		p.size = sizeof(p);
		p.type = SC_TOKEN_NUM;
		p.tokenNum = _tokenNum;
		m_session->Send(&p);
	}

	void CPacketSender::SendCustomizePartsPacket(short _partType[CUSTOMIZE_PART_NUM_FROM_SERVER], short _customizeNum[CUSTOMIZE_PART_NUM_FROM_SERVER], short _count[CUSTOMIZE_PART_NUM_FROM_SERVER]) const
	{
		SC_CUSTOMIZE_PARTS_PACKET p;
		p.size = sizeof(p);
		p.type = SC_CUSTOMIZE_PARTS;
		for (int i = 0; i < CUSTOMIZE_PART_NUM_FROM_SERVER; ++i) {
			p.partType[i] = static_cast<unsigned char>(_partType[i]);
			p.customizeNum[i] = static_cast<unsigned char>(_customizeNum[i]);
			p.count[i] = static_cast<unsigned char>(_count[i]);
		}
		m_session->Send(&p);
	}

	void CPacketSender::SendAuctionPartsNumPacket(int _num)
	{
		SC_AUCTION_PARTS_NUM_PACKET p;
		p.size = sizeof(p);
		p.type = SC_AUCTION_PARTS_NUM;
		p.num = _num;
		m_session->Send(&p);
	}

	void CPacketSender::SendStakedTokenInfoPacket(int _stakedToken, int _remainingDay) const
	{
		SC_STAKE_TOKEN_INFO_PACKET p;
		p.size = sizeof(p);
		p.type = SC_STAKE_TOKEN_INFO;
		p.stakedToken = _stakedToken;
		p.remainingDay = _remainingDay;
		m_session->Send(&p);
	}

	void CPacketSender::SendChangeChannelPacket(bool _fail, int _channel)
	{
		SC_CHANGE_CHANNEL_PACKET p;
		p.size = sizeof(SC_CHANGE_CHANNEL_PACKET);
		p.type = SC_CHANGE_CHANNEL;
		p.fail = _fail;
		p.channel = _channel;
		m_session->Send(&p);
	}

	void CPacketSender::SendPeerInfoPacket(std::string _ip, int _portNum)
	{
		SC_PEER_INFO_PACKET p;
		p.size = sizeof(p);
		p.type = SC_PEER_INFO;
		memcpy_s(p.peerIP, INET_ADDRSTRLEN, _ip.c_str(), INET_ADDRSTRLEN);
		p.peerPortNum = _portNum;
		m_session->Send(&p);
	}

	void CPacketSender::SendChangeNodePacket(bool _fail, char _reason)
	{
		SC_CHANGE_NODE_PACKET p;
		p.size = sizeof(p);
		p.type = SC_CHANGE_NODE;
		p.fail = _fail;
		p.reason = _reason;
		m_session->Send(&p);
	}

	void CPacketSender::SendTransactionPacket(std::vector<TransactionData> _transactions)
	{
		SC_TRANSACTION_PACKET p;
		p.size = sizeof(p);
		p.type = SC_TRANSACTION;

		for (int i = 0; i < _transactions.size(); ++i) {
			p.transactions[i] = _transactions[i];
		}
		m_session->Send(&p);
	}

	void CPacketSender::SendBlockHeaderPacket(const std::string& _hash, int _version, const std::string& _timeStamp, const std::string& _prevHash, const std::string& _merkleRoot, int _validatorID)
	{
		SC_BLOCK_HEADER_PACKET p;
		p.size = sizeof(p);
		p.type = SC_BLOCK_HEADER;
		memcpy_s(p.hash, 65, _hash.c_str(), _hash.size());
		p.version = _version;
		memcpy_s(p.timeStamp, 21, _timeStamp.c_str(), _timeStamp.size());
		memcpy_s(p.prevHash, 65, _prevHash.c_str(), _prevHash.size());
		memcpy_s(p.merkleRoot, 65, _merkleRoot.c_str(), _merkleRoot.size());
		p.validatorID = _validatorID;
		m_session->Send(&p);
	}

	void CPacketSender::SendBlockBodyPacket(const std::vector<std::string>& _transactions, int _index)
	{
		SC_BLOCK_BODY_PACKET p;
		p.size = sizeof(p);
		p.type = SC_BLOCK_BODY;
		memcpy_s(p.transaction, 65, _transactions[_index].c_str(), _transactions[_index].size());
		m_session->Send(&p);
	}

	void CPacketSender::SendFullNodePacket(bool _isFullNode)
	{
		SC_FULLNODE_PACKET p;
		p.size = sizeof(p);
		p.type = SC_FULLNODE;
		p.fullNode = _isFullNode;
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

	void CPacketSender::SendServerChangePacket(int _id)
	{
		SC_TEST_CHANGE_SERVER_PACKET p;
		p.size = sizeof(SC_TEST_CHANGE_SERVER_PACKET);
		p.type = SC_TEST_CHANGE_SERVER;
		p.id = _id;
		memset(p.gameip, 0, INET_ADDRSTRLEN);
		memcpy_s(p.gameip, INET_ADDRSTRLEN, network::GetInstance()->m_gameIP.c_str(), INET_ADDRSTRLEN);
		p.gameport = GAME_PORT;
		m_session->Send(&p);
	}

	void CPacketSender::SendDummyLoginInfo(int _id, const vec3& _pos)
	{
		SC_DUMMY_LOGIN_INFO_PACKET p;
		p.size = sizeof(p);
		p.type = SC_DUMMY_LOGIN_INFO;
		p.id = _id;
		p.x = _pos.m_x; p.y = _pos.m_y; p.z = _pos.m_z;
		m_session->Send(&p);
	}

	void CPacketSender::SendDummyMovePacket(int _id, const vec3& _pos, char _dir)
	{
		SC_MOVE_PLAYER_PACKET p;
		p.size = sizeof(p);
		p.type = SC_MOVE_PLAYER;
		p.x = _pos.m_x; p.y = _pos.m_y; p.z = _pos.m_z;
		p.id = _id;
		p.direction = _dir;
		p.move_time = static_cast<unsigned int>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count());
		m_session->Send(&p);
	}

}