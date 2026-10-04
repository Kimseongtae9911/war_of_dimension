#include "pch.h"
#include "CP2PNetwork.h"
#include "CUserMgr.h"
#include "CNetworkMgr.h"

namespace wod_server {

	CP2PNetwork::CP2PNetwork()
	{
		m_blockChain = new CBlockChain();
	}

	CP2PNetwork::~CP2PNetwork()
	{
	}

	void CP2PNetwork::RegisterPortNum(const std::string& ip, int portNum)
	{
		m_peerInfoLock.lock();
		if (!m_peerInfos.contains(ip)) {
			LogPrinter::PrintMsg("No Peer Info");
		}
		else {
			m_peerInfos[ip] = portNum;
		}
		m_peerInfoLock.unlock();
	}

	void CP2PNetwork::RegisterPeer(int id, const std::string& clientAddress)
	{
		m_peerInfoLock.lock();
		m_peerInfos.insert({ std::string(clientAddress), 0 });
		m_peerInfoLock.unlock();
		CUserMgr::GetInstance()->GetClient(id)->SetIP(std::string(clientAddress));
	}

	void CP2PNetwork::InsertTransaction(const TransactionData& transactionData)
	{
		m_transactionLock.lock();
		m_transactions.push_back(transactionData);

		if (m_transactions.size() == 7) {
			//int validatorID = SelectValidator();
			int validatorID = -1;
			if (validatorID == -1) {
				//No Online Validators, Lobby Server Does role as a validator
				CBlock* block = new CBlock(m_transactions, 1, m_blockChain->GetLastBlockHash());
				m_blockChain->AddBlock(block);
				m_blockChain->SaveChainDataToHTML("BlockChain/HHS.html");

				//Send to other peers
				for (const auto& pair : CUserMgr::GetInstance()->GetAllClient()) {
					if (pair.second->GetState() == CL_STATE::ST_LOBBY) {
						pair.second->GetPacketSender()->SendBlockHeaderPacket(block->GetHash(), block->GetVersion(), block->GetTimeStamp(), block->GetPrevHash(), block->GetMerkleRoot(), LOBBY_PORT);
						for (int i = 0; i < 7; ++i) {
							pair.second->GetPacketSender()->SendBlockBodyPacket(block->GetTransactions(), i);
						}
					}
				}

				std::cout << "Block Made By Lobby Server" << std::endl;
			}
			else {
				CUserMgr::GetInstance()->GetClient(validatorID)->GetPacketSender()->SendTransactionPacket(m_transactions);
			}
			m_transactions.clear();
		}
		m_transactionLock.unlock();
	}

	void CP2PNetwork::SetValidatorIDs(std::vector<std::pair<std::string, int>> validatorIDs)
	{
		if (validatorIDs.empty())
			return;

		m_validatorIDLock.lock();
		m_validatorIDs.clear();
		m_validatorIDs = std::move(validatorIDs);
		m_validatorIDLock.unlock();
	}

	int CP2PNetwork::SelectValidator()
	{
		SetValidatorIDs(network::GetInstance()->GetDataBaseThread()->GetDataBase()->GetValidatorIDs());
		
		std::unordered_map<int, std::pair<std::string, int>> onlineValidators;
		int totalStakedTokens = 0;

		for (const auto& [key, client] : CUserMgr::GetInstance()->GetAllClient()) {
			client->stateLock.lock_shared();
			if (client->GetState() != CL_STATE::ST_LOBBY) {
				client->stateLock.unlock_shared();
				continue;
			}
			else {
				client->stateLock.unlock_shared();

				for (const auto& validatorInfo : m_validatorIDs) {
					if (validatorInfo.first == client->GetName()) {
						onlineValidators.insert({ static_cast<int>(client->GetPacketSender()->GetSession()->GetSocket()), validatorInfo });
						totalStakedTokens += validatorInfo.second;
						break;
					}
				}
			}
		}

		//Choose one validator
		if (totalStakedTokens != 0) {
			int chosenTokenNum = GenerateRandomNumber(0, totalStakedTokens);
			for (const auto& validatorInfo : onlineValidators) {
				chosenTokenNum -= validatorInfo.second.second;
				if (chosenTokenNum <= 0) {
					return validatorInfo.first;
				}
			}
		}


		return -1;
	}
}