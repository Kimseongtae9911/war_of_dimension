#include "stdafx.h"
#include "Object.h"
#include "StickyArrow.h"

StickyArrow::StickyArrow(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, CLoadedModelInfo* model)
{
	SetChild(model->m_pModelRootObject, true);

	SetScaleValue(2.f, 2.f, 2.f);

	AllCreateShaderVariables(pd3dDevice, pd3dCommandList);
}

StickyArrow::~StickyArrow()
{
}