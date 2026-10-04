#include "pch.h"
#include "OverlapEx.h"

namespace wod_server {

	OverlapEx::OverlapEx()
	{
		m_wsabuf.len = BUF_SIZE;
		ZeroMemory(&m_sendbuf, BUF_SIZE);
		m_wsabuf.buf = m_sendbuf;
		m_op = OP_TYPE::OP_RECV;
		ZeroMemory(&m_over, sizeof(m_over));
	}

	void OverlapEx::Intialize(BASE_PACKET* packet)
	{
		ZeroMemory(&m_over, sizeof(m_over));
		m_wsabuf.len = packet->size;
		m_wsabuf.buf = m_sendbuf;
		m_op = OP_TYPE::OP_SEND;
		memcpy(m_sendbuf, packet, packet->size);
	}

	void OverlapEx::Reset()
	{
		m_wsabuf.len = BUF_SIZE;
		m_wsabuf.buf = m_sendbuf;
		m_op = OP_TYPE::OP_RECV;
		ZeroMemory(&m_over, sizeof(m_over));
	}
}