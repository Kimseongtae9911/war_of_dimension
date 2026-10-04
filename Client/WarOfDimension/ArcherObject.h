#pragma once

class ArcherObject : public CSkillObject
{
public:
	ArcherObject(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, CLoadedModelInfo* model);
	~ArcherObject() override;
};