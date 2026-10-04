#pragma once

class CMerkleTree {
public:
    CMerkleTree();
    ~CMerkleTree();

    void BuildMerkleTree();
    void AddTransaction(const std::string& tx) { m_transaction.push_back(tx); }
    void AddTransaction(const std::vector<std::string>& txs) { for (int i = 0; i < txs.size(); ++i) m_transaction.push_back(txs[i]); }
    void ChangeTransaction(const std::string& tx, const int index) { m_transaction[index] = tx; }   // for test
    const std::string& GetRoot() const { return m_root; }

    bool CheckTransaction(int index);
    void RemoveTransaction(int index);

private:
    std::vector<std::string> GetBranch(int index);

private:
    std::vector<std::string> m_transaction;
    std::vector<std::string> m_merkleTree;
    std::string m_root;
};