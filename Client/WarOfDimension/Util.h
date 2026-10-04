#pragma once
class Util
{
public:

	static int GenerateRandomInt(int start, int end) {
		uniform_int_distribution<> uid(start, end);

		return uid(m_randomEngine);
	}

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

	static float DistanceXZ(const XMFLOAT3& pos1, const XMFLOAT3& pos2) {
		return sqrtf(powf(pos1.x - pos2.x, 2.f) + powf(pos1.z - pos2.z, 2.f));
	}

private:
	static random_device m_randomDevice;
	static mt19937 m_randomEngine;
};

