#pragma once

class StickyArrow : public CSkillObject
{
public:
	StickyArrow(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, CLoadedModelInfo* model);
	~StickyArrow() override;
};