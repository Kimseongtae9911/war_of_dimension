#include "stdafx.h"
#include "Object.h"
#include "CSkillModel.h"

CLoadedModelInfo* CSkillModel::pWizardModel = nullptr;
CLoadedModelInfo* CSkillModel::pArcherModel = nullptr;
CLoadedModelInfo* CSkillModel::pOgreModel = nullptr;
CLoadedModelInfo* CSkillModel::pAreaHelloWorldModel = nullptr;
CLoadedModelInfo* CSkillModel::pProtectedAreaModel = nullptr;

CSkillModel::~CSkillModel()
{
	delete pWizardModel;
	delete pArcherModel;
	delete pOgreModel;
	delete pAreaHelloWorldModel;
	delete pProtectedAreaModel;
}
