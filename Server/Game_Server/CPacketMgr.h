#pragma once

namespace wod_server {

	class CClient;

	class CPacketMgr : public TSingleton<CPacketMgr>
	{
	public:
		virtual bool Initialize() override;
		virtual bool Release() override;

		void Packet_Exec(BASE_PACKET* _packet, std::shared_ptr<CClient> _client);

	private:
		//ReadyScene
		void LoginPacket(BASE_PACKET* _packet, std::shared_ptr<CClient> _client);
		void SkillSelectPacket(BASE_PACKET* _packet, std::shared_ptr<CClient> _client);
		void ReadyPacket(BASE_PACKET* _packet, std::shared_ptr<CClient> _client);
		void JobSelectPacket(BASE_PACKET* _packet, std::shared_ptr<CClient> _client);
		void ChatPacket(BASE_PACKET* _packet, std::shared_ptr<CClient> _client);
		void StatSelectPacket(BASE_PACKET* _packet, std::shared_ptr<CClient> _client);
		void LoadCompletePacket(BASE_PACKET* _packet, std::shared_ptr<CClient> _client);

		//GameScene
		void MovePacket(BASE_PACKET* _packet, std::shared_ptr<CClient> _client);
		void RotatePacket(BASE_PACKET* _packet, std::shared_ptr<CClient> _client);
		void SkillPacket(BASE_PACKET* _packet, std::shared_ptr<CClient> _client);
		void SkillFinishPacket(BASE_PACKET* _packet, std::shared_ptr<CClient> _client);
		void JumpPacket(BASE_PACKET* _packet, std::shared_ptr<CClient> _client);
		void TeleportPacket(BASE_PACKET* _packet, std::shared_ptr<CClient> _client);
		void TowerActivatePacket(BASE_PACKET* _packet, std::shared_ptr<CClient> _client);
		void MinionPathPacket(BASE_PACKET* _packet, std::shared_ptr<CClient> _client);
		void NpcAttackFinishPacket(BASE_PACKET* _packet, std::shared_ptr<CClient> _client);
		void BuyItemPacket(BASE_PACKET* _packet, std::shared_ptr<CClient> _client);
		void BuyStatPacket(BASE_PACKET* _packet, std::shared_ptr<CClient> _client);
		void UseItemPacket(BASE_PACKET* _packet, std::shared_ptr<CClient> _client);
		void DebugGoldPacket(BASE_PACKET* _packet, std::shared_ptr<CClient> _client);
		void RTTPacket(BASE_PACKET* _packet, std::shared_ptr<CClient> _client);

		void TestIngamePacket(BASE_PACKET* _packet, std::shared_ptr<CClient> _client);
		void TestIngamePacket2(BASE_PACKET* _packet, std::shared_ptr<CClient> _client);

		//Dummy
		void DummyClientPacket(BASE_PACKET* _packet, std::shared_ptr<CClient> _client);

	private:
		std::unordered_map<char, std::function<void(BASE_PACKET*, std::shared_ptr<CClient>)>> m_packetfunc;

		//Test
		bool m_testOnce = false;

		//Dummy
		std::atomic_int m_matchNum = 0;
		std::atomic_int m_dummyNums = 0;
	};

}