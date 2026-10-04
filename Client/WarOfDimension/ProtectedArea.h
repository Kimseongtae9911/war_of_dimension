#pragma once
class ProtectedArea : public CSkillObject
{
public:
	ProtectedArea(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, CLoadedModelInfo* model, CShader* pShader);
	~ProtectedArea() override;
};