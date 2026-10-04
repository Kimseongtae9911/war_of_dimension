#pragma once

class TCPSocket;
class OverlapEx;

class NetworkManager
{
	SINGLETON(NetworkManager);

public:
	void Initialize();
	void Release();

	void SendPacket(void* packet);
	void RecvPacket();

	void WorkerThread();

private:
	void Recv(int id, int bytes, OverlapEx* over_ex);
	void Send(int id, int bytes, OverlapEx* over_ex);
	void Disconnect(int id, int bytes, OverlapEx* over_ex);
	void Connect(int id, int bytes, OverlapEx* over_ex);

	void LoginInfoPacket(int id, char* packet);
	void MovePacket(int id, char* packet);
	void MatchPacket(int id, char* packet);
	void MatchEndPacket(int id, char* packet);

private:
	std::shared_ptr<TCPSocket> m_handle = nullptr;
	std::unordered_map<OP_TYPE, std::function<void(int, int, OverlapEx*)>> m_iocpfunc;
	std::unordered_map<char, std::function<void(char*, int)>> m_packetfunc;
};

