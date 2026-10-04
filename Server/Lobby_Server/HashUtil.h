#pragma once
class HashUtil
{
public:
	static std::string UCharBinToString(const unsigned char* bin, int size) {
		char* result = new char[2 * size + 1];
		result[2 * size] = 0;
		for (size_t i = 0; i < size; ++i) {
			sprintf_s(result + i * 2, (2 * size + 1) - i * 2, "%02x", bin[i]);
		}
		std::string s(result);
		delete[] result;
		return s;
	}
};

