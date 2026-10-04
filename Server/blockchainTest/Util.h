#pragma once
class Util
{
public:
	static std::string BinaryToHex(std::string bin);
	static std::string UCharBinToString(const unsigned char* bin, int size);
	static void ReleaseMemory(const unsigned char* mem) { delete[] mem; }
};

