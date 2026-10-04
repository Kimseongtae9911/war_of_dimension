#include "pch.h"
#include "CBlockChain.h"

CBlockChain::CBlockChain()
{
	if(!LoadChainData("BlockChain/HHS.html"))
		CreateGenesisBlock();
}

CBlockChain::~CBlockChain()
{
}

void CBlockChain::AddBlock(CBlock* newBlock)
{
	//newBlock->SetPrevHash(m_chain.back()->GetPrevHash());

	m_chain.push_back(newBlock);
}

void CBlockChain::DeleteLastBlock()
{
	m_chain.erase(m_chain.end());
}

bool CBlockChain::CheckChainValidity()
{
	for (int i = 1; i < m_chain.size(); ++i) {
		CBlock* curBlock = m_chain[i];
		CBlock* prevBlock = m_chain[i - 1];

		if (curBlock->GetHash() != SHA256::Encrpyt(curBlock->GetData())) {
			return false;
		}

		if (curBlock->GetPrevHash() != prevBlock->GetHash()) {
			return false;
		}
	}

	return true;
}

bool CBlockChain::LoadChainData(const std::string& filename)
{
	std::ifstream inputFile(filename);
	if (inputFile.fail()) {	
		return false;
	}

	std::string line;
	while (std::getline(inputFile, line))
	{
		if (line.find("<div class=\"block-info\"") != std::string::npos) {
			std::string hash;
			int version;
			std::string timeStamp;
			std::string prevHash;
			std::string merkleRoot;
			int validatorID;
			std::vector<std::string> transactions;

			while (std::getline(inputFile, line)) {

				size_t hashPos = line.find("<strong>Hash:</strong>");
				if (hashPos != std::string::npos) {
					size_t startPos = line.find(">", hashPos + 21) + 2;
					size_t endPos = line.find("<", startPos);
					hash = line.substr(startPos, endPos - startPos);
					continue;
				}

				size_t versionPos = line.find("<strong>Version:</strong>");
				if (versionPos != std::string::npos) {
					size_t startPos = line.find(">", versionPos + 24) + 1;
					size_t endPos = line.find("<", startPos);
					version = std::stoi(line.substr(startPos, endPos - startPos));
					continue;
				}

				size_t timeStampPos = line.find("<strong>TimeStamp:</strong>");
				if (timeStampPos != std::string::npos) {
					size_t startPos = line.find(">", timeStampPos + 26) + 2;
					size_t endPos = line.find("<", startPos);
					timeStamp = line.substr(startPos, endPos - startPos);
					continue;
				}

				size_t prevHashPos = line.find("<strong>PrevHash:</strong>");
				if (prevHashPos != std::string::npos) {
					size_t startPos = line.find(">", timeStampPos + 25) + 2;
					size_t endPos = line.find("<", startPos);
					prevHash = line.substr(startPos, endPos - startPos);
					continue;
				}

				size_t merkleRootPos = line.find("<strong>MerkleRoot:</strong>");
				if (merkleRootPos != std::string::npos) {
					size_t startPos = line.find(">", timeStampPos + 28) + 2;
					size_t endPos = line.find("<", startPos);
					merkleRoot = line.substr(startPos, endPos - startPos);
					continue;
				}

				size_t validatorIDPos = line.find("<strong>ValidatorID:</strong>");
				if (validatorIDPos != std::string::npos) {
					size_t startPos = line.find(">", timeStampPos + 29) + 2;
					size_t endPos = line.find("<", startPos);
					validatorID = std::stoi(line.substr(startPos, endPos - startPos));
					continue;
				}

				size_t transactionPos = line.find("<strong>Transaction:</strong>");
				if (transactionPos != std::string::npos) {
					size_t startPos = line.find(">", timeStampPos + 29) + 2;
					size_t endPos = line.find("<", startPos);
					transactions.push_back(line.substr(startPos, endPos - startPos));
					if (transactions.size() == 7)
						break;
					else
						continue;
				}
			}
			CBlock* newBlock = new CBlock(hash, version, timeStamp, prevHash, merkleRoot, validatorID, transactions);

			m_chain.push_back(newBlock);
		}
	}

	if (m_chain.empty())
		return false;

	return true;
}

