#include "pch.h"
#include "SHA256.h"

std::array<unsigned char, SHA256::BLOCKSIZE * 2> SHA256::m_buffer;
int SHA256::m_bufferIndex;
unsigned int SHA256::m_length;
std::array<unsigned int, 8> SHA256::m_chain;
constexpr std::array<unsigned int, 64> SHA256::m_k;

void SHA256::Update(const unsigned char* data, unsigned int length)
{
	unsigned int temp = BLOCKSIZE - m_bufferIndex;
	unsigned int remaining = length < temp ? length : temp;
	std::memcpy(&m_buffer[m_bufferIndex], data, remaining);
	if (m_bufferIndex + length < BLOCKSIZE) {
		m_bufferIndex += length;
		return;
	}
	unsigned int number = (length - remaining) / BLOCKSIZE;
	const unsigned char* message = data + remaining;
	ProcessBlock(m_buffer.data(), 1);
	ProcessBlock(message, number);
	remaining = (length - remaining) % BLOCKSIZE;
	std::memcpy(m_buffer.data(), &message[number << 6], remaining);
	m_bufferIndex = remaining;
	m_length += (number + 1) << 6;
}

void SHA256::ProcessBlock(const unsigned char* message, int number)
{
	unsigned int w[64];
	unsigned int chains[8] = {};
	const unsigned char* tempMessage;

	for (size_t j = 0; j < number; ++j) {
		tempMessage = message + (j << 6);
		for (int i = 0; i < 16; ++i) {
			w[i] = (static_cast<unsigned int>(tempMessage[(i << 2) + 3])) | (static_cast<unsigned int>(tempMessage[(i << 2) + 2]) << 8) | (static_cast<unsigned int>(tempMessage[(i << 2) + 1]) << 16) | (static_cast<unsigned int>(tempMessage[i << 2]) << 24);
		}

		for (int i = 16; i < 64; ++i) {
			unsigned int s0 = Sigma0(w[i - 15]);
			unsigned int s1 = Sigma1(w[i - 2]);
			w[i] = w[i - 16] + w[i - 7] + s0 + s1;
		}

		for (int i = 0; i < m_chain.size(); ++i) {
			chains[i] = m_chain[i];
		}

		for (int i = 0; i < BLOCKSIZE; ++i) {
			unsigned int T1 = chains[7] + (RightRotate(chains[4], 6) ^ RightRotate(chains[4], 11) ^ RightRotate(chains[4], 25)) + Ch(chains[4], chains[5], chains[6]) + m_k[i] + w[i];
			unsigned int T2 = (RightRotate(chains[0], 2) ^ RightRotate(chains[0], 13) ^ RightRotate(chains[0], 22)) + Maj(chains[0], chains[1], chains[2]);
			chains[7] = chains[6];
			chains[6] = chains[5];
			chains[5] = chains[4];
			chains[4] = chains[3] + T1;
			chains[3] = chains[2];
			chains[2] = chains[1];
			chains[1] = chains[0];
			chains[0] = T1 + T2;
		}

		for (int i = 0; i < m_chain.size(); ++i) {
			m_chain[i] += chains[i];
		}
	}
}

void SHA256::Finalize(unsigned char* output)
{
	unsigned int number = (1 + ((BLOCKSIZE - 9) < (m_bufferIndex % BLOCKSIZE)));
	unsigned int tempBl = (m_length + m_bufferIndex) << 3;
	unsigned int tempLen = number << 6;
	memset(m_buffer.data() + m_bufferIndex, 0, tempLen - static_cast<unsigned int>(m_bufferIndex));
	m_buffer[m_bufferIndex] = 0x80;

	for (int i = 3; i >= 0; --i) {
		*(m_buffer.data() + tempLen - 4 + i) = static_cast<unsigned char>(RightShift(tempBl, (3 - i) * 8));
	}

	ProcessBlock(m_buffer.data(), number);
	for (int i = 0; i < 8; ++i) {
		for (int j = 3; j >= 0; --j) {
			*(&output[i << 2] + j) = static_cast<unsigned char>(RightShift(m_chain[i], (3 - j) * 8));
		}
	}
}

std::string SHA256::Encrpyt(std::string data)
{
	unsigned char output[HASHSIZE];
	memset(output, 0, HASHSIZE);

	Reset();
	Update(reinterpret_cast<const unsigned char*>(data.c_str()), static_cast<unsigned int>(data.length()));
	Finalize(output);

	//char result[2 * HASHSIZE + 1];
 	return Util::UCharBinToString(output, HASHSIZE);

	//return std::string(result);
}

void SHA256::Reset()
{
	m_length = 0;
	m_bufferIndex = 0;

	//initial values
	m_chain[0] = 0x6a09e667;
	m_chain[1] = 0xbb67ae85;
	m_chain[2] = 0x3c6ef372;
	m_chain[3] = 0xa54ff53a;
	m_chain[4] = 0x510e527f;
	m_chain[5] = 0x9b05688c;
	m_chain[6] = 0x1f83d9ab;
	m_chain[7] = 0x5be0cd19;
}
