#pragma once
#include <mutex>
#include <string>
#include <unordered_map>

namespace wod_server
{
class AccountOwners
{
  public:
    bool Claim(const std::string& _name, void* _owner)
    {
        std::lock_guard lock(m_mutex);

        return m_owners.emplace(Key(_name), _owner).second;
    }

    void Release(const std::string& _name, const void* _owner)
    {
        std::lock_guard lock(m_mutex);
        const auto found = m_owners.find(Key(_name));
        if (found != m_owners.end() && found->second == _owner)
            m_owners.erase(found);
    }

    void* Find(const std::string& _name)
    {
        std::lock_guard lock(m_mutex);
        const auto found = m_owners.find(Key(_name));

        return found == m_owners.end() ? nullptr : found->second;
    }

  private:
    static std::string Key(std::string _name)
    {
        // 로컬 SQL collation의 ASCII 대소문자/후행 공백 동일성에 맞춘다.
        while (!_name.empty() && _name.back() == ' ')
            _name.pop_back();

        for (char& letter : _name)
            if (letter >= 'A' && letter <= 'Z')
                letter = static_cast<char>(letter - 'A' + 'a');

        return _name;
    }

    std::mutex m_mutex;
    std::unordered_map<std::string, void*> m_owners;
};
}
