#include "pch.h"
#include "CBlock.h"

CBlock::CBlock(const std::vector<TransactionData>& transactions, int version, const std::string& prevHash)
{
	std::time_t t = time(nullptr);
	std::tm time;
	gmtime_s(&time, &t);
	char buffer[TIME_SIZE];
	strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &time);
	std::puts(buffer);

	m_timeStamp = std::string(buffer);
	m_version = version;
	m_prevHash = prevHash;
	m_validatorID = LOBBY_PORT;
	for (int i = 0; i < 7; ++i) {
		std::string tx = std::string(transactions[i].name, 10) + std::to_string(transactions[i].token) + std::string(transactions[i].time, 20);
		m_transactions.push_back(SHA256::Encrpyt(tx));
	}
	CalculateMerkleRoot();
	CalculateHash();
}

CBlock::CBlock(const std::string& hash, int version, const std::string& timeStamp, const std::string& prevHash, const std::string& merkleRoot, int validatorID, std::vector<std::string>& transactions)
{
	m_hash = hash;
	m_version = version;
	m_timeStamp = timeStamp;
	m_prevHash = prevHash;
	m_merkleRoot = merkleRoot;
	m_validatorID = validatorID;

	m_transactions = std::move(transactions);
}

void CBlock::CreateGenesisBlock()
{
	std::time_t t = std::time(nullptr);
	std::tm time;
	gmtime_s(&time, &t);
	char buffer[TIME_SIZE + 5];
	strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &time);
	std::puts(buffer);

	m_version = 1;
	m_timeStamp = std::string(buffer, TIME_SIZE);
	m_prevHash = "";
	m_merkleRoot = SHA256::Encrpyt("");
	m_validatorID = 0;
	CalculateHash();
}

bool CBlock::CreateBlock(const std::string& blockData)
{
	if (sizeof(blockData) != BLOCK_SIZE) {
		return false;
	}

	m_hash = blockData.substr(0, SHA256::HASHSIZE);
	m_version = *reinterpret_cast<const int*>(blockData.data() + SHA256::HASHSIZE);
	m_timeStamp = blockData.substr(36, 20);
	m_prevHash = blockData.substr(56, SHA256::HASHSIZE);
	m_merkleRoot = blockData.substr(88, SHA256::HASHSIZE);
	m_validatorID = *reinterpret_cast<const int*>(blockData.data() + 120);

	if (!m_transactions.empty())
		m_transactions.clear();

	for (int i = 0; i < 7; ++i) {
		std::string tx = blockData.substr(124 + i * SHA256::HASHSIZE, SHA256::HASHSIZE);
		m_transactions.push_back(tx);
	}

	return true;
}

bool CBlock::CheckBlockValidity(const std::string& prevBlockHash, int validatorID)
{
	//Calculate Hash
	if (m_hash != SHA256::Encrpyt(std::to_string(m_version) + m_timeStamp + m_prevHash + m_merkleRoot + std::to_string(m_validatorID))) {
		return false;
	}

	//Check Previous Block
	if (m_prevHash != prevBlockHash)
		return false;

	//Check Merkle Root
	std::string txs;
	for (const std::string& tx : m_transactions) {
		txs += tx;
	}
	m_merkleRoot = SHA256::Encrpyt(txs);

	//Check Time
	std::time_t t = time(nullptr);
	std::tm time;
	gmtime_s(&time, &t);

	std::chrono::system_clock::time_point currentTime = std::chrono::system_clock::from_time_t(std::mktime(&time));
	std::istringstream ss(m_timeStamp);
	std::tm tm;
	ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
	std::chrono::system_clock::time_point blockTimePoint = std::chrono::system_clock::from_time_t(std::mktime(&tm));

	if (blockTimePoint > currentTime) {
		return false;
	}

	//Check Validator ID
	if (m_validatorID != validatorID)
		return false;

	return true;
}
