#include "stdafx.h"
#include "Object.h"
#include "SCL.h"

SCL::SCL(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, CLoadedModelInfo* model)
{
	SetChild(model->m_pModelRootObject, true);

	SetScaleValue(0.290938f * 2.f * 2.f, 1.f, 0.2125f * 2.f * 10.f);

	AllCreateShaderVariables(pd3dDevice, pd3dCommandList);
}

SCL::~SCL()
{
}