#pragma once

class MultipleShot : public CSkillObject
{
public:
	MultipleShot(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, CLoadedModelInfo* model);
	~MultipleShot() override;
};