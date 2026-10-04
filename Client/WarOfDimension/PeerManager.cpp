#include "stdafx.h"
#include "PeerManager.h"

const std::string CONNECTION_CHECK_MSG = "Connection Check";

std::unique_ptr<PeerManager> PeerManager::m_instance;

void PeerManager::Initialize(int portNum)
{
	m_peers.reserve(LOBBY_MAX_CLIENT);

	m_recvSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if (INVALID_SOCKET == m_recvSocket) {
		cout << "Faild to Create RecvSocket" << endl;
		return;
	}

	sockaddr_in receiveAddress{};
	receiveAddress.sin_family = AF_INET;
	receiveAddress.sin_port = htons(portNum);
	receiveAddress.sin_addr.s_addr = INADDR_ANY;

	if (::bind(m_recvSocket, reinterpret_cast<sockaddr*>(&receiveAddress), sizeof(receiveAddress)) == -1) {
		std::cout << "Failed to Bind Receive Socket." << std::endl;
		closesocket(m_recvSocket);
		return;
	}

	m_blockChain = new CBlockChain();

	m_recvThread = std::thread(&PeerManager::RecvFunc, this);
}

void PeerManager::Release()
{
	if (m_recvThread.joinable()) {
		m_recvThread.join();
	}

	if(m_recvSocket)
		closesocket(m_recvSocket);

	for (Peer* peer : m_peers)
		delete peer;
	m_peers.clear();

	delete m_blockChain;
}

bool PeerManager::RegisterPeer(const std::string& ipAddress, int portNum, bool connectionCheck)
{
	Peer* peer = new Peer;

	peer->peerSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if (INVALID_SOCKET == peer->peerSocket) {
		delete peer;
		return false;
	}

	
	peer->peerAddress.sin_family = AF_INET;
	peer->peerAddress.sin_port = htons(portNum);
	if (1 != inet_pton(AF_INET, ipAddress.c_str(), &(peer->peerAddress.sin_addr))) {
		delete peer;
		return false;
	}

	if (connectionCheck) {
		if (!SendMessageToPeer(peer, CONNECTION_CHECK_MSG)) {
			delete peer;
			return false;
		}
		m_peers.push_back(peer);
		cout << "New Peer Coneected" << endl;
	}
	else {
		delete peer;
	}

	return true;
}

bool PeerManager::SendMessageToPeer(Peer* peer, const std::string& message)
{
	if (sendto(peer->peerSocket, message.c_str(), static_cast<int>(message.size()), 0, reinterpret_cast<sockaddr*>(&peer->peerAddress), sizeof(peer->peerAddress)) == -1) {
		closesocket(peer->peerSocket);
		return false;
	}
	return true;
}

bool PeerManager::SendMessageToPeer(Peer* peer, const CBlock* block)
{
	string message = "";
	message += block->GetHash();
	message += block->GetVersion();
	message += string(block->GetTimeStamp(), sizeof(int));
	message += block->GetPrevHash();
	message += block->GetMerkleRoot();
	message += string(block->GetValidatorID(), sizeof(int));
	for (int i = 0; i < 7; ++i) {
		message += block->GetTransactions()[i];
	}

	if (sendto(peer->peerSocket, message.c_str(), static_cast<int>(message.size()), 0, reinterpret_cast<sockaddr*>(&peer->peerAddress), sizeof(peer->peerAddress)) == -1) {
		closesocket(peer->peerSocket);
		return false;
	}
	return true;
}

void PeerManager::CreateBlock(const TransactionData transactionDatas[7])
{
	//Create Block with 7 transactions
	CBlock* block = new CBlock();
	if (!block->CreateBlock(transactionDatas, m_blockChain->GetLastBlockHash())) {
		cout << "Failed To Create Block, Transaction Data Is Wrong" << endl;
		delete block;
		return;
	}

	m_blockChain->AddBlock(block);
	m_blockChain->SaveChainDataToHTML("BlockChain/HHS.html");

	//Send the block to other peers
	for (Peer* peer : m_peers) {
		SendMessageToPeer(peer, block);
	}
}

void PeerManager::CreateBlock(const string& blockData)
{
	//Block From Lobby, No Validtors, Assume the block is correct
	CBlock* block = new CBlock();
	if (!block->CreateBlock(blockData)) {
		cout << "Failed To Create Block" << endl;
		delete block;
		return;
	}

	m_blockChain->AddBlock(block);
	m_blockChain->SaveChainDataToHTML("BlockChain/HHS.html");
}

void PeerManager::RecvFunc()
{
	while (true) {
		char buffer[BLOCK_SIZE * 2];
		memset(buffer, 0, sizeof(buffer));

		sockaddr_in senderAddress{};
		int senderAddressLength = sizeof(senderAddress);
		int recvByte = recvfrom(m_recvSocket, buffer, sizeof(buffer) - 1, 0, reinterpret_cast<sockaddr*>(&senderAddress), &senderAddressLength);
		if (recvByte == -1) {
			cout << "Recv From Peer Failed" << endl;
			continue;
		}

		char senderIPAddress[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &(senderAddress.sin_addr), senderIPAddress, INET_ADDRSTRLEN);

		ProcessMessage(senderIPAddress, ntohs(senderAddress.sin_port), std::string(buffer, recvByte));
	}
}

void PeerManager::ProcessMessage(const std::string& senderIPAddress, int senderPortNum, const std::string& message)
{
	if (message == CONNECTION_CHECK_MSG) {
		RegisterPeer(senderIPAddress, senderPortNum, false);
	}
	else if (sizeof(message) == BLOCK_SIZE) {
		//Recv Block
		CBlock* block = new CBlock();
		if (!block->CreateBlock(message)) {
			delete block;
		}
		else {
			if (m_fullNode) {
				//Check Validity If the Peer is FullNode
				if (block->CheckBlockValidity(m_blockChain->GetLastBlockHash(), senderPortNum)) {
					//Add to BlockChain
					m_blockChain->AddBlock(block);
					if (m_blockChain->CheckChainValidity()) {
						m_blockChain->SaveChainDataToHTML("BlockChain/HHS.html");
					}
					else {
						//BlockChain Not Valid
						m_blockChain->DeleteLastBlock();
						delete block;
					}
				}
				else {
					//Wrong Block
					delete block;
				}
			}
			else {
				m_blockChain->AddBlock(block);
				m_blockChain->SaveChainDataToHTML("BlockChain/HHS.html");
			}
		}
	}
	else {
		cout << "Invalid Message" << endl;
	}
}
