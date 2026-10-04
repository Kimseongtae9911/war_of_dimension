#pragma once

namespace wod_server {

	class OverlapEx
	{
	public:
		OverlapEx();

		void Intialize(BASE_PACKET* packet);

		WSABUF& GetWSA() { return m_wsabuf; }
		WSAOVERLAPPED& GetOver() { return m_over; }
		const OP_TYPE GetOP() const { return m_op; }
		char* GetSendBuf() { return m_sendbuf; }

		void SetOP(const OP_TYPE op) { m_op = op; }
		void ResetOver() { ZeroMemory(&m_over, sizeof(m_over)); }
		void Reset();

	private:
		WSAOVERLAPPED m_over;
		WSABUF m_wsabuf;
		char m_sendbuf[BUF_SIZE];
		OP_TYPE m_op;
	};

}
