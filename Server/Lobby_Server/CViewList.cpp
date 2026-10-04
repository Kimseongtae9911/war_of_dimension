#include "pch.h"
#include "CViewList.h"
#include "CUserMgr.h"

namespace wod_server {
	CViewList::CViewList(CClient* client)
	{
		m_myClient = client;
	}

	void CViewList::CheckViewList(int channel, int socketID, int id)
	{
		if (channel == -1)
			return;

		try {
			std::unordered_set<int> nearList;
			viewLock.lock_shared();
			std::unordered_set<int> oldList = m_viewList;
			viewLock.unlock_shared();

			for (auto& object : CUserMgr::GetInstance()->GetChannelClients(channel)) {
				if (nullptr == object)
					continue;
				object->stateLock.lock_shared();
				if (object->GetState() != CL_STATE::ST_LOBBY) {
					object->stateLock.unlock_shared();
					continue;
				}
				object->stateLock.unlock_shared();
				if (socketID == object->GetSocketID())
					continue;
				if (m_myClient->GetTransform()->CheckDistance(object->GetTransform()->GetPos())) {
					nearList.insert(object->GetSocketID());
				}
			}

			for (int socketNum : nearList) {
				CClient* nearClient = CUserMgr::GetInstance()->GetClient(socketNum);
				nearClient->GetViewList()->viewLock.lock_shared();
				if (nearClient->GetViewList()->CheckViewList(socketID) != 0) {
					// myClient move
					nearClient->GetViewList()->viewLock.unlock_shared();
					nearClient->GetPacketSender()->SendMovePacket(id, m_myClient->GetTransform()->GetPos(), m_myClient->GetTransform()->GetDir());
				}
				else {
					// myClient moved into near client's view this frame
					nearClient->GetViewList()->viewLock.unlock_shared();
					nearClient->GetViewList()->AddToView(m_myClient->GetID(), m_myClient->GetSocketID(), m_myClient->GetTransform()->GetLook(), m_myClient->GetTransform()->GetRight(), m_myClient->GetPlayerInfo().model, nearClient->GetPacketSender());
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
		viewLock.lock();
		m_viewList.clear();
		viewLock.unlock();
	}

	void CViewList::AddToView(int id, int socketNum, const vec3& look, const vec3& right, const ModelCustomize& model, CPacketSender* sendTarget)
	{
		sendTarget->SendAddPlayerPacket(id, socketNum, look, right, model);

		viewLock.lock();
		m_viewList.insert(socketNum);
		viewLock.unlock();
	}

	bool CViewList::CheckViewList(int id)
	{
		viewLock.lock_shared();
		bool check = m_viewList.contains(id);
		viewLock.unlock_shared();

		return check;
	}

	void CViewList::DeleteFromView(int id, int socketNum, CPacketSender* sendTarget)
	{
		sendTarget->SendRemovePlayerPacket(id, socketNum);

		viewLock.lock();
		m_viewList.erase(socketNum);
		viewLock.unlock();
	}

	std::unordered_set<int> CViewList::GetView()
	{
		viewLock.lock_shared();
		std::unordered_set<int> ret = m_viewList;
		viewLock.unlock_shared();
		return ret;
	}

}