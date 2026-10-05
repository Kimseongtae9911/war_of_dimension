#include "stdafx.h"
#include "ModelPartSelection.h"
#include "Object.h"
#include <cstring>
#include <stdexcept>

namespace
{
    void Read(FILE* file, void* target, size_t bytes)
    {
        if (fread(target, 1, bytes, file) != bytes) throw std::runtime_error("잘린 선택 파츠 레코드");
    }
    int Count(FILE* file)
    {
        int count;
        Read(file, &count, sizeof(count));
        if (count < 0) throw std::runtime_error("선택 파츠 레코드의 음수 개수");
        return count;
    }
    std::string Token(FILE* file)
    {
        unsigned char size;
        Read(file, &size, 1);
        std::string value(size, '\0');
        Read(file, value.data(), size);
        return value;
    }
    void Skip(FILE* file, __int64 size)
    {
        const auto begin = _ftelli64(file);
        if (begin < 0 || size < 0 || _fseeki64(file, 0, SEEK_END)) throw std::runtime_error("파츠 레코드 범위 조회 실패");
        const auto end = _ftelli64(file);
        if (size > end - begin || _fseeki64(file, begin + size, SEEK_SET)) throw std::runtime_error("잘린 파츠 배열");
    }
    bool IsPartName(const std::string& name)
    {
        for (int part = 1; part < 28; ++part)
        {
            const std::string prefix = std::string(CGameObject::FrameNames[part]) + '_';
            if (!name.starts_with(prefix)) continue;
            auto suffix = name.substr(prefix.size());
            if (part >= 15)
            {
                if (suffix.starts_with("Male_")) suffix.erase(0, 5);
                else if (suffix.starts_with("Female_")) suffix.erase(0, 7);
                else continue;
            }
            if (suffix.size() == 2 && suffix[0] >= '0' && suffix[0] <= '9' && suffix[1] >= '0' && suffix[1] <= '9') return true;
        }
        return false;
    }
}

ModelPartSelection::ModelPartSelection(std::span<const ModelCustomize> appearances)
{
    static_assert(sizeof(ModelCustomize) == 28 * sizeof(short));
    for (const auto& appearance : appearances)
    {
        short values[28];
        memcpy(values, &appearance, sizeof(values));
        for (int part = 1; part < 28; ++part)
        {
            if (values[part] < 0 || values[part] > 99) continue;
#ifdef WITH_DATABASE
            // DB 경로의 0번 부품은 유효하다. -1이 미선택이다.
#else
            if (values[part] == 0) continue;
#endif
            char name[128];
            if (part < 15) sprintf_s(name, "%s_%02d", CGameObject::FrameNames[part], values[part]);
            else sprintf_s(name, "%s_%s_%02d", CGameObject::FrameNames[part], appearance.Chr_Sex == 0 ? "Male" : "Female", values[part]);
            selected.insert(name);
        }
    }
}

bool ModelPartSelection::Includes(const char* frameName) const
{
    // 공통 본·무기·루트 및 미지원 이름은 보존한다. 새 에셋의 이름을 추측해서 제거하지 않는다.
    return !IsPartName(frameName) || selected.contains(frameName);
}

void SkipModelSkinInfo(FILE* file)
{
    Token(file);
    for (;;)
    {
        const auto tag = Token(file);
        if (tag == "</SkinningInfo>") return;
        if (tag == "<BonesPerVertex>:") Count(file);
        else if (tag == "<Bounds>:") Skip(file, 24);
        else if (tag == "<BoneNames>:")
        {
            const int count = Count(file);
            for (int i = 0; i < count; ++i) Token(file);
        }
        else if (tag == "<BoneOffsets>:") Skip(file, static_cast<__int64>(Count(file)) * 64);
        else if (tag == "<BoneIndices>:" || tag == "<BoneWeights>:") Skip(file, static_cast<__int64>(Count(file)) * 16);
        else throw std::runtime_error("미지원 스킨 레코드");
    }
}

void SkipModelMaterials(FILE* file)
{
    Count(file);
    for (;;)
    {
        const auto tag = Token(file);
        if (tag == "</Materials>") return;
        if (tag == "<Material>:") Count(file);
        else if (tag == "<AlbedoColor>:" || tag == "<EmissiveColor>:" || tag == "<SpecularColor>:") Skip(file, 16);
        else if (tag == "<Glossiness>:" || tag == "<Smoothness>:" || tag == "<Metallic>:" ||
                 tag == "<SpecularHighlight>:" || tag == "<GlossyReflection>:") Skip(file, 4);
        else if (tag == "<AlbedoMap>:" || tag == "<SpecularMap>:" || tag == "<NormalMap>:" ||
                 tag == "<MetallicMap>:" || tag == "<EmissionMap>:" || tag == "<DetailAlbedoMap>:" || tag == "<DetailNormalMap>:") Token(file);
        else if (tag != "</Material>") throw std::runtime_error("미지원 재질 레코드");
    }
}
