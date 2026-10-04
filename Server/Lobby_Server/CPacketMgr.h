#pragma once

namespace wod_server {
	class CClient;

	class CPacketMgr : public TSingleton<CPacketMgr>
	{
	public:
		bool Initialize() override;
		bool Release() override;

		void Packet_Exec(BASE_PACKET* packet, CClient* client);

	private:
		void LoginPacket(BASE_PACKET* packet, CClient* client);
		void MovePacket(BASE_PACKET* packet, CClient* client);
		void MatchPacket(BASE_PACKET* packet, CClient* client);
		void ChatPacket(BASE_PACKET* packet, CClient* client);
		void RotatePacket(BASE_PACKET* packet, CClient* client);
		void CustomizePacket(BASE_PACKET* packet, CClient* client);
		void ShopPacket(BASE_PACKET* packet, CClient* client);
		void SignUpPacket(BASE_PACKET* packet, CClient* client);
		void RegisterAuctionPacket(BASE_PACKET* packet, CClient* client);
		void GetAuctionInfoPacket(BASE_PACKET* packet, CClient* client);
		void ChangeChannelPacket(BASE_PACKET* packet, CClient* client);
		void PortNumPacket(BASE_PACKET* packet, CClient* client);
		void StakeTokenPacket(BASE_PACKET* packet, CClient* client);
		void ChangeNodePacket(BASE_PACKET* packet, CClient* client);
		void DummyClientPacket(BASE_PACKET* packet, CClient* client);
		void CreateTransactionPacket(BASE_PACKET* packet, CClient* client);
		void OpenCustomizePacket(BASE_PACKET* packet, CClient* client);
		void OpenAuctionPacket(BASE_PACKET* packet, CClient* client);
		void OpenBlockChainPacket(BASE_PACKET* packet, CClient* client);
		void BuyAuctionPacket(BASE_PACKET* packet, CClient* client);
		void LoginCompletePacket(BASE_PACKET* packet, CClient* client);
		void RTTPacket(BASE_PACKET* packet, CClient* client);

		// For Test
		void ChangeServerPacket(BASE_PACKET* packet, CClient* client);

		bool GetPlayerInfo(char name[NAME_SIZE], char password[NAME_SIZE], CClient* client);

	private:
		std::unordered_map<char, std::function<void(BASE_PACKET*, CClient*)>> m_packetfunc;		

		int m_matchNum = 0;

		std::mt19937 m_randomEngine;
		std::array<int, static_cast<int>(SHOP_TYPE::COUNT)> m_shopMaxNums;

		//For Test
		std::atomic_int m_matchId = 0;
	};

}