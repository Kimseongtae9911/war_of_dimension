#include "stdafx.h"
#include "Object.h"
#include "BigBang.h"

BigBang::BigBang(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, CLoadedModelInfo* model)
{
	SetChild(model->m_pModelRootObject, true);

	SetScaleValue(0.25f, 0.25f, 0.25f);

	AllCreateShaderVariables(pd3dDevice, pd3dCommandList);
}

BigBang::~BigBang()
{
}