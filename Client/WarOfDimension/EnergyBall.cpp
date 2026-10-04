#include "stdafx.h"
#include "Object.h"
#include "EnergyBall.h"

EnergyBall::EnergyBall(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, CLoadedModelInfo* model)
{
	SetChild(model->m_pModelRootObject, true);

	SetScaleValue(1.f, 1.f, 1.f);

	AllCreateShaderVariables(pd3dDevice, pd3dCommandList);
}

EnergyBall::~EnergyBall()
{
}