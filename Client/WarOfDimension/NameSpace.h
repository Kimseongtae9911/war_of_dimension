#pragma once
//#include "stdafx.h"

namespace BASIC_ANI
{
	int Idle(JOB eJob)
	{
		switch (eJob)
		{
		case JOB::ARCHER:
			return 0;
		case JOB::FIGHTER:
			return 12;
		case JOB::SWORDMAN:
			return 31;
		case JOB::WIZARD:
			return 45;
		default:
			return -1;
		}
	}

	int WalkForward(JOB eJob)
	{
		switch (eJob)
		{
		case JOB::ARCHER:
			return 1;
		case JOB::FIGHTER:
			return 13;
		case JOB::SWORDMAN:
			return 32;
		case JOB::WIZARD:
			return 46;
		default:
			return -1;
		}
	}

	int WalkBack(JOB eJob)
	{
		switch (eJob)
		{
		case JOB::ARCHER:
			return 2;
		case JOB::FIGHTER:
			return 14;
		case JOB::SWORDMAN:
			return 33;
		case JOB::WIZARD:
			return 47;
		default:
			return -1;
		}
	}

	int WalkLeft(JOB eJob)
	{
		switch (eJob)
		{
		case JOB::ARCHER:
			return 3;
		case JOB::FIGHTER:
			return 15;
		case JOB::SWORDMAN:
			return 34;
		case JOB::WIZARD:
			return 48;
		default:
			return -1;
		}
	}

	int WalkRight(JOB eJob)
	{
		switch (eJob)
		{
		case JOB::ARCHER:
			return 4;
		case JOB::FIGHTER:
			return 16;
		case JOB::SWORDMAN:
			return 35;
		case JOB::WIZARD:
			return 49;
		default:
			return -1;
		}
	}

	int Death(JOB eJob)
	{
		switch (eJob)
		{
		case JOB::ARCHER:
			return 5;
		case JOB::FIGHTER:
			return 17;
		case JOB::SWORDMAN:
			return 36;
		case JOB::WIZARD:
			return 50;
		default:
			return -1;
		}
	}

	int Attack(JOB eJob)
	{
		switch (eJob)
		{
		case JOB::ARCHER:
			return 6;
		case JOB::FIGHTER:
			return 18;
		case JOB::SWORDMAN:
			return 37;
		case JOB::WIZARD:
			return 51;
		default:
			return -1;
		}
	}
};


namespace BOSS_OGRE_ANI
{
	int Idle()
	{
		return 0;
	}

	int WalkForward()
	{
		return 1;
	}

	int WalkBack()
	{
		return 4;
	}

	int WalkLeft()
	{
		return 2;
	}

	int WalkRight()
	{
		return 3;
	}

	int Attack()
	{
		return 5;
	}

	int Skill_AniNum(int skillNum)
	{
		switch (skillNum)
		{
		case 1://Heavy Swing
			return 11;
		case 2://Crunch
			return 8;
		case 3://Charging
			return 7;
		case 4://Roar
			return 12;
		case 5://Glottony
			return 10;
		case 6://Endure
			return 10;
		case 7://Rock-Throw
			return 11;
		case 8://DimensionPunch
			return 5;
		case 9://Butting
			return 6;
		case 10://DimensionCrush
			return 9;
		default:
			return - 1;
		}
	}

	int Death()
	{
		return 13;
	}
}

namespace BOSS_PROGRAMMER_ANI
{
	int Idle()
	{
		return 0;
	}

	int Attack()
	{
		return 1;
	}

	int Skill_AniNum(int skillNum)
	{
		switch (skillNum)
		{
		case 11://SwitchCaseMove
		case 12://Pointer
		case 13://++
		case 14://-- 
		case 15://Release
		case 16://Delete
		case 17://Return0
		case 19://WhileTrue
			return 1;
		case 18://SwitchCaseLaser
			return 3;	
		case 20://HelloWorld
			return 2;
		default:
			return -1;
		}
	}

	int Death()
	{
		return 4;
	}
}

