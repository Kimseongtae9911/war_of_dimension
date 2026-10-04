#pragma once
class DimensionCrush : public CSkillObject
{
public:
	DimensionCrush(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, CLoadedModelInfo* model);
	~DimensionCrush() override;

	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera = NULL, int SharedNum = 0, int nPipelineState = 0) {};
};