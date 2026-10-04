#pragma once

namespace wod_server {

	class CClient;

	class CPacketMgr : public TSingleton<CPacketMgr>
	{
	public:
		virtual bool Initialize() override;
		virtual bool Release() override;

		void Packet_Exec(BASE_PACKET* packet, std::shared_ptr<CClient> client);

	private:
		//ReadyScene
		void LoginPacket(BASE_PACKET* packet, std::shared_ptr<CClient> client);
		void SkillSelectPacket(BASE_PACKET* packet, std::shared_ptr<CClient> client);
		void ReadyPacket(BASE_PACKET* packet, std::shared_ptr<CClient> client);
		void JobSelectPacket(BASE_PACKET* packet, std::shared_ptr<CClient> client);
		void ChatPacket(BASE_PACKET* packet, std::shared_ptr<CClient> client);
		void StatSelectPacket(BASE_PACKET* packet, std::shared_ptr<CClient> client);
		void LoadCompletePacket(BASE_PACKET* packet, std::shared_ptr<CClient> client);

		//GameScene
		void MovePacket(BASE_PACKET* packet, std::shared_ptr<CClient> client);		
		void RotatePacket(BASE_PACKET* packet, std::shared_ptr<CClient> client);
		void SkillPacket(BASE_PACKET* packet, std::shared_ptr<CClient> client);
		void SkillFinishPacket(BASE_PACKET* packet, std::shared_ptr<CClient> client);
		void JumpPacket(BASE_PACKET* packet, std::shared_ptr<CClient> client);
		void TeleportPacket(BASE_PACKET* packet, std::shared_ptr<CClient> client);
		void TowerActivatePacket(BASE_PACKET* packet, std::shared_ptr<CClient> client);
		void MinionPathPacket(BASE_PACKET* packet, std::shared_ptr<CClient> client);
		void NpcAttackFinishPacket(BASE_PACKET* packet, std::shared_ptr<CClient> client);
		void BuyItemPacket(BASE_PACKET* packet, std::shared_ptr<CClient> client);
		void BuyStatPacket(BASE_PACKET* packet, std::shared_ptr<CClient> client);
		void UseItemPacket(BASE_PACKET* packet, std::shared_ptr<CClient> client);
		void DebugGoldPacket(BASE_PACKET* packet, std::shared_ptr<CClient> client);
		void RTTPacket(BASE_PACKET* packet, std::shared_ptr<CClient> client);

		void TestIngamePacket(BASE_PACKET* packet, std::shared_ptr<CClient> client);
		void TestIngamePacket2(BASE_PACKET* packet, std::shared_ptr<CClient> client);

		//Dummy
		void DummyClientPacket(BASE_PACKET* packet, std::shared_ptr<CClient> client);

	private:
		std::unordered_map<char, std::function<void(BASE_PACKET*, std::shared_ptr<CClient>)>> m_packetfunc;

		//Test
		bool testOnce = false;

		//Dummy
		std::atomic_int m_matchNum = 0;
		std::atomic_int m_dummyNums = 0;
	};

}