namespace PARTICLE_SKILLSETTING
{
	int NumParticle(int SkillNum)//Set When Skill Packet arrive
	{
		switch (SkillNum)
		{
		case 24://Roar
			return 1;
		case 25://Gluttony
			return 1;
		case 26://Endure
			return 1;
		case 28://DimensionPunch
			return 1;
		case 29://Butting
			return 1;
		case 34://--
			return 1;
		case 35://Release
			return 1;
		case 36://Delete
			return 1;
		case 39://while true
			return 1;
		case 48://back step
			return 1;
		case 49://Dodge
			return 1;
		case 52://Vital Point
			return 1;
		case 55://Hunter Eyes
			return 1;
		case 56://wind step
			return 1;
		case 60://Fighter Dash
			return 1;
		case 61://Fighter Dodge
			return 1;
		case 63://Wild Attack
			return 1;
		case 64://명왕권
			return 1;
		case 65://운기조식
			return 1;
		case 66://Point Blood
			return 1;
		case 67://Indestructible
			return 1;
		case 69://Counter
			return 1;
		case 70://FireBall
			return 1;
		case 71://승룡각
			return 1;
		case 72://SwordManDodge
			return 1;
		case 73://RUN
			return 1;
		case 75://ShieldBash
			return 1;
		case 76://war_cry
			return 1;
		case 77://Defensive Stance
			return 1;
		case 78://Berserk
			return 1;
		case 80://Hell_blade
			return 1;
		case 84://Teleport
			return 1;
		case 85://Blink
			return 1;
		case 87://Enchant
			return 1;
		case 88://EarthImpact
			return 1;
		case 90://EnergyBall
			return 1;
		case 91://Magic Eye
			return 1;
		case 93://Reflect
			return 1;
		case 95://OverLoad
			return 1;
		default:
			return 0;
		}
	}

