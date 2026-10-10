#pragma once

namespace wod_server {
	class CClient;

	class CPacketMgr : public TSingleton<CPacketMgr>
	{
	public:
		bool Initialize() override;
		bool Release() override;

		void Packet_Exec(BASE_PACKET* _packet, CClient* _client);

	private:
		void LoginPacket(BASE_PACKET* _packet, CClient* _client);
		void MovePacket(BASE_PACKET* _packet, CClient* _client);
		void MatchPacket(BASE_PACKET* _packet, CClient* _client);
		void ChatPacket(BASE_PACKET* _packet, CClient* _client);
		void RotatePacket(BASE_PACKET* _packet, CClient* _client);
		void CustomizePacket(BASE_PACKET* _packet, CClient* _client);
		void ShopPacket(BASE_PACKET* _packet, CClient* _client);
		void SignUpPacket(BASE_PACKET* _packet, CClient* _client);
		void RegisterAuctionPacket(BASE_PACKET* _packet, CClient* _client);
		void GetAuctionInfoPacket(BASE_PACKET* _packet, CClient* _client);
		void ChangeChannelPacket(BASE_PACKET* _packet, CClient* _client);
		void PortNumPacket(BASE_PACKET* _packet, CClient* _client);
		void StakeTokenPacket(BASE_PACKET* _packet, CClient* _client);
		void ChangeNodePacket(BASE_PACKET* _packet, CClient* _client);
		void DummyClientPacket(BASE_PACKET* _packet, CClient* _client);
		void CreateTransactionPacket(BASE_PACKET* _packet, CClient* _client);
		void OpenCustomizePacket(BASE_PACKET* _packet, CClient* _client);
		void OpenAuctionPacket(BASE_PACKET* _packet, CClient* _client);
		void OpenBlockChainPacket(BASE_PACKET* _packet, CClient* _client);
		void BuyAuctionPacket(BASE_PACKET* _packet, CClient* _client);
		void LoginCompletePacket(BASE_PACKET* _packet, CClient* _client);
		void RTTPacket(BASE_PACKET* _packet, CClient* _client);

		// For Test
		void ChangeServerPacket(BASE_PACKET* _packet, CClient* _client);

		bool GetPlayerInfo(char _name[NAME_SIZE], char _password[NAME_SIZE], CClient* _client);

	private:
		std::unordered_map<char, std::function<void(BASE_PACKET*, CClient*)>> m_packetfunc;

		int m_matchNum = 0;

		std::mt19937 m_randomEngine;
		std::array<int, static_cast<int>(SHOP_TYPE::COUNT)> m_shopMaxNums;

		//For Test
		std::atomic_int m_matchId = 0;
	};

}