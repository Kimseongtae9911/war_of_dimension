#pragma once

class PhoenixArrow : public CSkillObject
{
public:
	PhoenixArrow(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, CLoadedModelInfo* model);
	~PhoenixArrow() override;
};