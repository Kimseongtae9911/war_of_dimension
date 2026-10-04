#include "pch.h"
#include "CSkillHandlerFactory.h"
#include "LogUtil.h"

constexpr int HANDLER_NUM = 5;
constexpr int SKILL_START_NUM = 48;

namespace wod_server {
	std::unique_ptr<CSkillHandlerFactory> CSkillHandlerFactory::m_instance;

	bool CSkillHandlerFactory::Initialize()
	{
		try {
			m_handlers.insert({ 48, new ArcherBackStepHandler });
			m_handlers.insert({ 49, new ArcherDodgeHandler });
			m_handlers.insert({ 50, new ArcherMultipleShotHandler });
			m_handlers.insert({ 51, new ArcherVaultHandler });
			m_handlers.insert({ 52, new ArcherVitalPointHandler });
			m_handlers.insert({ 53, new ArcherPenetraitingShotHandler });
			m_handlers.insert({ 54, new ArcherStickyArrowHandler });
			m_handlers.insert({ 55, new ArcherHunterEyesHandler });
			m_handlers.insert({ 56, new ArcherWindStepHandler });
			m_handlers.insert({ 57, new ArcherArrowRainHandler });
			m_handlers.insert({ 58, new ArcherPhoenixArrowHandler });
			m_handlers.insert({ 59, new ArcherStromArrowHandler });

			m_handlers.insert({ 60, new FighterDashHandler });
			m_handlers.insert({ 61, new FighterDodgeHandler });
			m_handlers.insert({ 62, new FighterSpinKickHandler });
			m_handlers.insert({ 63, new FighterWildAttackHandler });
			m_handlers.insert({ 64, new FighterDragonFistHandler });
			m_handlers.insert({ 65, new FighterMeditationHandler });
			m_handlers.insert({ 66, new FighterPointBloodHandler });
			m_handlers.insert({ 67, new FighterIndestructibleHandler });
			m_handlers.insert({ 68, new FighterWindKickHandler });
			m_handlers.insert({ 69, new FighterCounterHandler });
			m_handlers.insert({ 70, new FighterFireBallHandler });
			m_handlers.insert({ 71, new FighterRisingDragonHandler });

			m_handlers.insert({ 72, new SwordManDodgeHandler });
			m_handlers.insert({ 73, new SwordManRunHandler });
			m_handlers.insert({ 74, new SwordManHeavySlashHandler });
			m_handlers.insert({ 75, new SwordManShieldBashHandler });
			m_handlers.insert({ 76, new SwordManWarCryHandler });
			m_handlers.insert({ 77, new SwordManDefensiveStanceHandler });
			m_handlers.insert({ 78, new SwordManBerserkHandler });
			m_handlers.insert({ 79, new SwordManAuraBladeHandler });
			m_handlers.insert({ 80, new SwordManHellBladeHandler });
			m_handlers.insert({ 81, new SwordManJudgementSwordHandler });
			m_handlers.insert({ 82, new SwordManProtectedAreaHandler });
			m_handlers.insert({ 83, new SwordManAnkleCutHandler });

			m_handlers.insert({ 84, new WizardTeleportHandler });
			m_handlers.insert({ 85, new WizardBlinkHandler });
			m_handlers.insert({ 86, new WizardBodyStrengthHandler });
			m_handlers.insert({ 87, new WizardEnchantHandler });
			m_handlers.insert({ 88, new WizardEarthImpactHandler });
			m_handlers.insert({ 89, new WizardMagicMissilHandler });
			m_handlers.insert({ 90, new WizardEnergyBallHandler });
			m_handlers.insert({ 91, new WizardMagicEyeHandler });
			m_handlers.insert({ 92, new WizardDarknessRayHandler });
			m_handlers.insert({ 93, new WizardReflectHandler });
			m_handlers.insert({ 94, new WizardBigBangHandler });
			m_handlers.insert({ 95, new WizardOverloadHandler });

			m_handlers.insert({ 96, new OgreHeavySwingHandler });
			m_handlers.insert({ 97, new OgreCrunchHandler });
			m_handlers.insert({ 98, new OgreChargingHandler });
			m_handlers.insert({ 99, new OgreRoarHandler });
			m_handlers.insert({ 100, new OgreGluttonyHandler });
			m_handlers.insert({ 101, new OgreEndureHandler });
			m_handlers.insert({ 102, new OgreRockThrowHandler });
			m_handlers.insert({ 103, new OgreDimensionPunchHandler });
			m_handlers.insert({ 104, new OgreButtingHandler });
			m_handlers.insert({ 105, new OgreDimensionCrushHandler });
			
			m_handlers.insert({ 106, new ProSCMHandler });
			m_handlers.insert({ 107, new ProPointerHandler });
			m_handlers.insert({ 108, new ProPlusStatHandler });
			m_handlers.insert({ 109, new ProStatMinusHandler });
			m_handlers.insert({ 110, new ProReleaseHandler });
			m_handlers.insert({ 111, new ProDeleteHandler });
			m_handlers.insert({ 112, new ProReturnZeroHandler });
			m_handlers.insert({ 113, new ProSCLHandler });
			m_handlers.insert({ 114, new ProWhileTrueHandler });
			m_handlers.insert({ 115, new ProHelloWorldHandler });

			m_handlers.insert({ 116, new ArcherAttackHandler });
			m_handlers.insert({ 117, new FighterAttackHandler });
			m_handlers.insert({ 118, new SwordManAttackHandler });
			m_handlers.insert({ 119, new WizardAttackHandler });
			m_handlers.insert({ 120, new OgreAttackHandler });
			m_handlers.insert({ 121, new ProAttackHandler });
			

			for (int i = SKILL_START_NUM; i < 122; ++i) {
				for (int j = 0; j < HANDLER_NUM; ++j) {
					CSkillHandler* handler = m_handlers[i]->CreateHandler(nullptr);
					m_handlerPool[i - SKILL_START_NUM].push(handler);
				}
			}
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg(ex.what());
			return false;
		}

		return true;
	}

	bool CSkillHandlerFactory::Release()
	{
		return true;
	}

	void CSkillHandlerFactory::Handle(int skillNum, std::shared_ptr<CClient> client)
	{
		try {
			if (!m_handlers.contains(skillNum)) {
				LogPrinter::PrintMsg("Wrong SkillNum In SkillHandlerFactory");
				return;
			}

			CSkillHandler* handler;
			if (m_handlerPool[skillNum - SKILL_START_NUM].empty()) {
				handler = m_handlers[skillNum]->CreateHandler(client);
			}
			else {
				if (m_handlerPool[skillNum - SKILL_START_NUM].try_pop(handler)) {
					handler->SetClient(client);
				}
				else {
					handler = m_handlers[skillNum - SKILL_START_NUM]->CreateHandler(client);
				}
			}
			handler->Handle();
			m_handlerPool[skillNum - SKILL_START_NUM].push(handler);
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(CSkillHandlerFactory Handle), " + std::string(ex.what()));
		}
	}

}