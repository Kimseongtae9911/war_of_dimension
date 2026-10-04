#include "pch.h"
#include "CMerkleTree.h"
#include "SHA256.h"

CMerkleTree::CMerkleTree()
{
}

CMerkleTree::~CMerkleTree()
{
}

void CMerkleTree::BuildMerkleTree()
{
    m_merkleTree.clear();
    for (const auto& s : m_transaction) {
        m_merkleTree.push_back(SHA256::Encrpyt(s));
    }

    int step = 0;
    for (int size = static_cast<int>(m_transaction.size()); size > 1; size = (size + 1) / 2) {
        for (int firstT = 0; firstT < size; firstT += 2) {
            int secondT = std::min(firstT + 1, size - 1);     // if the number of transactions are odd, the last transaction hash with itself
            m_merkleTree.push_back(SHA256::Encrpyt(m_merkleTree[step + firstT] + m_merkleTree[step + secondT]));
        }
        step += size;
    }

    m_root = m_merkleTree.back();
}

std::vector<std::string> CMerkleTree::GetBranch(int index)
{
    // Get nodes for Transaction check
    if (m_merkleTree.empty())
        BuildMerkleTree();

    std::vector<std::string> branch;
    int step = 0;
    for (int size = static_cast<int>(m_transaction.size()); size > 1; size = (size + 1) / 2) {
        int offset = std::min(index^1, size - 1);
        branch.push_back(m_merkleTree[step + offset]);
        index >>= 1;
        step += size;
    }

    return branch;
}

bool CMerkleTree::CheckTransaction(int index)
{
    std::vector<std::string> v = GetBranch(index);
    std::string transaction = SHA256::Encrpyt(m_transaction[index]);
    for (const auto& s : v) {
        if (index & 1) { // odd
            transaction = SHA256::Encrpyt(s + transaction);
        }
        else {  //even
            transaction = SHA256::Encrpyt(transaction + s);
        }
        index >>= 1;
    }

    return (transaction == m_root);
}

void CMerkleTree::RemoveTransaction(int index)
{
    // if the transaction is corrupted remove the corrupted transaction and fill it by duplicate of the adjacent transaction
    if (index & 1) {
        m_transaction[index] = m_transaction[index - 1];
    }
    else {
        m_transaction[index] = m_transaction[index + 1];
    }
    BuildMerkleTree();
}
