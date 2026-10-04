#pragma once
enum class OP_TYPE { OP_ACCEPT, OP_RECV, OP_SEND, OP_DISCONNECT, OP_CONNECT_UPDATE, OP_READY_UPDATE, OP_LOADING_UPDATE, OP_MATCH_UPDATE, OP_MONSTER_RESPAWN, OP_MATCH_FINISH, OP_HEALTHMANA_CHANGE, OP_NPC_ACTIVE};

enum class CL_STATE { ST_FREE, ST_ALLOC, ST_READY, ST_INGAME };

enum class NPC_STATE {ST_IDLE, ST_MOVE, ST_CHASE, ST_RETURN, ST_ATTACK, ST_MOVETO_STRUCTURE, ST_ATTACK_STRUCTURE, ST_DIE};

enum class DAMAGE_TYPE { STRENGTH, MAGIC };

enum class DAMAGE_BUFF { NONE, FIRE, ICE, THUNDER, POISON, GLUTTONY };

enum class COOLTIME_BUFF { NONE, OVERLOAD };

enum class DEFENSIVE_BUFF { NONE, REFLECT, DEFENSIVE_STANCE, INDESTRUCTIBLE, COUNTER, ENDURE };

enum class ECharacterType : unsigned char { None = 0, Archer = 1, Fighter = 2, Ogre = 3, Programmer = 4, SwordMan = 5, Wizard = 6, End = Wizard + 1, Begin = None };

enum class ESkillType : unsigned char {None = 0, Attack = 1, Buff = 2, Debuff = 3, Move = 4};

enum class ESkillName : short { None = 0, 
	ArcherAttack = 1, ArrowRain = 2, HunterEyes = 3, MultipleShot = 4, PenetratingShot = 5, PhoenixArrow = 6, StickyArrow = 7, StormArrow = 8, Vault = 9, WindStep = 10, BackStep = 11, Dodge = 12,
};