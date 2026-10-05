#pragma once
#include <cstdio>
#include <span>
#include <string>
#include <unordered_set>

struct ModelCustomize;

// 확정 외형만 사용한다. 기존 파일과 프레임 계층은 유지하며 미선택 자원 생성만 생략한다.
class ModelPartSelection
{
public:
    explicit ModelPartSelection(std::span<const ModelCustomize> appearances);
    bool Includes(const char* frameName) const;
private:
    std::unordered_set<std::string> selected;
};

void SkipModelSkinInfo(FILE* file);
void SkipModelMaterials(FILE* file);
