#pragma once

#include "CBlockChain.h"

struct Peer
{
	SOCKET peerSocket = -1;
	sockaddr_in peerAddress = {};
};

class PeerManager
{
	SINGLETON(PeerManager)

public:
	void Initialize(int portNum);
	void Release();

	bool RegisterPeer(const std::string& ipAddress, int portNum, bool connectionCheck = true);

	bool SendMessageToPeer(Peer* peer, const std::string& message);
	bool SendMessageToPeer(Peer* peer, const CBlock* block);

	void CreateBlock(const TransactionData transactionDatas[7]);
	void CreateBlock(const string& block);

	void SetFullNode(bool fullNode) { m_fullNode = fullNode; }

private:
	void RecvFunc();
	void ProcessMessage(const std::string& senderIPAddress, int senderPortNum, const std::string& message);

private:
	std::thread m_recvThread;
	SOCKET m_recvSocket;
	
	std::vector<Peer*> m_peers;
	bool m_fullNode;

	CBlockChain* m_blockChain;
};

