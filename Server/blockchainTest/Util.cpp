#include "pch.h"
#include "Util.h"
#include <sstream>
#include <iomanip>

std::string Util::BinaryToHex(std::string bin)
{
    std::string result;
    for (int i = 0; i < 64; ++i) {
        std::string temp;
        for (int j = 0; j < 4; ++j) {
            temp += bin[i * 4 + j];
        }
        std::bitset<4> num(temp);
        std::stringstream hexnum;
        hexnum << std::hex << std::setfill('0') << std::setw(1) << num.to_ullong();
        result += hexnum.str();
    }

    return result;
}

std::string Util::UCharBinToString(const unsigned char* bin, int size)
{
    char* result = new char[2 * size + 1];
    result[2 * size] = 0;
    for (size_t i = 0; i < size; ++i) {
        sprintf_s(result + i * 2, (2 * size + 1) - i * 2, "%02x", bin[i]);
    }
    std::string s(result);
    delete[] result;
    return s;
}
