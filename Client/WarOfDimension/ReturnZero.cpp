#include "stdafx.h"
#include "Object.h"
#include "ReturnZero.h"

ReturnZero::ReturnZero(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, CLoadedModelInfo* model)
{
	SetChild(model->m_pModelRootObject, true);

	SetScaleValue(0.5f, 0.5f, 0.5f);

	AllCreateShaderVariables(pd3dDevice, pd3dCommandList);
}

ReturnZero::~ReturnZero()
{
}