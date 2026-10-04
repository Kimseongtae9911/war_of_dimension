#pragma once
class AuraBlade : public CSkillObject
{
public:
	AuraBlade(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, CLoadedModelInfo* model);
	~AuraBlade() override;

	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera = NULL, int SharedNum = 0, int nPipelineState = 0) {};
};


