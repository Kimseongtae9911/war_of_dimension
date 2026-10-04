#include "stdafx.h"
#include "Object.h"
#include "ProtectedArea.h"
#include "Shader.h"

ProtectedArea::ProtectedArea(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, CLoadedModelInfo* model, CShader* pShader)
{
	SetChild(model->m_pModelRootObject, true);

	SetScaleValue(40.f, 40.f, 40.f);
	AllCreateShaderVariables(pd3dDevice, pd3dCommandList);

	pShader->AddRef();
	model->m_pModelRootObject->m_ppMaterials[0]->SetShader(pShader);
}

ProtectedArea::~ProtectedArea()
{
}