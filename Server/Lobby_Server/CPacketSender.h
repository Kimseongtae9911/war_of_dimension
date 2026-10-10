#pragma once
#include "TCPSocket.h"

namespace wod_server {
	class CPacketSender
	{
	public:
		CPacketSender();
		~CPacketSender() {}

		void Initailize(const SOCKET& _socket);
		void Reset();

	public:
		std::shared_ptr<Session> GetSession() const { return m_session; }
		void SetTimeDifference(const long long& _time) { m_clientTimeDifference = _time; }

		void SendLoginInfoPacket(int _id, const vec3& _pos, const ModelCustomize& _modelCustomize) const;
		void SendLoginFailPacket(char _reason) const;

		void SendMovePacket(int _id, const vec3& _pos, char _dir) const;
		void SendRotatePacket(int _id, const vec3& _look, const vec3& _right) const;

		void SendAddPlayerPacket(int _id, int _socketNum, const vec3& _look, const vec3& _right, const ModelCustomize& _model);
		void SendRemovePlayerPacket(int _id, int _socketNum);

		void SendMatchPacket(int _id);

		void SendModelCustomizePacket(int _id, const ModelCustomize& _model) const;
		void SendShopPacket(int _customizePart, int _customizeDetail) const;
		void SendAuctionInfoPacket(const std::vector<AuctionInfo> _auctionInfos);
		void SendStakeTokenPacket(bool _fail, char _reason);
		void SendTokenNumPacket(int _tokenNum);
		void SendCustomizePartsPacket(short _partType[CUSTOMIZE_PART_NUM_FROM_SERVER], short _customizeNum[CUSTOMIZE_PART_NUM_FROM_SERVER], short _count[CUSTOMIZE_PART_NUM_FROM_SERVER]) const;
		void SendAuctionPartsNumPacket(int _num);
		void SendStakedTokenInfoPacket(int _stakedToken, int _remainingDay) const;

		void SendChangeChannelPacket(bool _fail, int _channel);

		void SendPeerInfoPacket(std::string _ip, int _portNum);
		void SendChangeNodePacket(bool _fail, char _reason);
		void SendTransactionPacket(std::vector<TransactionData> _transactions);
		void SendBlockHeaderPacket(const std::string& _hash, int _version, const std::string& _timeStamp, const std::string& _prevHash, const std::string& _merkleRoot, int _validatorID);
		void SendBlockBodyPacket(const std::vector<std::string>& _transactions, int _index);
		void SendFullNodePacket(bool _isFullNode);

		void SendRTTPacket() const;

		void SendServerChangePacket(int _id);

		//Dummy Client
		void SendDummyLoginInfo(int _id, const vec3& _pos);
		void SendDummyMovePacket(int _id, const vec3& _pos, char _dir);

	private:
		std::shared_ptr<Session> m_session;
		long long m_clientTimeDifference = 0;
	};

}