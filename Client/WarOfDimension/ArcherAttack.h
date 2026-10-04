#pragma once

class ArcherAttack : public CSkillObject
{
public:
	ArcherAttack(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, CLoadedModelInfo* model);
	~ArcherAttack() override;
};