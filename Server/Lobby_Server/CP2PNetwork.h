#pragma once

#include "CBlockChain.h"

namespace wod_server {
	class CP2PNetwork
	{
	public:
		CP2PNetwork();
		~CP2PNetwork();

		std::unordered_map<std::string, int> GetPeerInfos() { m_peerInfoLock.lock(); auto temp = m_peerInfos; m_peerInfoLock.unlock(); return temp; }

		void RegisterPortNum(const std::string& _ip, int _portNum);
		void RegisterPeer(int _id, const std::string& _clientAddress);

		void InsertTransaction(const TransactionData& _transactionData);
		void SetValidatorIDs(std::vector<std::pair<std::string, int>> _validatorIDs);
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