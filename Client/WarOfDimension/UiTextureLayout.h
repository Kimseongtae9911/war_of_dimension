#pragma once
#include <cstring>
#include <stdexcept>

// DDS reserved1의 UIB7/v1 계약. DirectX loader가 검증한 header에만 사용한다.
// xy: 내용 scale, zw: wrap gutter 뒤 내용 시작 offset.
inline DirectX::XMFLOAT4 ReadUiTextureLayout(const uint8_t* dds, const D3D12_RESOURCE_DESC& resource)
{
    uint32_t layout[6]{};
    std::memcpy(layout, dds + 32, sizeof(layout));
    if (layout[0] != 0x37424955u) return {1, 1, 0, 0};
    if (layout[1] != 1 || resource.Format != DXGI_FORMAT_BC7_UNORM ||
        resource.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE2D || resource.DepthOrArraySize != 1 ||
        resource.MipLevels != 1 || !layout[2] || !layout[3] ||
        static_cast<UINT64>(layout[2]) + layout[4] > resource.Width ||
        static_cast<UINT64>(layout[3]) + layout[5] > resource.Height ||
        ((layout[2] != resource.Width || layout[3] != resource.Height) &&
            (!layout[4] || !layout[5] ||
             static_cast<UINT64>(layout[2]) + layout[4] >= resource.Width ||
             static_cast<UINT64>(layout[3]) + layout[5] >= resource.Height)))
        throw std::runtime_error("UI DDS 내용 영역/BC7 형식 계약 불일치");
    return {static_cast<float>(layout[2]) / static_cast<float>(resource.Width),
            static_cast<float>(layout[3]) / resource.Height,
            static_cast<float>(layout[4]) / static_cast<float>(resource.Width),
            static_cast<float>(layout[5]) / resource.Height};
}
