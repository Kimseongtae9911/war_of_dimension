#pragma once
#include <unordered_set>
#include "CPacketSender.h"
#include "CTransform.h"
#include "CPhysic.h"
#include "CViewList.h"
#include <ServerCore/SessionHandler.h>
#include <ServerCore/Concurrency.h>

namespace wod_server {

	struct PlayerInfo {
		ModelCustomize m_model;
		int m_tokenNum;

		void Reset() {
			m_tokenNum = 0;
			memset(&m_model, 0, sizeof(m_model));
		}
	};

	class CClient : public std::enable_shared_from_this<CClient>, public wod::core::SessionHandler<Session>, public wod::core::JobTarget
	{
	public:
		CClient();
		~CClient();

		void Initialize(const SOCKET& _socket);

		void Move();
		bool Reset();

	public:
		CPacketSender* GetPacketSender() const { return m_packetSender.get(); }
		CL_STATE GetState() const { return m_state; }
		const PlayerInfo& GetPlayerInfo() const { return m_playerInfo; }
		const ModelCustomize& GetModelCustomize() const { return m_playerInfo.m_model; }
		int GetSocketID() const { return m_socketID; }
		const std::string& GetIP() const { return m_ipAddress; }
		bool IsFullNode() { return m_isFullNode; }
		int GetID() const { return m_id; }
		const char* GetName() const { return m_name; }
		int GetChannel() const { return m_channel; }

		CTransform* GetTransform() { return m_transform; }
		CPhysic* GetPhysics() { return m_physics; }
		CViewList* GetViewList() { return m_viewList; }

		void SetState(CL_STATE _st) { m_state = _st; }
		void SetUpdateTime() { m_updateTime = std::chrono::system_clock::now(); }
		void SetPlayerInfo(const PlayerInfo& _info) { m_playerInfo = _info; }
		void SetTokenNum(int _num) { m_playerInfo.m_tokenNum = _num; }
		void SetModelCustomize(const ModelCustomize& _model) { m_playerInfo.m_model = _model; }
		void SetIP(const std::string& _ip) { m_ipAddress = _ip; }
		void SetFullNode(bool _fullNode) { m_isFullNode = _fullNode; }
		void SetChannel(int _channel) { m_channel = _channel; }
		void SetSection(int _x, int _z) { m_sectionX = _x; m_sectionZ = _z; }
		void SetID(const int _id) { m_id = _id; }
		void SetName(const char* _name) { memcpy_s(m_name, NAME_SIZE, _name, NAME_SIZE); }

		void ProcessUpdate(bool _isDummy = false);

		std::shared_mutex m_stateLock;

	private:
		SessionRef GetTransportSession() const override;
		bool ValidateFrame(std::span<const char> _frame) const override;
		bool DispatchFrame(Frame _frame, const SessionRef& _session, uint64_t _generation) override;
		void OnReceiveComplete(size_t _frames) override;
		void OnJobQueueDisconnected() override;

		std::unique_ptr<CPacketSender> m_packetSender;

		CL_STATE m_state;

		std::chrono::system_clock::time_point m_updateTime = {};

		int m_socketID = -1;
		PlayerInfo m_playerInfo;

		int m_id = -1;
		char m_name[NAME_SIZE] = {};

		int m_channel = -1;
		int m_sectionX = -1;
		int m_sectionZ = -1;

		CTransform* m_transform;
		CPhysic* m_physics;
		CViewList* m_viewList;

		//P2P
		std::string m_ipAddress;
		bool m_isFullNode = false;
	};
}
