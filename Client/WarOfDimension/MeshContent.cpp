#include "stdafx.h"
#include "MeshContent.h"
#include <bcrypt.h>
#include <stdexcept>

#pragma comment(lib, "bcrypt.lib")

namespace
{
    class ContentReader
    {
    public:
        explicit ContentReader(FILE* file) : m_file(file)
        {
            if (!file || BCryptCreateHash(BCRYPT_SHA256_ALG_HANDLE, &m_hash, nullptr, 0, nullptr, 0, 0) < 0)
                throw std::runtime_error("메시 내용 해시를 준비하지 못했습니다.");
        }
        ~ContentReader() { BCryptDestroyHash(m_hash); }
        ContentReader(const ContentReader&) = delete;
        ContentReader& operator=(const ContentReader&) = delete;

        void Read(void* data, size_t bytes, bool hash = true)
        {
            if (fread(data, 1, bytes, m_file) != bytes)
                throw std::runtime_error("메시 데이터가 잘렸습니다.");
            if (hash)
            {
                if (BCryptHashData(m_hash, static_cast<PUCHAR>(data), static_cast<ULONG>(bytes), 0) < 0)
                    throw std::runtime_error("메시 내용 해시 계산에 실패했습니다.");
                m_bytes += bytes;
            }
        }
        int Count()
        {
            int value = 0;
            Read(&value, sizeof(value));
            if (value < 0) throw std::runtime_error("음수 메시 데이터 개수입니다.");
            return value;
        }
        std::string String(bool hash = true)
        {
            unsigned char length = 0;
            Read(&length, sizeof(length), hash);
            if (length >= 64) throw std::runtime_error("메시 이름/태그가 63바이트를 초과했습니다.");
            std::string value(length, '\0');
            Read(value.data(), length, hash);
            return value;
        }
        void Array(std::uint64_t count, size_t elementBytes)
        {
            std::array<unsigned char, 64 * 1024> chunk{};
            std::uint64_t bytes = count * elementBytes;
            const auto remaining = _filelengthi64(_fileno(m_file)) - _ftelli64(m_file);
            if (remaining < 0 || bytes > static_cast<std::uint64_t>(remaining))
                throw std::runtime_error("메시 배열 크기가 파일 범위를 초과했습니다.");
            while (bytes)
            {
                const size_t part = static_cast<size_t>((std::min)(bytes, static_cast<std::uint64_t>(chunk.size())));
                Read(chunk.data(), part);
                bytes -= part;
            }
        }
        void Finish(MeshContentRecord& record)
        {
            if (BCryptFinishHash(m_hash, record.digest.data(), static_cast<ULONG>(record.digest.size()), 0) < 0)
                throw std::runtime_error("메시 내용 해시 완료에 실패했습니다.");
            record.contentBytes = m_bytes;
        }
    private:
        FILE* m_file;
        BCRYPT_HASH_HANDLE m_hash = nullptr;
        std::uint64_t m_bytes = 0;
    };
}

MeshContentRecord ReadMeshContentRecord(FILE* file)
{
    ContentReader reader(file);
    MeshContentRecord record;
    record.begin = _ftelli64(file);
    record.vertices = reader.Count();
    record.name = reader.String(false);
    for (;;)
    {
        const auto token = reader.String();
        if (token == "</Mesh>") break;
        if (token == "<Bounds>:") reader.Array(6, sizeof(float));
        else if (token == "<Positions>:" || token == "<Normals>:" || token == "<Tangents>:" || token == "<BiTangents>:")
        {
            const int count = reader.Count();
            if (count != 0 && count != record.vertices)
                throw std::runtime_error("메시 정점 속성 개수가 서로 다릅니다.");
            reader.Array(count, sizeof(float) * 3);
        }
        else if (token == "<Colors>:") reader.Array(reader.Count(), sizeof(float) * 4);
        else if (token == "<TextureCoords0>:" || token == "<TextureCoords1>:")
        {
            const int count = reader.Count();
            if (count != 0 && count != record.vertices)
                throw std::runtime_error("메시 UV 개수가 정점 수와 다릅니다.");
            reader.Array(count, sizeof(float) * 2);
        }
        else if (token == "<SubMeshes>:")
        {
            const int count = reader.Count();
            for (int i = 0; i < count; ++i)
            {
                if (reader.String() != "<SubMesh>:") throw std::runtime_error("메시 subset 태그가 올바르지 않습니다.");
                if (reader.Count() != i) throw std::runtime_error("메시 subset 순서가 올바르지 않습니다.");
                reader.Array(reader.Count(), sizeof(unsigned int));
            }
        }
        else throw std::runtime_error("지원하지 않는 메시 태그: " + token);
    }
    record.end = _ftelli64(file);
    if (record.begin < 0 || record.end < record.begin) throw std::runtime_error("메시 파일 위치 확인에 실패했습니다.");
    reader.Finish(record);
    return record;
}
