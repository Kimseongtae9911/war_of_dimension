#include "pch.h"
#include "BlockChain.h"

CBlockChain::~CBlockChain()
{
	for (int i = 0; i < m_chain.size(); ++i) {
		delete m_chain[i];
	}
}

void CBlockChain::AddBlock(CBlock* newBlock)
{
	// assume that the block has other datas
	newBlock->prevHash = m_chain.back()->hash;
	m_chain.push_back(newBlock);
}

// check the transactions validity and the connection with the prev block, need to fix
bool CBlockChain::CheckChainValidity()
{
	for (int i = 1; i < m_chain.size(); ++i) {
		CBlock* curBlock = m_chain[i];
		CBlock* prevBlock = m_chain[i - 1];

		if (curBlock->hash != SHA256::Encrpyt(curBlock->GetData())) {
			return false;
		}

		if (curBlock->prevHash != prevBlock->hash) {
			return false;
		}
	}

	return true;
}

void CBlockChain::CreateGenesisBlock()
{
	CBlock* genesis = new CBlock;
	
	genesis->version = 1;
	genesis->timeStamp = GetTime();
	genesis->prevHash = "";
	genesis->merkleRoot = SHA256::Encrpyt("");
	genesis->validatorID = 0;
	genesis->CalculateHash();
	m_chain.emplace_back(genesis);
}

char* CBlockChain::GetTime() const
{
	std::time_t t = time(nullptr);
	std::tm time;
	gmtime_s(&time, &t);
	char buffer[128];
	strftime(buffer, sizeof(buffer), "%Y-%m-%d %X", &time);

	return buffer;
}
