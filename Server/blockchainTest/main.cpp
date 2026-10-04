#include "pch.h"
#include "SHA256.h"
#include "CMerkleTree.h"
#include "BlockChain.h"
#include "Wallet.h"
#include <Windows.h>
// need the add libs in the linker
int main()
{
	std::vector<std::string> data = {
	  "Hash", "256", "WOD", "KHS", "KHJ", "KST"
	};
	CMerkleTree* tree = new CMerkleTree();
	tree->AddTransaction(data);
	tree->BuildMerkleTree();

	std::cout << "Root: " << tree->GetRoot() << std::endl;
	tree->ChangeTransaction("ABC", 1);

	if (false == tree->CheckTransaction(1)) {
		std::cout << "Wrong Transaction1" << std::endl;
		tree->RemoveTransaction(1);
	}	
	std::cout << "Root: " << tree->GetRoot() << std::endl;

	CBlockChain* chain = new CBlockChain;

	CWallet* wallet = new CWallet();

	Signature* sig = wallet->MakeSignature("Hello");
	if (wallet->VerifySignature(sig)) {
		std::cout << "O" << std::endl;
	}
	else {
		std::cout << "X" << std::endl;
	}

	delete chain;
	delete wallet;

	return 0;
}