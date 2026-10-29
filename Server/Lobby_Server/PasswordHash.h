#pragma once
#include <Windows.h>
#include <bcrypt.h>
#include <array>
#include <stdexcept>
#include <string>
#include <string_view>
#pragma comment(lib, "bcrypt.lib")

namespace wod_server
{
class PasswordHash
{
  public:
    static std::string Encode(const std::string& _password)
    {
        std::array<unsigned char, 16> salt{};
        if (BCryptGenRandom(nullptr, salt.data(), static_cast<ULONG>(salt.size()), BCRYPT_USE_SYSTEM_PREFERRED_RNG) < 0)
            throw std::runtime_error("password salt generation failed");

        const auto hash = Derive(_password, salt);

        return std::string(m_prefix) + Hex(salt.data(), salt.size()) + '$' + Hex(hash.data(), hash.size());
    }

    static bool Verify(const std::string& _password, std::string_view _encoded)
    {
        constexpr size_t suffixSize = 32 + 1 + 64;
        if (!_encoded.starts_with(m_prefix) || _encoded.size() != m_prefix.size() + suffixSize)
            return false;

        const auto saltText = _encoded.substr(m_prefix.size(), 32);
        const auto hashText = _encoded.substr(m_prefix.size() + 33);
        if (_encoded[m_prefix.size() + 32] != '$' || !IsHex(saltText) || !IsHex(hashText))
            return false;

        std::array<unsigned char, 16> salt{};
        std::array<unsigned char, 32> expected{};
        Decode(saltText, salt.data());
        Decode(hashText, expected.data());
        auto actual = Derive(_password, salt);
        unsigned int difference = 0;
        for (size_t i = 0; i < actual.size(); ++i)
            difference |= actual[i] ^ expected[i];

        SecureZeroMemory(actual.data(), actual.size());

        return difference == 0;
    }

  private:
    static std::array<unsigned char, 32> Derive(const std::string& _password, const std::array<unsigned char, 16>& _salt)
    {
        BCRYPT_ALG_HANDLE algorithm = nullptr;
        if (BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, BCRYPT_ALG_HANDLE_HMAC_FLAG) < 0)
            throw std::runtime_error("password algorithm unavailable");

        std::array<unsigned char, 32> hash{};
        const auto result = BCryptDeriveKeyPBKDF2(algorithm, reinterpret_cast<PUCHAR>(const_cast<char*>(_password.data())), static_cast<ULONG>(_password.size()), const_cast<PUCHAR>(_salt.data()), static_cast<ULONG>(_salt.size()), 600000, hash.data(),
                                                  static_cast<ULONG>(hash.size()), 0);
        BCryptCloseAlgorithmProvider(algorithm, 0);
        if (result < 0)
            throw std::runtime_error("password derivation failed");

        return hash;
    }

    static bool IsHex(std::string_view _text)
    {
        return _text.find_first_not_of("0123456789abcdef") == std::string_view::npos;
    }

    static unsigned char Nibble(char _letter)
    {
        return static_cast<unsigned char>(_letter <= '9' ? _letter - '0' : _letter - 'a' + 10);
    }

    static void Decode(std::string_view _text, unsigned char* _bytes)
    {
        for (size_t i = 0; i < _text.size(); i += 2)
            _bytes[i / 2] = static_cast<unsigned char>((Nibble(_text[i]) << 4) | Nibble(_text[i + 1]));
    }

    static std::string Hex(const unsigned char* _bytes, size_t _size)
    {
        constexpr char digits[] = "0123456789abcdef";
        std::string text;
        for (size_t i = 0; i < _size; ++i)
        {
            text += digits[_bytes[i] >> 4];
            text += digits[_bytes[i] & 15];
        }

        return text;
    }

    static constexpr std::string_view m_prefix = "pbkdf2-sha256$600000$";
};
}