	int NumParticle(SKILL_TYPE Type)//Set When Add_SkillObject_Packet arrive
	{
		int ParticleNum = 0;

		switch (Type)
		{
		case SKILL_TYPE::NONE:
			break;
		case SKILL_TYPE::ARCHER_ATTACK:
			ParticleNum = 1;
			break;
		case SKILL_TYPE::ARCHER_BACKSTEP:
			break;
		case SKILL_TYPE::ARCHER_DODGE:
			break;
		case SKILL_TYPE::ARCHER_MULTIPLE_SHOT:
			break;
		case SKILL_TYPE::ARCHER_VAULT:
			break;
		case SKILL_TYPE::ARCHER_PENETRAITING_SHOT:
			ParticleNum = 1;
			break;
		case SKILL_TYPE::ARCHER_STICKY_ARROW:
			ParticleNum = 1;
			break;
		case SKILL_TYPE::ARCHER_PHOENIX_ARROW:
			ParticleNum = 1;
			break;
		case SKILL_TYPE::ARCHER_STROM_ARROW:
			ParticleNum = 1;
			break;
		case SKILL_TYPE::ARCHER_ARROW_RAIN:
			ParticleNum = 1;
			break;
		case SKILL_TYPE::FIGHTER_DODGE:
			break;
		case SKILL_TYPE::FIGTER_SPIN_KICK:
			break;
		case SKILL_TYPE::FIGHTER_WILD_ATTACK:
			break;
		case SKILL_TYPE::FIGHTER_WIND_KICK:
			break;
		case SKILL_TYPE::FIGHTER_FIREBALL:
			ParticleNum = 1;
			break;
		case SKILL_TYPE::FIGHTER_RISING_DRAGON:
			break;
		case SKILL_TYPE::FIGHTER_MEDITATION:
			break;
		case SKILL_TYPE::FIGTHER_DRAGON_FIST:
			break;
		case SKILL_TYPE::FIGHTER_INDESTRUCTIBLE:
			break;
		case SKILL_TYPE::FIGHTER_COUNTER:
			break;
		case SKILL_TYPE::SWORDMAN_DODGE:
			break;
		case SKILL_TYPE::SWORDMAN_HEAVY_SLASH:
			break;
		case SKILL_TYPE::SWORDMAN_AURA_BLADE:
			ParticleNum = 1;
			break;
		case SKILL_TYPE::SWORDMAN_HELL_BLADE:
			break;
		case SKILL_TYPE::SWORDMAN_JUDGEMENT_SWORD:
			ParticleNum = 1;
			break;
		case SKILL_TYPE::SWORDMAN_ANKLE_CUT:
			break;
		case SKILL_TYPE::SWORDMAN_SHIELD_BASH:
			break;
		case SKILL_TYPE::SWORDMAN_PROTECTED_AREA:
			break;
		case SKILL_TYPE::WIZARD_ATTACK:
			ParticleNum = 1;
			break;
		case SKILL_TYPE::WIZARD_TELEPORT:
			break;
		case SKILL_TYPE::WIZARD_EARTH_IMPACT:
			break;
		case SKILL_TYPE::WIZARD_MAGIC_MISSILE:
			ParticleNum = 1;
			break;
		case SKILL_TYPE::WIZARD_MAGIC_EYE:
			break;
		case SKILL_TYPE::WIZARD_ENERGY_BALL:
			ParticleNum = 1;
			break;
		case SKILL_TYPE::WIZARD_DARKNESS_RAY:
			ParticleNum = 1;
			break;
		case SKILL_TYPE::WIZARD_BIGBANG:
			ParticleNum = 1;
			break;
		case SKILL_TYPE::WIZARD_BIGBANG_CONTINUE:
			ParticleNum = 1;
			break;
		case SKILL_TYPE::WIZARD_REFLECT:
			break;
		case SKILL_TYPE::WIZARD_OVERLOAD:
			break;
		case SKILL_TYPE::OGRE_ATTACK:
			break;
		case SKILL_TYPE::OGRE_HEAVY_SWING:
			break;
		case SKILL_TYPE::OGRE_CRUNCH:
			break;
		case SKILL_TYPE::OGRE_CHARGING:
			break;
		case SKILL_TYPE::OGRE_ROAR:
			break;
		case SKILL_TYPE::OGRE_ENDURE:
			break;
		case SKILL_TYPE::OGRE_GLUTTONY:
			break;
		case SKILL_TYPE::OGRE_ROCK_THROW:
			break;
		case SKILL_TYPE::OGRE_BUTTING:
			break;
		case SKILL_TYPE::OGRE_DIMENSION_PUNCH:
			break;
		case SKILL_TYPE::OGRE_DIMENSION_CRUSH:
			ParticleNum = 1;
			break;
		case SKILL_TYPE::PRO_ATTACK:
			ParticleNum = 1;
			break;
		case SKILL_TYPE::PRO_POINTER:
			break;
		case SKILL_TYPE::PRO_RELEASE:
			break;
		case SKILL_TYPE::PRO_DELETE:
			break;
		case SKILL_TYPE::PRO_RETURN_ZERO:
			ParticleNum = 1;
			break;
		case SKILL_TYPE::PRO_SCL:
			ParticleNum = 1;
			break;
		case SKILL_TYPE::PRO_WHILE_TRUE:
			break;
		case SKILL_TYPE::PRO_HELLO_WORLD:
			break;
		case SKILL_TYPE::BURN:
			break;
		case SKILL_TYPE::POISON:
			break;
		case SKILL_TYPE::MEMORY_LEAK:
			break;
		case SKILL_TYPE::SILENCE:
			break;
		case SKILL_TYPE::TYPE_COUNT:
			break;
		case SKILL_TYPE::TOWERATTACK:
			ParticleNum = 1;
			break;
		default:
			break;
		}

		return ParticleNum;
	}

	PARTICLE_TYPE Type(int SkillNum, int NumParticle = 0) //Set When Skill Packet arrive
	{
		switch (SkillNum)
		{
		case 24://Roar
			return BUFF;
		case 25://Gluttony
			return BUFF;
		case 26://Endure
			return BUFF;
		case 28://DimensionPunch
			return BILLBOARDFRONT;
		case 29://Butting
			return XYEMITTER;
		case 34://--
			return ROUNDRANGE;
		case 35://Release
			return XZEMITTER;
		case 36://Delete
			return ROUNDRANGE;
		case 39://while true
			return ROUNDRANGE;
		case 48://back step
			return FOG;
		case 49://back step
			return FOG;
		case 52://Vital Point
			return BUFF;
		case 55://Hunter Eyes
			return BUFF;
		case 56://wind step
			return FOG;
		case 60://Fighter Dash
			return FOG;
		case 61://Fighter Dodge
			return FOG;
		case 63://Wild Attack
			return ATTACK;
		case 64://명왕권
			return INSIDEMOVE;
		case 65://운기조식
			return SPINSCENTER;
		case 66://Point Blood
			return BUFF;
		case 67://Indestructible
			return BUFF;
		case 69://Counter
			return ATTACK;
		case 70://FireBall
			return INSIDEMOVE;
		case 71://승룡각
			return XZEMITTER;
		case 72://SwordManDodge
			return FOG;
		case 73://RUN
			return BUFF;
		case 75://ShieldBash
			return BASH;
		case 76://war_cry
			return XYEMITTER;
		case 77://Defensive Stance
			return FOG;
		case 78://Berserk
			return FIRE;
		case 80://Hell_blade
			return BILLBOARD;
		case 84://Teleport
			return FOG;
		case 85://Blink
			return XZEMITTER;
		case 87://Enchant
			return FIRE;
		case 88://EarthImpact
			return XZEMITTER;
		case 90://EnergyBall
			return INSIDEMOVE;
		case 91://Magic Eye
			return BUFF;
		case 93://Reflect
			return SPHERE;
		case 95://OverLoad
			return TORNADO;
		default:
			return NONE;
		}
	}


