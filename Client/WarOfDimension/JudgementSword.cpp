#include "stdafx.h"
#include "Object.h"
#include "JudgementSword.h"

JudgementSword::JudgementSword(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, CLoadedModelInfo* model)
{
	SetChild(model->m_pModelRootObject, true);

	SetScaleValue(0.7f, 1.3f, 0.7f);

	AllCreateShaderVariables(pd3dDevice, pd3dCommandList);
}

JudgementSword::~JudgementSword()
{
}