#include "pch.h"
#include "CViewList.h"
#include "CUserMgr.h"

namespace wod_server {
	CViewList::CViewList(CClient* _client)
	{
		m_myClient = _client;
	}

	void CViewList::CheckViewList(int _channel, int _socketID, int _id)
	{
		if (_channel == -1)
			return;

		try {
			std::unordered_set<int> nearList;
			m_viewLock.lock_shared();
			std::unordered_set<int> oldList = m_viewList;
			m_viewLock.unlock_shared();

			for (auto& object : CUserMgr::GetInstance()->GetChannelClients(_channel)) {
				if (nullptr == object)
					continue;
				object->m_stateLock.lock_shared();
				if (object->GetState() != CL_STATE::ST_LOBBY) {
					object->m_stateLock.unlock_shared();
					continue;
				}
				object->m_stateLock.unlock_shared();
				if (_socketID == object->GetSocketID())
					continue;
				if (m_myClient->GetTransform()->CheckDistance(object->GetTransform()->GetPos())) {
					nearList.insert(object->GetSocketID());
				}
			}

			for (int socketNum : nearList) {
				CClient* nearClient = CUserMgr::GetInstance()->GetClient(socketNum);
				nearClient->GetViewList()->m_viewLock.lock_shared();
				if (nearClient->GetViewList()->CheckViewList(_socketID) != 0) {
					// myClient move
					nearClient->GetViewList()->m_viewLock.unlock_shared();
					nearClient->GetPacketSender()->SendMovePacket(_id, m_myClient->GetTransform()->GetPos(), m_myClient->GetTransform()->GetDir());
				}
				else {
					// myClient moved into near client's view this frame
					nearClient->GetViewList()->m_viewLock.unlock_shared();
					nearClient->GetViewList()->AddToView(m_myClient->GetID(), m_myClient->GetSocketID(), m_myClient->GetTransform()->GetLook(), m_myClient->GetTransform()->GetRight(), m_myClient->GetPlayerInfo().m_model, nearClient->GetPacketSender());
				}

				// clients which came into view this frame
				if (oldList.count(socketNum) == 0) {
					AddToView(nearClient->GetID(), socketNum, nearClient->GetTransform()->GetLook(), nearClient->GetTransform()->GetRight(), nearClient->GetModelCustomize(), m_myClient->GetPacketSender());
				}
			}

			for (int id : oldList) {
				// clients which went out of view this frame
				if (nearList.count(id) == 0) {
					DeleteFromView(CUserMgr::GetInstance()->GetClient(id)->GetID(), id, m_myClient->GetPacketSender());
				}
			}
		}
		catch (std::exception ex) {
			LogPrinter::PrintMsg("Err(Client CheckViewList), " + std::string(ex.what()));
		}
	}

	void CViewList::ClearViewList()
	{
		m_viewLock.lock();
		m_viewList.clear();
		m_viewLock.unlock();
	}

	void CViewList::AddToView(int _id, int _socketNum, const vec3& _look, const vec3& _right, const ModelCustomize& _model, CPacketSender* _sendTarget)
	{
		_sendTarget->SendAddPlayerPacket(_id, _socketNum, _look, _right, _model);

		m_viewLock.lock();
		m_viewList.insert(_socketNum);
		m_viewLock.unlock();
	}

	bool CViewList::CheckViewList(int _id)
	{
		m_viewLock.lock_shared();
		bool check = m_viewList.contains(_id);
		m_viewLock.unlock_shared();

		return check;
	}

	void CViewList::DeleteFromView(int _id, int _socketNum, CPacketSender* _sendTarget)
	{
		_sendTarget->SendRemovePlayerPacket(_id, _socketNum);

		m_viewLock.lock();
		m_viewList.erase(_socketNum);
		m_viewLock.unlock();
	}

	std::unordered_set<int> CViewList::GetView()
	{
		m_viewLock.lock_shared();
		std::unordered_set<int> ret = m_viewList;
		m_viewLock.unlock_shared();
		return ret;
	}

}