	PARTICLE_TYPE Type(SKILL_TYPE Type, int NumParticle = 0)//Set When Add_SkillObject_Packet arrive
	{
		PARTICLE_TYPE ParticleType = NONE;

		switch (Type)
		{
		case SKILL_TYPE::NONE:
			break;
		case SKILL_TYPE::ARCHER_ATTACK:
			ParticleType = TAILSTAR;
			break;
		case SKILL_TYPE::ARCHER_BACKSTEP:
			break;
		case SKILL_TYPE::ARCHER_DODGE:
			break;
		case SKILL_TYPE::ARCHER_MULTIPLE_SHOT:
			break;
		case SKILL_TYPE::ARCHER_VAULT:
			break;
		case SKILL_TYPE::ARCHER_PENETRAITING_SHOT:
			ParticleType = TAILSTAR;
			break;
		case SKILL_TYPE::ARCHER_STICKY_ARROW:
			ParticleType = TAILSTAR;
			break;
		case SKILL_TYPE::ARCHER_PHOENIX_ARROW:
			ParticleType = PHOENIX;
			break;
		case SKILL_TYPE::ARCHER_STROM_ARROW:
			ParticleType = STORMARROW;
			break;
		case SKILL_TYPE::ARCHER_ARROW_RAIN:
			ParticleType = DROPARROW;
			break;
		case SKILL_TYPE::FIGHTER_DODGE:
			break;
		case SKILL_TYPE::FIGTER_SPIN_KICK:
			break;
		case SKILL_TYPE::FIGHTER_WILD_ATTACK:
			break;
		case SKILL_TYPE::FIGHTER_WIND_KICK:
			break;
		case SKILL_TYPE::FIGHTER_FIREBALL:
			ParticleType = BALL;
			break;
		case SKILL_TYPE::FIGHTER_RISING_DRAGON:
			break;
		case SKILL_TYPE::FIGHTER_MEDITATION:
			break;
		case SKILL_TYPE::FIGTHER_DRAGON_FIST:
			break;
		case SKILL_TYPE::FIGHTER_INDESTRUCTIBLE:
			break;
		case SKILL_TYPE::FIGHTER_COUNTER:
			break;
		case SKILL_TYPE::SWORDMAN_DODGE:
			break;
		case SKILL_TYPE::SWORDMAN_HEAVY_SLASH:
			break;
		case SKILL_TYPE::SWORDMAN_AURA_BLADE:
			ParticleType = AURABLADE;
			break;
		case SKILL_TYPE::SWORDMAN_HELL_BLADE:
			break;
		case SKILL_TYPE::SWORDMAN_JUDGEMENT_SWORD:
			ParticleType = BILLBOARD;
			break;
		case SKILL_TYPE::SWORDMAN_ANKLE_CUT:
			break;
		case SKILL_TYPE::SWORDMAN_SHIELD_BASH:
			break;
		case SKILL_TYPE::SWORDMAN_PROTECTED_AREA:
			break;
		case SKILL_TYPE::WIZARD_ATTACK:
			ParticleType = MAGICBALL;
			break;
		case SKILL_TYPE::WIZARD_TELEPORT:
			break;
		case SKILL_TYPE::WIZARD_EARTH_IMPACT:
			break;
		case SKILL_TYPE::WIZARD_MAGIC_MISSILE:
			ParticleType = TAILSTAR;
			break;
		case SKILL_TYPE::WIZARD_MAGIC_EYE:
			break;
		case SKILL_TYPE::WIZARD_ENERGY_BALL:
			ParticleType = BALL;
			break;
		case SKILL_TYPE::WIZARD_DARKNESS_RAY:
			ParticleType = RAY;
			break;
		case SKILL_TYPE::WIZARD_BIGBANG:
			ParticleType = MAGICBALL;
			break;
		case SKILL_TYPE::WIZARD_BIGBANG_CONTINUE:
			ParticleType = BIGBANG;
			break;
		case SKILL_TYPE::WIZARD_REFLECT:
			break;
		case SKILL_TYPE::WIZARD_OVERLOAD:
			break;
		case SKILL_TYPE::OGRE_ATTACK:
			break;
		case SKILL_TYPE::OGRE_HEAVY_SWING:
			break;
		case SKILL_TYPE::OGRE_CRUNCH:
			break;
		case SKILL_TYPE::OGRE_CHARGING:
			break;
		case SKILL_TYPE::OGRE_ROAR:
			break;
		case SKILL_TYPE::OGRE_ENDURE:
			break;
		case SKILL_TYPE::OGRE_GLUTTONY:
			break;
		case SKILL_TYPE::OGRE_ROCK_THROW:
			break;
		case SKILL_TYPE::OGRE_BUTTING:
			break;
		case SKILL_TYPE::OGRE_DIMENSION_PUNCH:
			break;
		case SKILL_TYPE::OGRE_DIMENSION_CRUSH:
			ParticleType = CRUSH;
			break;
		case SKILL_TYPE::PRO_ATTACK:
			ParticleType = MAGICBALL;
			break;
		case SKILL_TYPE::PRO_POINTER:
			break;
		case SKILL_TYPE::PRO_RELEASE:
			break;
		case SKILL_TYPE::PRO_DELETE:
			break;
		case SKILL_TYPE::PRO_RETURN_ZERO:
			ParticleType = MAGICBALL;
			break;
		case SKILL_TYPE::PRO_SCL:
			ParticleType = RAY;
			break;
		case SKILL_TYPE::PRO_WHILE_TRUE:
			break;
		case SKILL_TYPE::PRO_HELLO_WORLD:
			break;
		case SKILL_TYPE::BURN:
			break;
		case SKILL_TYPE::POISON:
			break;
		case SKILL_TYPE::MEMORY_LEAK:
			break;
		case SKILL_TYPE::SILENCE:
			break;
		case SKILL_TYPE::TOWERATTACK:
			ParticleType = MAGICBALL;
			break;
		case SKILL_TYPE::TYPE_COUNT:
			break;
		default:
			break;
		}

		return ParticleType;
	}

