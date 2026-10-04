#pragma once
#include "SHA256.h"

struct CBlock {
	// Header
	std::string hash;
	int version;
	std::string timeStamp;
	std::string prevHash;
	std::string merkleRoot;
	int validatorID;

	// Body
	std::vector<std::string> transactions;

	// functions
	void CalculateHash() {
		hash = SHA256::Encrpyt(std::to_string(version) + timeStamp + prevHash + merkleRoot + std::to_string(validatorID));
	}

	const std::string GetData() const {
		return std::to_string(version) + timeStamp + prevHash + merkleRoot + std::to_string(validatorID);
	}

	void CalculateMerkleRoot() {
		std::string txs;
		for (const std::string& tx : transactions) {
			txs += tx;
		}
		merkleRoot = SHA256::Encrpyt(txs);
	}
};

struct CNode {
	int id;
	int balance;
};

class CBlockChain
{
public:
	CBlockChain() { CreateGenesisBlock(); }
	~CBlockChain();
	
	void AddBlock(CBlock* newBlock);
	bool CheckChainValidity();

private:
	void CreateGenesisBlock();
	char* GetTime() const;

private:
	std::vector<CBlock*> m_chain;

};

