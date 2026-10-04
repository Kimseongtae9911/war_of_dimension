#pragma once

#include "CBlock.h"

class CBlockChain
{
public:
	CBlockChain();
	~CBlockChain();

	void AddBlock(CBlock* newBlock);
	void DeleteLastBlock();
	bool CheckChainValidity();

	const std::string& GetLastBlockHash() { return m_chain.back()->GetHash(); }
	void SaveChainDataToHTML(const std::string& filename) const;

private:
	bool LoadChainData(const std::string& filename);	//From HTML File
	bool LoadChainData();								//From Database
	void CreateGenesisBlock();

private:
	std::vector<CBlock*> m_chain;
};

