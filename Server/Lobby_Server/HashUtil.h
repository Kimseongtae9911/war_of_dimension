#pragma once
class HashUtil
{
public:
	static std::string UCharBinToString(const unsigned char* _bin, int _size) {
		char* result = new char[2 * _size + 1];
		result[2 * _size] = 0;
		for (size_t i = 0; i < _size; ++i) {
			sprintf_s(result + i * 2, (2 * _size + 1) - i * 2, "%02x", _bin[i]);
		}
		std::string s(result);
		delete[] result;
		return s;
	}
};

