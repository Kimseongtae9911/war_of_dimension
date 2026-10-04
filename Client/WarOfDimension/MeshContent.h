#pragma once

#include <array>
#include <cstdint>
#include <cstdio>
#include <string>

// 기존 .bin 형식은 유지하며 메시 이름을 제외한 전체 geometry를 식별한다.
struct MeshContentRecord
{
    std::array<unsigned char, 32> digest{};
    std::string name;
    int vertices = 0;
    std::int64_t begin = 0;
    std::int64_t end = 0;
    std::uint64_t contentBytes = 0;
};

// <Mesh>: 직후부터 </Mesh>까지 읽고 끝 위치에 남는다. 잘린/미지원 데이터는 예외로 거부한다.
MeshContentRecord ReadMeshContentRecord(FILE* file);
