#pragma once

namespace wod_server {

	class OverlapEx
	{
	public:
		OverlapEx();
		void Intialize(BASE_PACKET* packet);
		void Reset();

		WSABUF& GetWSA() { return m_wsabuf; }
		WSAOVERLAPPED& GetOver() { return m_over; }
		const OP_TYPE GetOP() const { return m_op; }
		char* GetSendBuf() { return m_sendbuf; }
		int GetSocketID() { return m_socketid; }
		int GetInfo() const { return m_info; }

		void SetOP(const OP_TYPE op) { m_op = op; }
		void SetSocketID(int id) { m_socketid = id; }
		void SetInfo(int info) { m_info = info; }

		void ResetOver() { ZeroMemory(&m_over, sizeof(m_over)); }

	private:
		WSAOVERLAPPED m_over;
		WSABUF m_wsabuf;
		char m_sendbuf[BUF_SIZE];
		OP_TYPE m_op;
		int m_socketid;
		int m_info;
	};

}