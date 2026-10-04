#include "stdafx.h"
#include "Object.h"
#include "HelloWorld.h"
#include "Shader.h"


HelloWorld::HelloWorld(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, CLoadedModelInfo* model, CShader* pShader)
{
	SetChild(model->m_pModelRootObject, true);

	SetScaleValue(30.f, 30.f, 30.f);

	AllCreateShaderVariables(pd3dDevice, pd3dCommandList);

	pShader->AddRef();
	model->m_pModelRootObject->m_ppMaterials[0]->SetShader(pShader);
	
}

HelloWorld::~HelloWorld()
{
}