	int TextureAddress(int SkillNum, int NumParticle = 0) //Set When Skill Packet arrive
	{
		switch (SkillNum)
		{
		case 24://Roar
			return PARTICLE_ADDRESS::NOISE;
		case 25://Gluttony
			return PARTICLE_ADDRESS::NOISE;
		case 26://Endure
			return PARTICLE_ADDRESS::NOISE;
		case 28://DimensionPunch
			return PARTICLE_ADDRESS::DIMENSION;
		case 29://Butting
			return PARTICLE_ADDRESS::ROUND;
		case 34://--
			return PARTICLE_ADDRESS::ROUND;
		case 35://Release
			return PARTICLE_ADDRESS::ROUND;
		case 36://Delete
			return PARTICLE_ADDRESS::ROUND;
		case 39://while true
			return PARTICLE_ADDRESS::ROUND;
		case 48://back step
			return PARTICLE_ADDRESS::NOISE;
		case 49://Dodge
			return PARTICLE_ADDRESS::NOISE;
		case 52://Vital Point
			return PARTICLE_ADDRESS::ROUND;
		case 55://Hunter Eyes
			return PARTICLE_ADDRESS::DUST;
		case 56://wind step
			return PARTICLE_ADDRESS::ROUND;
		case 60://Fighter Dash
			return PARTICLE_ADDRESS::NOISE;
		case 61://Fighter Dodge
			return PARTICLE_ADDRESS::NOISE;
		case 63://Wild Attack
			return PARTICLE_ADDRESS::FIGHTATTACK;
		case 64://명왕권
			return PARTICLE_ADDRESS::ROUND;
		case 65://운기조식
			return PARTICLE_ADDRESS::ROUND;
		case 66://Point Blood
			return PARTICLE_ADDRESS::NOISE;
		case 67://Indestructible
			return PARTICLE_ADDRESS::ROUND;
		case 69://Counter
			return PARTICLE_ADDRESS::COUNTER;
		case 70://FireBall
			return PARTICLE_ADDRESS::ROUND;
		case 71://승룡각
			return PARTICLE_ADDRESS::ROUND;
		case 72://SwordManDodge
			return PARTICLE_ADDRESS::NOISE;
		case 73://RUN
			return PARTICLE_ADDRESS::NOISE;
		case 75://ShieldBash
			return  PARTICLE_ADDRESS::ROUND;
		case 76://war_cry
			return PARTICLE_ADDRESS::ROUND;
		case 77://Defensive Stance
			return PARTICLE_ADDRESS::SHIELD;
		case 78://Berserk
			return PARTICLE_ADDRESS::ROUND;
		case 80://Hell_blade
			return PARTICLE_ADDRESS::HELLBLADE;
		case 84://Teleport
			return PARTICLE_ADDRESS::ROUND;
		case 85://Blink
			return PARTICLE_ADDRESS::ROUND;
		case 87://Enchant
			return PARTICLE_ADDRESS::ROUND;
		case 88://EarthImpact
			return PARTICLE_ADDRESS::ROUND;
		case 90://EnergyBall
			return PARTICLE_ADDRESS::ROUND;
		case 91://Magic Eye
			return PARTICLE_ADDRESS::NOISE;
		case 93://Reflect
			return PARTICLE_ADDRESS::REFLECT;
		case 95://OverLoad
			return PARTICLE_ADDRESS::ROUND;
		default:
			return PARTICLE_ADDRESS::ADDRESS_COUNT;
		}
	}



