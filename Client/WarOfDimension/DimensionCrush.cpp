#include "stdafx.h"
#include "Object.h"
#include "DimensionCrush.h"

DimensionCrush::DimensionCrush(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, CLoadedModelInfo* model)
{
	SetChild(model->m_pModelRootObject, true);

	SetScaleValue(24.f, 24.f, 24.f);

	AllCreateShaderVariables(pd3dDevice, pd3dCommandList);
}

DimensionCrush::~DimensionCrush()
{
}