bool CBlockChain::LoadChainData()
{
	return false;
}

void CBlockChain::CreateGenesisBlock()
{
	CBlock* genesis = new CBlock;

	genesis->CreateGenesisBlock();

	m_chain.emplace_back(genesis);

	SaveChainDataToHTML("BlockChain/HHS.html");
}

void CBlockChain::SaveChainDataToHTML(const std::string& filename) const
{
	std::ofstream outFile(filename);

	if (outFile.fail()) {
		return;
	}

	outFile << "<!DOCTYPE html>\n";
	outFile << "<html>\n";
	outFile << "<head>\n";
	outFile << "  <title>HHS BlockChain</title>\n";
	outFile << "  <style>\n";
	outFile << "    body {\n";
	outFile << "      font-family: Arial, sans-serif;\n";
	outFile << "      text-align: center;\n";
	outFile << "    }\n";
	outFile << "    .block-chain {\n";
	outFile << "      display: flex;\n";
	outFile << "      align-items: center;\n";
	outFile << "      justify-content: flex-start;\n";
	outFile << "      margin-top: 200px;\n";
	outFile << "      overflow-x: auto;\n";
	outFile << "      white-space: nowrap; \n";
	outFile << "      padding-bottom:10px;\n";
	outFile << "      padding-top:10px;\n";
	outFile << "    }\n";
	outFile << "    .block {\n";
	outFile << "      width: 100px;\n";
	outFile << "      height: 100px;\n";
	outFile << "      background-color: lightblue;\n";
	outFile << "      display: flex;\n";
	outFile << "      justify-content: center;\n";
	outFile << "      align-items: center;\n";
	outFile << "      position: relative;\n";
	outFile << "      cursor: pointer;\n";
	outFile << "      border: 2px solid transparent;\n";
	outFile << "      transition: all 0.3s ease;\n";
	outFile << "    }\n";
	outFile << "    .block:hover {\n";
	outFile << "      transform: scale(1.2);\n";
	outFile << "    }\n";
	outFile << "    .block.clicked {\n";
	outFile << "      background-color: lightgreen;\n";
	outFile << "    }\n";
	outFile << "    .block.active {\n";
	outFile << "      border-color: orange;\n";
	outFile << "    }\n";
	outFile << "    .block::before {\n";
	outFile << "      content: \"\";\n";
	outFile << "      position: absolute;\n";
	outFile << "      top: -20px;\n";
	outFile << "      left: 50%;\n";
	outFile << "      transform: translateX(-50%);\n";
	outFile << "      font-weight: bold;\n";
	outFile << "    }\n";
	outFile << "    .connector {\n";
	outFile << "      flex: 1;\n";
	outFile << "      height: 4px;\n";
	outFile << "      background-color: black;\n";
	outFile << "    }\n";
	outFile << "    .block-info {\n";
	outFile << "      display: none;\n";
	outFile << "      position: absolute;\n";
	outFile << "      top: 400px;\n";
	outFile << "      left: 50%;\n";
	outFile << "      transform: translateX(-50%);\n";
	outFile << "      width: 700px;\n";
	outFile << "      background-color: white;\n";
	outFile << "      border: 1px solid black;\n";
	outFile << "      padding: 10px;\n";
	outFile << "      text-align: left;\n";
	outFile << "    }\n";
	outFile << "  </style>\n";
	outFile << "</head>\n";
	outFile << "<body>\n";
	outFile << "  <h1>HHS BlockChain</h1>\n";
	outFile << "  <div class=\"block-chain\">\n";

	for (int i = static_cast<int>(m_chain.size()) - 1; i >= 0; --i) {
		outFile << "    <div class=\"block\" data-block-number=\"" + std::to_string(i + 1) + "\" onclick=\"showBlockInfo(" + std::to_string(i + 1) + ")\">\n";
		outFile << "      <span style=\"font-weight: bold;\">Block " + std::to_string(i + 1) + "</span>\n";
		outFile << "    </div>\n";
		if(i != 0)
			outFile << "    <div class=""connector""></div>\n";
	}
	outFile << "  </div>\n\n";

	for (int i = 0; i < m_chain.size(); ++i) {
		outFile << "  <div class=\"block-info\" id=\"blockInfo" + std::to_string(i + 1) + "\">\n";
		outFile << "    <h2>Block Information</h2>\n";
		outFile << "    <p><strong>Block Number:</strong> 1</p>\n";
		outFile << "    <p><strong>Hash:</strong> " + m_chain[i]->GetHash() + "</p>\n";
		outFile << "    <p><strong>Version:</strong> " + std::to_string(m_chain[i]->GetVersion()) + "</p>\n";
		outFile << "    <p><strong>TimeStamp:</strong> " + m_chain[i]->GetTimeStamp() + "</p>\n";
		outFile << "    <p><strong>PrevHash:</strong> " + m_chain[i]->GetPrevHash() + "</p>\n";
		outFile << "    <p><strong>MerkleRoot:</strong> " + m_chain[i]->GetMerkleRoot() + "</p>\n";
		outFile << "    <p><strong>ValidatorID:</strong> " + std::to_string(m_chain[i]->GetValidatorID()) + "</p>\n";

		if (m_chain[i]->GetTransactions().empty()) {
			for (int k = 0; k < 7; ++k) {
				outFile << "    <p><strong>Transaction:</strong> \"\" </p>\n";
			}
		}
		else {
			for (int j = 0; j < m_chain[i]->GetTransactions().size(); ++j) {
				outFile << "    <p><strong>Transaction:</strong> " + m_chain[i]->GetTransactions()[j] + "</p>\n";
			}
		}
		outFile << "  </div>\n\n";
	}

	outFile << "  <script>\n";
	outFile << "    function showBlockInfo(blockNumber) {\n";
	outFile << "      const blockInfoDivs = document.querySelectorAll(\".block-info\");\n";
	outFile << "      blockInfoDivs.forEach(div => {\n";
	outFile << "        div.style.display = \"none\";\n";
	outFile << "      });\n\n";
	outFile << "      const blockInfo = document.getElementById(\"blockInfo\" + blockNumber);\n";
	outFile << "      if (blockInfo) {\n";
	outFile << "        blockInfo.style.display = \"block\";\n";
	outFile << "      }\n\n";
	outFile << "      const blocks = document.querySelectorAll(\".block\");\n";
	outFile << "      blocks.forEach(block => {\n";
	outFile << "        block.classList.remove(\"active\");\n";
	outFile << "      });\n\n";
	outFile << "      const clickedBlock = document.querySelector(`[data-block-number=\"${blockNumber}\"]`);\n";
	outFile << "      clickedBlock.classList.add(\"active\");\n";
	outFile << "    }\n\n";
	outFile << "    let isDragging = false;\n";
	outFile << "    let startX;\n";
	outFile << "    let scrollLeft;\n\n";
	outFile << "    document.querySelector(\".block-chain\").addEventListener(\"mousedown\", e => {\n";
	outFile << "      isDragging = true;\n";
	outFile << "      startX = e.pageX - document.querySelector(\".block-chain\").offsetLeft;\n";
	outFile << "      scrollLeft = document.querySelector(\".block-chain\").scrollLeft;\n";
	outFile << "    });\n\n";
	outFile << "    document.querySelector(\".block-chain\").addEventListener(\"mousemove\", e => {\n";
	outFile << "      if (!isDragging) return;\n";
	outFile << "      e.preventDefault();\n";
	outFile << "      const x = e.pageX - document.querySelector(\".block-chain\").offsetLeft;\n";
	outFile << "      const walk = (x - startX) * 3;\n";
	outFile << "      document.querySelector(\".block-chain\").scrollLeft = scrollLeft - walk;\n";
	outFile << "    });\n\n";
	outFile << "    document.querySelector(\".block-chain\").addEventListener(\"mouseup\", () => {\n";
	outFile << "      isDragging = false;\n";
	outFile << "    });\n\n";
	outFile << "    document.querySelector(\".block-chain\").addEventListener(\"mouseleave\", () => {\n";
	outFile << "      isDragging = false;\n";
	outFile << "    });\n";
	outFile << "  </script>\n";
	outFile << "</body>\n";
	outFile << "</html>";
}