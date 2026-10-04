#pragma once

class RockThrow : public CSkillObject
{
public:
	RockThrow(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, CLoadedModelInfo* model);
	~RockThrow() override;
};