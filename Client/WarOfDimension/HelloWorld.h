#pragma once
class HelloWorld : public CSkillObject
{
public:
	HelloWorld(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, CLoadedModelInfo* model, CShader* pShader);
	~HelloWorld() override;
};