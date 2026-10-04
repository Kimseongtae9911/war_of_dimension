#pragma once

namespace test_client {

	class OverlapEx
	{
	public:
		OverlapEx();

		OverlapEx(char* packet);

		WSABUF& GetWSA() { return m_wsabuf; }
		WSAOVERLAPPED& GetOver() { return m_over; }
		const OP_TYPE GetOP() const { return m_op; }
		char* GetSendBuf() { return m_sendbuf; }
		const int GetInfo1() const { return m_info1; }
		const char* GetInfo2() const { return m_info2; }


		void SetInfo1(const int n) { m_info1 = n; }
		void SetInfo2(const char* c) { memcpy_s(m_info2, INET_ADDRSTRLEN, c, INET_ADDRSTRLEN); }
		void SetOP(const OP_TYPE op) { m_op = op; }
		void ResetOver() { ZeroMemory(&m_over, sizeof(m_over)); }

	private:
		WSAOVERLAPPED m_over;
		WSABUF m_wsabuf;
		char m_sendbuf[BUF_SIZE];
		OP_TYPE m_op;
		int m_info1;
		char m_info2[INET_ADDRSTRLEN];
	};

}