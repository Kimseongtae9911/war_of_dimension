#include "pch.h"
#include "CBlock.h"

CBlock::CBlock(const std::vector<TransactionData>& _transactions, int _version, const std::string& _prevHash)
{
	std::time_t t = time(nullptr);
	std::tm time;
	gmtime_s(&time, &t);
	char buffer[TIME_SIZE];
	strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &time);
	std::puts(buffer);

	m_timeStamp = std::string(buffer);
	m_version = _version;
	m_prevHash = _prevHash;
	m_validatorID = LOBBY_PORT;
	for (int i = 0; i < 7; ++i) {
		std::string tx = std::string(_transactions[i].name, 10) + std::to_string(_transactions[i].token) + std::string(_transactions[i].time, 20);
		m_transactions.push_back(SHA256::Encrpyt(tx));
	}
	CalculateMerkleRoot();
	CalculateHash();
}

CBlock::CBlock(const std::string& _hash, int _version, const std::string& _timeStamp, const std::string& _prevHash, const std::string& _merkleRoot, int _validatorID, std::vector<std::string>& _transactions)
{
	m_hash = _hash;
	m_version = _version;
	m_timeStamp = _timeStamp;
	m_prevHash = _prevHash;
	m_merkleRoot = _merkleRoot;
	m_validatorID = _validatorID;

	m_transactions = std::move(_transactions);
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

bool CBlock::CreateBlock(const std::string& _blockData)
{
	if (sizeof(_blockData) != BLOCK_SIZE) {
		return false;
	}

	m_hash = _blockData.substr(0, SHA256::m_HASHSIZE);
	m_version = *reinterpret_cast<const int*>(_blockData.data() + SHA256::m_HASHSIZE);
	m_timeStamp = _blockData.substr(36, 20);
	m_prevHash = _blockData.substr(56, SHA256::m_HASHSIZE);
	m_merkleRoot = _blockData.substr(88, SHA256::m_HASHSIZE);
	m_validatorID = *reinterpret_cast<const int*>(_blockData.data() + 120);

	if (!m_transactions.empty())
		m_transactions.clear();

	for (int i = 0; i < 7; ++i) {
		std::string tx = _blockData.substr(124 + i * SHA256::m_HASHSIZE, SHA256::m_HASHSIZE);
		m_transactions.push_back(tx);
	}

	return true;
}

bool CBlock::CheckBlockValidity(const std::string& _prevBlockHash, int _validatorID)
{
	//Calculate Hash
	if (m_hash != SHA256::Encrpyt(std::to_string(m_version) + m_timeStamp + m_prevHash + m_merkleRoot + std::to_string(m_validatorID))) {
		return false;
	}

	//Check Previous Block
	if (m_prevHash != _prevBlockHash)
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
	if (m_validatorID != _validatorID)
		return false;

	return true;
}
