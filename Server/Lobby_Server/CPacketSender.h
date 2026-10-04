#pragma once
#include "TCPSocket.h"

namespace wod_server {
	class CPacketSender
	{
	public:
		CPacketSender();
		~CPacketSender() {}

		void Initailize(const SOCKET& socket);
		void Reset();

	public:
		std::shared_ptr<Session> GetSession() const { return m_session; }
		void SetTimeDifference(const long long& time) { m_clientTimeDifference = time; }

		void SendLoginInfoPacket(int id, const vec3& pos, const ModelCustomize& modelCustomize) const;
		void SendLoginFailPacket(char reason) const;

		void SendMovePacket(int id, const vec3& pos, char dir) const;
		void SendRotatePacket(int id, const vec3& look, const vec3& right) const;

		void SendAddPlayerPacket(int id, int socketNum, const vec3& look, const vec3& right, const ModelCustomize& model);
		void SendRemovePlayerPacket(int id, int socketNum);

		void SendMatchPacket(int id);

		void SendModelCustomizePacket(int id, const ModelCustomize& model) const;
		void SendShopPacket(int customizePart, int customizeDetail) const;
		void SendAuctionInfoPacket(const std::vector<AuctionInfo> auctionInfos);
		void SendStakeTokenPacket(bool fail, char reason);
		void SendTokenNumPacket(int tokenNum);
		void SendCustomizePartsPacket(short partType[CUSTOMIZE_PART_NUM_FROM_SERVER], short customizeNum[CUSTOMIZE_PART_NUM_FROM_SERVER], short count[CUSTOMIZE_PART_NUM_FROM_SERVER]) const;
		void SendAuctionPartsNumPacket(int num);
		void SendStakedTokenInfoPacket(int stakedToken, int remainingDay) const;

		void SendChangeChannelPacket(bool fail, int channel);

		void SendPeerInfoPacket(std::string ip, int portNum);
		void SendChangeNodePacket(bool fail, char reason);
		void SendTransactionPacket(std::vector<TransactionData> transactions);
		void SendBlockHeaderPacket(const std::string& hash, int version, const std::string& timeStamp, const std::string& prevHash, const std::string& merkleRoot, int validatorID);
		void SendBlockBodyPacket(const std::vector<std::string>& transactions, int index);
		void SendFullNodePacket(bool isFullNode);

		void SendRTTPacket() const;

		void SendServerChangePacket(int id);

		//Dummy Client
		void SendDummyLoginInfo(int id, const vec3& pos);
		void SendDummyMovePacket(int id, const vec3& pos, char dir);

	private:
		std::shared_ptr<Session> m_session;
		long long m_clientTimeDifference = 0;
	};

}