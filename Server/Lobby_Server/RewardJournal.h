#pragma once
#include <Windows.h>
#include <bcrypt.h>
#include <array>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <unordered_set>
#pragma comment(lib, "bcrypt.lib")

namespace wod_server
{
class RewardJournal
{
  public:
    struct Record
    {
        std::string m_operation;
        std::string m_account;
        int m_delta = 0;
    };

    static std::string Hash(const void* _data, size_t _size)
    {
        std::array<unsigned char, 32> bytes{};
        if (BCryptHash(BCRYPT_SHA256_ALG_HANDLE, nullptr, 0, static_cast<PUCHAR>(const_cast<void*>(_data)), static_cast<ULONG>(_size), bytes.data(), static_cast<ULONG>(bytes.size())) < 0)
            throw std::runtime_error("reward digest failed");

        return Hex(bytes.data(), bytes.size());
    }

    explicit RewardJournal(std::filesystem::path _directory) : m_directory(std::move(_directory))
    {
        std::filesystem::create_directories(m_directory);
        m_lock = CreateFileW((m_directory / L"owner.lock").c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (m_lock == INVALID_HANDLE_VALUE)
            throw std::runtime_error("reward journal already owned or inaccessible");
    }

    ~RewardJournal()
    {
        if (m_lock != INVALID_HANDLE_VALUE)
            CloseHandle(m_lock);
    }

    RewardJournal(const RewardJournal&) = delete;
    RewardJournal& operator=(const RewardJournal&) = delete;

    std::vector<Record> Recover()
    {
        std::lock_guard lock(m_mutex);
        std::vector<Record> pending;
        std::vector<std::filesystem::path> files;
        for (const auto& file : std::filesystem::directory_iterator(m_directory))
        {
            if (file.path().extension() != L".pending" && file.path().extension() != L".writing")
                continue;

            if (!file.is_regular_file() || file.file_size() > 1024)
                throw std::runtime_error("invalid reward journal file; preserve for inspection");

            files.push_back(file.path());
        }

        std::unordered_set<std::string> identities;
        for (const auto& file : files)
        {
            std::ifstream input(file, std::ios::binary);
            const std::string data((std::istreambuf_iterator<char>(input)), {});
            input.close();
            auto record = Decode(data);
            if (file.stem().string() != record.m_operation || !identities.insert(record.m_operation).second)
                throw std::runtime_error("reward journal identity mismatch");

            if (file.extension() == L".writing")
                Promote(file, Path(record.m_operation));

            pending.push_back(std::move(record));
            if (pending.size() > 4096)
                throw std::runtime_error("reward journal recovery capacity exceeded");
        }

        m_pending = pending.size();

        return pending;
    }

    Record Append(const std::string& _account, int _delta)
    {
        std::lock_guard lock(m_mutex);
        if (_account.empty() || _account.size() >= 10 || m_pending >= 4096)
            throw std::runtime_error("reward journal admission rejected");

        std::array<unsigned char, 16> id{};
        if (BCryptGenRandom(nullptr, id.data(), static_cast<ULONG>(id.size()), BCRYPT_USE_SYSTEM_PREFERRED_RNG) < 0)
            throw std::runtime_error("reward operation identity failed");

        Record record{Hex(id.data(), id.size()), _account, _delta};
        const auto target = Path(record.m_operation);
        auto temporary = target;
        temporary.replace_extension(L".writing");
        const std::string body = "WOD_REWARD_V1\n" + record.m_operation + '\n' + Hex(reinterpret_cast<const unsigned char*>(_account.data()), _account.size()) + '\n' + std::to_string(_delta) + '\n';
        const std::string data = body + Hash(body.data(), body.size()) + '\n';
        HANDLE file = CreateFileW(temporary.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH, nullptr);
        if (file == INVALID_HANDLE_VALUE)
            throw std::runtime_error("cannot create reward recovery record");

        DWORD written = 0;
        const bool saved = WriteFile(file, data.data(), static_cast<DWORD>(data.size()), &written, nullptr) && written == data.size() && FlushFileBuffers(file);
        CloseHandle(file);
        if (!saved)
            throw std::runtime_error("cannot persist reward recovery record; incomplete file retained");

        Promote(temporary, target);
        ++m_pending;

        return record;
    }

    void Complete(const Record& _record)
    {
        std::lock_guard lock(m_mutex);
        if (!DeleteFileW(Path(_record.m_operation).c_str()))
            throw std::runtime_error("reward committed; recovery record retained for idempotent replay");

        --m_pending;
    }

    size_t Pending() const
    {
        std::lock_guard lock(m_mutex);

        return m_pending;
    }

  private:
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

    static bool IsHex(const std::string& _value)
    {
        return !_value.empty() && _value.find_first_not_of("0123456789abcdef") == std::string::npos;
    }

    static Record Decode(const std::string& _data)
    {
        std::istringstream input(_data);
        std::array<std::string, 5> lines;
        for (auto& line : lines)
            if (!std::getline(input, line))
                throw std::runtime_error("incomplete reward record; manual inspection required");

        std::string extra;
        if (std::getline(input, extra) || lines[0] != "WOD_REWARD_V1" || lines[1].size() != 32 || !IsHex(lines[1]) || lines[2].size() > 18 || lines[2].size() % 2 || !IsHex(lines[2]))
            throw std::runtime_error("invalid reward record; manual inspection required");

        const std::string body = lines[0] + '\n' + lines[1] + '\n' + lines[2] + '\n' + lines[3] + '\n';
        if (Hash(body.data(), body.size()) != lines[4])
            throw std::runtime_error("reward checksum mismatch");

        Record record{lines[1], {}, 0};
        for (size_t i = 0; i < lines[2].size(); i += 2)
            record.m_account += static_cast<char>(std::stoul(lines[2].substr(i, 2), nullptr, 16));

        size_t consumed = 0;
        record.m_delta = std::stoi(lines[3], &consumed);
        if (consumed != lines[3].size() || record.m_account.find('\0') != std::string::npos)
            throw std::runtime_error("invalid reward payload");

        return record;
    }

    static void Promote(const std::filesystem::path& _temporary, const std::filesystem::path& _target)
    {
        if (!MoveFileExW(_temporary.c_str(), _target.c_str(), MOVEFILE_WRITE_THROUGH))
            throw std::runtime_error("cannot promote durable reward record");
    }

    std::filesystem::path Path(const std::string& _operation) const
    {
        if (_operation.size() != 32 || !IsHex(_operation))
            throw std::runtime_error("invalid reward operation path");

        return m_directory / (_operation + ".pending");
    }

    const std::filesystem::path m_directory;
    HANDLE m_lock = INVALID_HANDLE_VALUE;
    mutable std::mutex m_mutex;
    size_t m_pending = 0;
};
}
