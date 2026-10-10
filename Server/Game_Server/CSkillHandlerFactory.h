#pragma once
#include "CSkillHandler.h"
#include "ArcherAttackHandler.h"
#include "ArcherArrowRainHandler.h"
#include "ArcherBackStepHandler.h"
#include "ArcherDodgeHandler.h"
#include "ArcherHunterEyesHandler.h"
#include "ArcherMultipleShotHandler.h"
#include "ArcherPenetraitingShotHandler.h"
#include "ArcherPhoenixArrowHandler.h"
#include "ArcherStickyArrowHandler.h"
#include "ArcherStromArrowHandler.h"
#include "ArcherVaultHandler.h"
#include "ArcherVitalPointHandler.h"
#include "ArcherWindStepHandler.h"

#include "FighterAttackHandler.h"
#include "FighterCounterHandler.h"
#include "FighterDashHandler.h"
#include "FighterDodgeHandler.h"
#include "FighterDragonFistHandler.h"
#include "FighterFireBallHandler.h"
#include "FighterIndestructibleHandler.h"
#include "FighterMeditationHandler.h"
#include "FighterPointBloodHandler.h"
#include "FighterRisingDragonHandler.h"
#include "FighterSpinKickHandler.h"
#include "FighterWildAttackHandler.h"
#include "FighterWindKickHandler.h"

#include "SwordManAttackHandler.h"
#include "SwordManAnkleCutHandler.h"
#include "SwordManAuraBladeHandler.h"
#include "SwordManBerserkHandler.h"
#include "SwordManDefensiveStanceHandler.h"
#include "SwordManDodgeHandler.h"
#include "SwordManHeavySlashHandler.h"
#include "SwordManHellBladeHandler.h"
#include "SwordManJudgementSwordHandler.h"
#include "SwordManProtectedAreaHandler.h"
#include "SwordManRunHandler.h"
#include "SwordManShieldBashHandler.h"
#include "SwordManWarCryHandler.h"

#include "WizardAttackHandler.h"
#include "WizardBigBangHandler.h"
#include "WizardBlinkHandler.h"
#include "WizardBodyStrengthHandler.h"
#include "WizardDarknessRayHandler.h"
#include "WizardEarthImpactHandler.h"
#include "WizardEnchantHandler.h"
#include "WizardEnergyBallHandler.h"
#include "WizardMagicEyeHandler.h"
#include "WizardMagicMissilHandler.h"
#include "WizardOverloadHandler.h"
#include "WizardReflectHandler.h"
#include "WizardTeleportHandler.h"

#include "ProAttackHandler.h"
#include "ProDeleteHandler.h"
#include "ProHelloWorldHandler.h"
#include "ProPlusStatHandler.h"
#include "ProPointerHandler.h"
#include "ProReleaseHandler.h"
#include "ProReturnZeroHandler.h"
#include "ProSCLHandler.h"
#include "ProSCMHandler.h"
#include "ProStatMinusHandler.h"
#include "ProWhileTrueHandler.h"

#include "OgreAttackHandler.h"
#include "OgreButtingHandler.h"
#include "OgreChargingHandler.h"
#include "OgreCrunchHandler.h"
#include "OgreDimensionCrushHandler.h"
#include "OgreDimensionPunchHandler.h"
#include "OgreEndureHandler.h"
#include "OgreGluttonyHandler.h"
#include "OgreHeavySwingHandler.h"
#include "OgreRoarHandler.h"
#include "OgreRockThrowHandler.h"

namespace wod_server {

	class CSkillHandler;
	class CClient;

	class CSkillHandlerFactory : public TSingleton<CSkillHandlerFactory>
	{
	public:
		bool Initialize() override;
		bool Release() override;

		void Handle(int _skillNum, std::shared_ptr<CClient> _client);

		void RegisterHandler(int _skillNum, CSkillHandler* _handler) { m_handlerPool[_skillNum].push(_handler); }

	private:
		std::unordered_map<int, CSkillHandler*> m_handlers;
		std::array<concurrency::concurrent_priority_queue<CSkillHandler*>, 74> m_handlerPool;
	};

}