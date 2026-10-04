#pragma once

#include "CBlockChain.h"

namespace wod_server {
	class CP2PNetwork
	{
	public:
		CP2PNetwork();
		~CP2PNetwork();

		std::unordered_map<std::string, int> GetPeerInfos() { m_peerInfoLock.lock(); auto temp = m_peerInfos; m_peerInfoLock.unlock(); return temp; }

		void RegisterPortNum(const std::string& ip, int portNum);
		void RegisterPeer(int id, const std::string& clientAddress);

		void InsertTransaction(const TransactionData& transactionData);
		void SetValidatorIDs(std::vector<std::pair<std::string, int>> validatorIDs);
		int SelectValidator();

	private:
		std::unordered_map<std::string, int> m_peerInfos;
		std::mutex m_peerInfoLock;
		CBlockChain* m_blockChain;
		std::vector<std::pair<std::string, int>> m_validatorIDs;
		std::mutex m_validatorIDLock;

		std::vector<TransactionData> m_transactions;
		std::mutex m_transactionLock;
	};

}