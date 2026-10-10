#pragma once

namespace wod_server {
	class CClient;
	class CPacketSender;

	class CViewList
	{
	public:
		CViewList() {}
		CViewList(CClient* _client);
		~CViewList() {}

		void CheckViewList(int _channel, int _socketID, int _id);
		void ClearViewList();
		void AddToView(int _id, int _socketNum, const vec3& _look, const vec3& _right, const ModelCustomize& _model, CPacketSender* _sendTarget);
		void DeleteFromView(int _id, int _socketNum, CPacketSender* _sendTarget);
		std::unordered_set<int> GetView();

		std::shared_mutex m_viewLock;

	private:
		bool CheckViewList(int _id);

	private:
		std::unordered_set<int> m_viewList;
		CClient* m_myClient;
	};
}