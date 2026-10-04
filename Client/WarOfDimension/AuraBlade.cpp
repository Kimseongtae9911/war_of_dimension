#include "stdafx.h"
#include "Object.h"
#include "AuraBlade.h"

AuraBlade::AuraBlade(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, CLoadedModelInfo* model)
{
	SetChild(model->m_pModelRootObject, true);

	SetScaleValue(0.2f, 2.0f, 0.22f);

	AllCreateShaderVariables(pd3dDevice, pd3dCommandList);
}

AuraBlade::~AuraBlade()
{
}