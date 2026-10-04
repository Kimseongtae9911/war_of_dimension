#pragma once
#include <openssl/bn.h>
#include <openssl/ec.h>
#include <openssl/evp.h>
#include <openssl/ecdsa.h>
#include <openssl/obj_mac.h>
#include <openssl/bio.h>
#include <openssl/pem.h>

struct Signature
{
	unsigned char* signature;
	unsigned int len;
	std::string hash;
};

class CWallet
{
public:
	CWallet();
	~CWallet();

	Signature* MakeSignature(std::string msg);
	bool VerifySignature(Signature* sig);

	// for test
	void PrintKeys();

private:
	int CreateKeys();

public:
	EC_KEY* m_ecKey;
	unsigned char* m_publicKey;

private:
	BUF_MEM* m_privateKey;
};

