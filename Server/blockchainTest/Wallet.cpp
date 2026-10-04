#include "pch.h"
#include "Wallet.h"
#include "SHA256.h"

CWallet::CWallet()
{
    OpenSSL_add_all_algorithms();
	CreateKeys();
}

CWallet::~CWallet()
{
    delete[] m_publicKey;
    EC_KEY_free(m_ecKey);
}

int CWallet::CreateKeys()
{
    // generate a new EC_KEY object for secp256k1 curve
    m_ecKey = EC_KEY_new_by_curve_name(NID_secp256k1);
    if (!m_ecKey) {
        return -1;
    }

    // generate a new public-private key pair
    if (EC_KEY_generate_key(m_ecKey) != 1) {
        return -1;
    }

    // get the public key in DER format
    const EC_POINT* ec_point = EC_KEY_get0_public_key(m_ecKey);

    // get size for public key
    size_t pub_len = EC_POINT_point2oct(EC_KEY_get0_group(m_ecKey), ec_point, POINT_CONVERSION_COMPRESSED, NULL, 0, NULL);
    m_publicKey = new unsigned char[pub_len]; 

    // generate public key
    if (EC_POINT_point2oct(EC_KEY_get0_group(m_ecKey), ec_point, POINT_CONVERSION_COMPRESSED, m_publicKey, pub_len, NULL) != pub_len) {
        delete[] m_publicKey;
        EC_KEY_free(m_ecKey);
        return -1;
    }

    // get the private key in PEM format
    BIO* bio = BIO_new(BIO_s_mem());
    if (!bio) {
        return -1;
    }
    if (PEM_write_bio_ECPrivateKey(bio, m_ecKey, NULL, NULL, 0, NULL, NULL) != 1) {
        return -1;
    }
    BIO_get_mem_ptr(bio, &m_privateKey);

    //PrintKeys();

    // cleanup
    BIO_free(bio);
}

Signature* CWallet::MakeSignature(std::string msg)
{
    // create a SHA256 hash of the message
    Signature* sig = new Signature;
    sig->hash = SHA256::Encrpyt(msg);

    // sign the hash using the private key
    sig->signature = new unsigned char[ECDSA_size(m_ecKey)];
    if (ECDSA_sign(0, reinterpret_cast<const unsigned char*>(sig->hash.c_str()), SHA256::HASHSIZE, sig->signature, &sig->len, m_ecKey) != 1) {
        return nullptr;
    }
    else {
        return sig;
    }
}

bool CWallet::VerifySignature(Signature* sig)
{
    // verify the signature using the public key
    if (ECDSA_verify(0, reinterpret_cast<const unsigned char*>(sig->hash.c_str()), SHA256::HASHSIZE, sig->signature, sig->len, m_ecKey) != 1) {
        return false;
    }
    else {
        return true;
    }
}

void CWallet::PrintKeys()
{
    printf("Public key (DER format):\n");
    for (size_t i = 0; i < 33; i++) {
        printf("%02x", m_publicKey[i]);
    }

    printf("\n\nPrivate key (PEM format):\n%s", m_privateKey->data);
}
