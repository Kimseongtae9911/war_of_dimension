#pragma once

namespace wod_server {
	class CClient;
	class CPacketSender;

	class CViewList
	{
	public:
		CViewList() {}
		CViewList(CClient* client);
		~CViewList() {}

		void CheckViewList(int channel, int socketID, int id);
		void ClearViewList();
		void AddToView(int id, int socketNum, const vec3& look, const vec3& right, const ModelCustomize& model, CPacketSender* sendTarget);
		void DeleteFromView(int id, int socketNum, CPacketSender* sendTarget);
		std::unordered_set<int> GetView();

		std::shared_mutex viewLock;

	private:
		bool CheckViewList(int id);

	private:
		std::unordered_set<int> m_viewList;
		CClient* m_myClient;
	};
}