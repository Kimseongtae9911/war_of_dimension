#pragma once

#include "SHA256.h"

constexpr int BLOCK_SIZE = 348;

class CBlock
{
public:
	CBlock() {}
	CBlock(const std::vector<TransactionData>& transactions, int version, const std::string& prevHash);
	CBlock(const std::string& hash, int version, const std::string& timeStamp, const std::string& prevHash, const std::string& merkleRoot, int validatorID, std::vector<std::string>& transactions);
	~CBlock() {}

	void CreateGenesisBlock();

	void CalculateHash() {
		m_hash = SHA256::Encrpyt(std::to_string(m_version) + m_timeStamp + m_prevHash + m_merkleRoot + std::to_string(m_validatorID));
	}

	const std::string GetData() const {
		return std::to_string(m_version) + m_timeStamp + m_prevHash + m_merkleRoot + std::to_string(m_validatorID);
	}

	void CalculateMerkleRoot() {
		std::string txs;
		for (const std::string& tx : m_transactions) {
			txs += tx;
		}
		m_merkleRoot = SHA256::Encrpyt(txs);
	}

	bool CreateBlock(const std::string& blockData);
	bool CheckBlockValidity(const std::string& prevBlockHash, int validatorID);

	void SetPrevHash(const std::string& prevHash) { m_prevHash = prevHash; }

	const std::string& GetHash() const { return m_hash; }
	int GetVersion() const { return m_version; }
	const std::string& GetTimeStamp() const { return m_timeStamp; }
	const std::string& GetPrevHash() const { return m_prevHash; }
	const std::string& GetMerkleRoot() const { return m_merkleRoot; }
	int GetValidatorID() const { return m_validatorID; }
	const std::vector<std::string>& GetTransactions() const { return m_transactions; }

private:
	// Header
	std::string m_hash;
	int m_version;
	std::string m_timeStamp;
	std::string m_prevHash;
	std::string m_merkleRoot;
	int m_validatorID;

	// Body
	std::vector<std::string> m_transactions;
};