	int TextureAddress(SKILL_TYPE Type, int NumParticle = 0) // Set When Add_SkillObject_Packet arrive
	{
		int enumAddress = PARTICLE_ADDRESS::ADDRESS_COUNT;

		switch (Type)
		{
		case SKILL_TYPE::NONE:
			break;
		case SKILL_TYPE::ARCHER_ATTACK:
			enumAddress = PARTICLE_ADDRESS::ROUND;
			break;
		case SKILL_TYPE::ARCHER_BACKSTEP:
			break;
		case SKILL_TYPE::ARCHER_DODGE:
			break;
		case SKILL_TYPE::ARCHER_MULTIPLE_SHOT:
			break;
		case SKILL_TYPE::ARCHER_VAULT:
			break;
		case SKILL_TYPE::ARCHER_PENETRAITING_SHOT:
			enumAddress = PARTICLE_ADDRESS::ROUND;
			break;
		case SKILL_TYPE::ARCHER_STICKY_ARROW:
			enumAddress = PARTICLE_ADDRESS::ROUND;
			break;
		case SKILL_TYPE::ARCHER_PHOENIX_ARROW:
			enumAddress = PARTICLE_ADDRESS::ROUND;
			break;
		case SKILL_TYPE::ARCHER_STROM_ARROW:
			enumAddress = PARTICLE_ADDRESS::ROUND;
			break;
		case SKILL_TYPE::ARCHER_ARROW_RAIN:
			enumAddress = PARTICLE_ADDRESS::ARROW;
			break;
		case SKILL_TYPE::FIGHTER_DODGE:
			break;
		case SKILL_TYPE::FIGTER_SPIN_KICK:
			break;
		case SKILL_TYPE::FIGHTER_WILD_ATTACK:
			break;
		case SKILL_TYPE::FIGHTER_WIND_KICK:
			break;
		case SKILL_TYPE::FIGHTER_FIREBALL:
			enumAddress = PARTICLE_ADDRESS::ROUND;
			break;
		case SKILL_TYPE::FIGHTER_RISING_DRAGON:
			break;
		case SKILL_TYPE::FIGHTER_MEDITATION:
			break;
		case SKILL_TYPE::FIGTHER_DRAGON_FIST:
			break;
		case SKILL_TYPE::FIGHTER_INDESTRUCTIBLE:
			break;
		case SKILL_TYPE::FIGHTER_COUNTER:
			break;
		case SKILL_TYPE::SWORDMAN_DODGE:
			break;
		case SKILL_TYPE::SWORDMAN_HEAVY_SLASH:
			break;
		case SKILL_TYPE::SWORDMAN_AURA_BLADE:
			enumAddress = PARTICLE_ADDRESS::ROUND;
			break;
		case SKILL_TYPE::SWORDMAN_HELL_BLADE:
			break;
		case SKILL_TYPE::SWORDMAN_JUDGEMENT_SWORD:
			enumAddress = PARTICLE_ADDRESS::SWORD;
			break;
		case SKILL_TYPE::SWORDMAN_ANKLE_CUT:
			break;
		case SKILL_TYPE::SWORDMAN_SHIELD_BASH:
			break;
		case SKILL_TYPE::SWORDMAN_PROTECTED_AREA:
			break;
		case SKILL_TYPE::WIZARD_ATTACK:
			enumAddress = PARTICLE_ADDRESS::ROUND;
			break;
		case SKILL_TYPE::WIZARD_TELEPORT:
			break;
		case SKILL_TYPE::WIZARD_EARTH_IMPACT:
			break;
		case SKILL_TYPE::WIZARD_MAGIC_MISSILE:
			enumAddress = PARTICLE_ADDRESS::ROUND;
			break;
		case SKILL_TYPE::WIZARD_MAGIC_EYE:
			break;
		case SKILL_TYPE::WIZARD_ENERGY_BALL:
			enumAddress = PARTICLE_ADDRESS::ROUND;
			break;
		case SKILL_TYPE::WIZARD_DARKNESS_RAY:
			enumAddress = PARTICLE_ADDRESS::ROUND;
			break;
		case SKILL_TYPE::WIZARD_BIGBANG:
			enumAddress = PARTICLE_ADDRESS::ROUND;
			break;
		case SKILL_TYPE::WIZARD_BIGBANG_CONTINUE:
			enumAddress = PARTICLE_ADDRESS::ROUND;
			break;
		case SKILL_TYPE::WIZARD_REFLECT:
			break;
		case SKILL_TYPE::WIZARD_OVERLOAD:
			break;
		case SKILL_TYPE::OGRE_ATTACK:
			break;
		case SKILL_TYPE::OGRE_HEAVY_SWING:
			break;
		case SKILL_TYPE::OGRE_CRUNCH:
			break;
		case SKILL_TYPE::OGRE_CHARGING:
			break;
		case SKILL_TYPE::OGRE_ROAR:
			break;
		case SKILL_TYPE::OGRE_ENDURE:
			break;
		case SKILL_TYPE::OGRE_GLUTTONY:
			break;
		case SKILL_TYPE::OGRE_ROCK_THROW:
			break;
		case SKILL_TYPE::OGRE_BUTTING:
			break;
		case SKILL_TYPE::OGRE_DIMENSION_PUNCH:
			break;
		case SKILL_TYPE::OGRE_DIMENSION_CRUSH:
			enumAddress = PARTICLE_ADDRESS::ROUND;
			break;
		case SKILL_TYPE::PRO_ATTACK:
			enumAddress = PARTICLE_ADDRESS::ROUND;
			break;
		case SKILL_TYPE::PRO_POINTER:
			break;
		case SKILL_TYPE::PRO_RELEASE:
			break;
		case SKILL_TYPE::PRO_DELETE:
			break;
		case SKILL_TYPE::PRO_RETURN_ZERO:
			enumAddress = PARTICLE_ADDRESS::ROUND;
			break;
		case SKILL_TYPE::PRO_SCL:
			enumAddress = PARTICLE_ADDRESS::ROUND;
			break;
		case SKILL_TYPE::PRO_WHILE_TRUE:
			break;
		case SKILL_TYPE::PRO_HELLO_WORLD:
			break;
		case SKILL_TYPE::BURN:
			break;
		case SKILL_TYPE::POISON:
			break;
		case SKILL_TYPE::MEMORY_LEAK:
			break;
		case SKILL_TYPE::SILENCE:
			break;
		case SKILL_TYPE::TOWERATTACK:
			enumAddress = PARTICLE_ADDRESS::ROUND;
			break;
		case SKILL_TYPE::TYPE_COUNT:
			break;
		default:
			break;
		}

		return enumAddress;
	}

}