#pragma once
class MagicMissile : public CSkillObject
{
public:
	MagicMissile(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, CLoadedModelInfo* model);
	~MagicMissile() override;

	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera = NULL, int SharedNum = 0, int nPipelineState = 0) {};
};

