#pragma once

class PenetraitingShot : public CSkillObject
{
public:
	PenetraitingShot(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, CLoadedModelInfo* model);
	~PenetraitingShot() override;
};