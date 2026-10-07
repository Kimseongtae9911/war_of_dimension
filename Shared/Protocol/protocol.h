#pragma once
#include <WS2tcpip.h>
#include <chrono>

//#define WITH_DATABASE
#define LOCAL_TEST

constexpr int BUF_SIZE  = 256;
constexpr int NAME_SIZE =  10;
constexpr int CHAT_SIZE =  30;
constexpr int DATE_SIZE =  17;
constexpr int TIME_SIZE = 20;

constexpr int AUCTION_DATA_NUM = 7;

//Monster Constants
constexpr int UNIQUE_KILL_BUFF_TIME = 90000;
constexpr int GOLEM_KILL_BUFF_TIME = 60000;
constexpr int GREEN_KILL_BUFF_TIME = 60000;

//Item Constants
constexpr int ITEM_NUM = 5;
constexpr int HEAL_ITEM_GOLD = 50;
constexpr int STAT_ITEM_GOLD = 100;

constexpr int LOBBY_PORT = 8910;
constexpr int GAME_PORT = 8911;

constexpr int LOBBY_MAX_CLIENT = 4;		//For test 100
constexpr int MONSTER_NUM = 9;
#define MINION_WAVE 4
constexpr int MAX_MINION = 12;

constexpr int PATH_NUM = 4;

//Skill Defines
constexpr int MAX_JOB = 4;
constexpr int MAX_SKILL_OBJECT = 5;
constexpr int MAX_SKILL = 4;
constexpr int PLAYER_SKILL = 48 * 4;
constexpr int BOSS_SKILL = 20 * 2;

//Shop Define
#define GET_SHOP_PRICE(currentPrice, level) ((currentPrice) + 0.2 * 10 * (level))
constexpr float SHOP_POS_X = -137.6804f;
constexpr float SHOP_POS_Z = -140.7198f;
constexpr float BOSS_SHOP_POS_X = 17.72f;
constexpr float BOSS_SHOP_POS_Z = 5.9f;
constexpr float SHOP_DISTANCE = 10.0f;

//Customize Constants
constexpr int CUSTOMIZE_PART_NUM_FROM_SERVER = 83;

enum class SKILL_TYPE {
	NONE,
	ARCHER_ATTACK, ARCHER_BACKSTEP, ARCHER_DODGE, ARCHER_MULTIPLE_SHOT, ARCHER_VAULT, ARCHER_PENETRAITING_SHOT, ARCHER_STICKY_ARROW, ARCHER_PHOENIX_ARROW, ARCHER_STROM_ARROW, ARCHER_ARROW_RAIN,
	FIGHTER_DODGE, FIGTER_SPIN_KICK, FIGHTER_WILD_ATTACK, FIGHTER_WIND_KICK, FIGHTER_FIREBALL, FIGHTER_RISING_DRAGON, FIGHTER_MEDITATION, FIGTHER_DRAGON_FIST, FIGHTER_INDESTRUCTIBLE, FIGHTER_COUNTER,
	SWORDMAN_DODGE, SWORDMAN_HEAVY_SLASH, SWORDMAN_AURA_BLADE, SWORDMAN_HELL_BLADE, SWORDMAN_JUDGEMENT_SWORD, SWORDMAN_ANKLE_CUT, SWORDMAN_SHIELD_BASH, SWORDMAN_PROTECTED_AREA,
	WIZARD_ATTACK, WIZARD_TELEPORT, WIZARD_EARTH_IMPACT, WIZARD_MAGIC_MISSILE, WIZARD_MAGIC_EYE, WIZARD_ENERGY_BALL, WIZARD_DARKNESS_RAY, WIZARD_BIGBANG, WIZARD_BIGBANG_CONTINUE, WIZARD_REFLECT, WIZARD_OVERLOAD,
	OGRE_ATTACK, OGRE_HEAVY_SWING, OGRE_CRUNCH, OGRE_CHARGING, OGRE_ROAR, OGRE_ENDURE, OGRE_GLUTTONY, OGRE_ROCK_THROW, OGRE_BUTTING, OGRE_DIMENSION_PUNCH, OGRE_DIMENSION_CRUSH,
	PRO_ATTACK, PRO_POINTER, PRO_RELEASE, PRO_DELETE, PRO_RETURN_ZERO, PRO_SCL, PRO_WHILE_TRUE, PRO_HELLO_WORLD,
	BURN, POISON, MEMORY_LEAK, SILENCE, STUN, TOWERATTACK, TYPE_COUNT
};

enum class NPC_TYPE { MINION, UNIQUE_DRAGON, RARE_GREEN_DRAGON, RARE_GOLEM, NORMAL_BEAR, NORMAL_MINOTAUR, NORMAL_CHEST, NORMAL_BEHOLDER };

enum class SKILL_BUFF { NONE, SILENCE, STUN };

enum class SHOP_TYPE { HEAD_COVERING_BASE_HAIR, HEAD_COVERING_FACIAL_HAIR, HEAD_COVERING_NO_HAIR, HAIR,
	HELMET_ATTACHMENT, BACK_ATTACHMENT, SHOULDER_ATTACHMENT_R, SHOULDER_ATTACHMENT_L, ELLBOW_ATTACHMENT_L, ELLBOW_ATTACHMENT_R, HIP_ATTACHMENT, KNEE_ATTACHMENT_R, KNEE_ATTACHMENT_L,
	EAR,
	HEAD, HEAD_NO_ELEMENT,
	EYEBROW,
	TORSO,
	ARM_UPPER_R, ARM_UPPER_L, ARM_LOWER_R, ARM_LOWER_L,
	HAND_R, HAND_L,
	HIPS,
	LEG_R, LEG_L,
	COUNT
};

enum class ITEMKIND
{
	HEALHP,
	HEALMP,
	HP,
	MP,
	ATTACK,
	MATTACK,
	DEFENSE,
	MDEFENSE,
	SPEED,
	TENACITY,
	CRITICAL,
	NONE
};

// Player protocol
constexpr float PLAYER_SPEED	 =	3.f;
constexpr float PLAYER_MAX_VELXZ =	6.0f;
constexpr float PLAYER_MAX_VELY  =	50.0f;
#define PLAYER_SCALE 0.75f
#define OGRE_SCALE 1.25f

// Minion Protocol
constexpr float MINON_SPEED = 2.f;
constexpr float MINION_MAX_VELXZ = 4.0f;
#define MINION_SCALE 0.3f

//Monster Protocol
constexpr float MONSTER_SPEED = 2.f;
constexpr float MONSTER_MAX_VELXZ = 5.0f;
constexpr float UNIQUE_RED_SCALE = 1.0f;
constexpr float RARE_GREEN_SCALE = 0.5f;
constexpr float RARE_GOLEM_SCALE = 0.4f;
constexpr float NORMAL_BEAR_SCALE = 0.8f;
constexpr float NORMAL_MINOTAUR_SCALE = 0.8f;
constexpr float NORMAL_CHEST_SCALE = 1.0f;
constexpr float NORMAL_BEHOLDER_SCALE = 1.0f;

constexpr float TELEPORT_INTERACTION_DISTANCE = 6.f;
constexpr float TOWER_INTERACTION_DISTANCE = 5.f;

constexpr float WORLD_FRICTION	 =  250.0f;

#define DIR_FORWARD					0x01
#define DIR_BACKWARD				0x02
#define DIR_LEFT					0x04
#define DIR_RIGHT					0x08
#define DIR_UP						0x10
#define DIR_DOWN					0x20

// Packet
constexpr char CS_LOGIN = 1;
constexpr char CS_MOVE = 2;
constexpr char CS_ROTATE = 3;
constexpr char CS_MATCH = 4;
constexpr char CS_MATHCH_END = 5;
constexpr char CS_CHAT = 6;
constexpr char CS_SKILL = 7;
constexpr char CS_LOAD_COMPLETE = 8;
constexpr char CS_SKILL_SELECT = 9;
constexpr char CS_READY = 10;
constexpr char CS_JOB_SELECT = 11;
constexpr char CS_SKILL_FINISH = 12;
constexpr char CS_CUSTOMIZE = 13;
constexpr char CS_JUMP = 14;
constexpr char CS_TELEPORT = 15;
constexpr char CS_MINION_PATH = 16;
constexpr char CS_TOWER_ACTIVATE = 17;
constexpr char CS_NPC_ATTACK_FINISH = 18;
constexpr char CS_SHOP = 19;
constexpr char CS_SIGN_UP = 20;
constexpr char CS_REGISTER_AUCTION = 21;
constexpr char CS_GET_AUCTION_INFO = 22;
constexpr char CS_CHANGE_CHANNEL = 23;
constexpr char CS_PORT_NUM = 24;
constexpr char CS_STAKE_TOKEN = 25;
constexpr char CS_CHANGE_NODE = 26;
constexpr char CS_STAT_SELECT = 27;
constexpr char CS_BUY_ITEM = 28;
constexpr char CS_BUY_STAT = 29;
constexpr char CS_USE_ITEM = 30;
constexpr char CS_DUMMY_CLIENT = 31;
constexpr char CS_CREATE_TRANSACTION = 32;
constexpr char CS_OPEN_AUCTION = 33;
constexpr char CS_OPEN_CUSTOMIZE = 34;
constexpr char CS_OPEN_BLOCKCHAIN = 35;
constexpr char CS_BUY_AUCTION = 36;
constexpr char CS_DEBUG_GOLD = 37;
constexpr char CS_LOGIN_COMPLETE = 38;
constexpr char CS_RTT = 39;

constexpr char SC_LOGIN_INFO = 1;
constexpr char SC_ADD_PLAYER = 2;
constexpr char SC_REMOVE_PLAYER = 3;
constexpr char SC_MOVE_PLAYER = 4;
constexpr char SC_MATCH_PLAYER = 5;
constexpr char SC_MATCH_END = 6;
constexpr char SC_CHAT = 7;
constexpr char SC_ROTATE_PLAYER = 8;
constexpr char SC_COOLTIME = 9;
constexpr char SC_SKILL = 10;
constexpr char SC_ADD_NPC = 11;
constexpr char SC_SKILL_SELECT = 12;
constexpr char SC_GAME_TIME = 13;
constexpr char SC_READY = 14;
constexpr char SC_JOB_SELECT = 15;
constexpr char SC_GAME_START = 16;
constexpr char SC_MODEL_CUSTOMIZE = 17;
constexpr char SC_MOVE_NPC = 18;
constexpr char SC_TELEPORT = 19;
constexpr char SC_TOWER_ATTACK = 20;
constexpr char SC_TOWER_ATTACK_REMOVE = 21;
constexpr char SC_TOWER_ATTACK_ADD = 22;
constexpr char SC_NPC_STAT_CHANGE = 23;
constexpr char SC_REMOVE_NPC = 24;
constexpr char SC_ADD_SKILL_OBJECT = 25;
constexpr char SC_UPDATE_SKILL_OBJECT = 26;
constexpr char SC_REMOVE_SKILL_OBJECT = 27;
constexpr char SC_NPC_ATTACK = 28;
constexpr char SC_DAMAGE_PLAYER = 29;
constexpr char SC_PLAYER_STAT_CHANGE = 30;
constexpr char SC_GIVE_GOLD = 31;
constexpr char SC_STRUCTURE_STAT = 32;
constexpr char SC_PLAYER_STATUS_CHANGE = 33;
constexpr char SC_LOGIN_FAIL = 34;
constexpr char SC_SHOP = 35;
constexpr char SC_AUCTION_INFO = 36;
constexpr char SC_SKILL_FINISH = 37;
constexpr char SC_HEALTH_MANA = 38;
constexpr char SC_PLAYER_RESPWAN = 39;
constexpr char SC_STRUCTURE_STAT_CHANGE = 40;
constexpr char SC_STRUCTURE_STATUS_CHANGE = 41;
constexpr char SC_CHANGE_CHANNEL = 42;
constexpr char SC_PEER_INFO = 43;
constexpr char SC_CHANGE_NODE = 44;
constexpr char SC_STAKE_TOKEN = 45;
constexpr char SC_TOKEN_NUM = 46;
constexpr char SC_TRANSACTION = 47;
constexpr char SC_MAGIC_EYE_POS = 48;
constexpr char SC_BLOCK_HEADER = 49;
constexpr char SC_BLOCK_BODY = 50;
constexpr char SC_FULLNODE = 51;
constexpr char SC_DUMMY_LOGIN_INFO = 52;
constexpr char SC_TELEPORT_ACTIVE = 53;
constexpr char SC_GAME_OVER = 54;
constexpr char SC_MONSTER_KILL_BUFF = 55;
constexpr char SC_JUMP_FINISH = 56;
constexpr char SC_CUSTOMIZE_PARTS = 57;
constexpr char SC_AUCTION_PARTS_NUM = 58;
constexpr char SC_STAKE_TOKEN_INFO = 59;
constexpr char SC_RTT = 60;

//Lobby To Game
constexpr char LG_MATCH_START = 1;
constexpr char LG_MATCH_PLAYER = 2;

//Game To Lobby
constexpr char GL_TRANSACTIONS = 1;

#pragma pack(push, 1)
struct ModelCustomize
{
	short Chr_Sex = 0;

	short Chr_HeadCoverings_Base_Hair = 0;
	short Chr_HeadCoverings_No_FacialHair = 0;
	short Chr_HeadCoverings_No_Hair = 0;
	short Chr_Hair = 0;
	short Chr_HelmetAttachment = 0;
	short Chr_BackAttachment = 0;
	short Chr_ShoulderAttachRight = 0;
	short Chr_ShoulderAttachLeft = 0;
	short Chr_ElbowAttachRight = 0;
	short Chr_ElbowAttachLeft = 0;
	short Chr_HipsAttachment = 0;
	short Chr_KneeAttachRight = 0;
	short Chr_KneeAttachLeft = 0;
	short Chr_Ear_Ear = 0;

	short Chr_Head = 0;
	short Chr_Head_No_Elements = 0;
	short Chr_Eyebrow = 0;
	short Chr_Torso = 0;
	short Chr_ArmUpperRight = 0;
	short Chr_ArmUpperLeft = 0;
	short Chr_ArmLowerRight = 0;
	short Chr_ArmLowerLeft = 0;
	short Chr_HandRight = 0;
	short Chr_HandLeft = 0;
	short Chr_Hips = 0;
	short Chr_LegRight = 0;
	short Chr_LegLeft = 0;
};

struct AuctionInfo {
	char playerName[NAME_SIZE];
	short customizeType;
	short customizeNum;
	unsigned short buyPrice;
	char deadLine[TIME_SIZE];
};

struct TransactionData {
	char name[10];
	short token;
	char time[20];
};

struct BASE_PACKET {
	unsigned char size;
	char type;
};

struct CS_LOGIN_PACKET : BASE_PACKET {
	char name[NAME_SIZE];
	char password[NAME_SIZE];
};

struct CS_MOVE_PACKET : BASE_PACKET {
	char direction;	//0:Stop 1:Forward 2:Backward 4:Left 5:Forward-Left 6:Backward-Left 8:Right 9:Forward-Right 10:Backward-Right
	std::chrono::high_resolution_clock::time_point move_time;
};

struct CS_ROTATE_PACKET : BASE_PACKET {
	float lookX, lookY, lookZ;
	float rightX, rightY, rightZ;
};

struct CS_MATCH_PACKET : BASE_PACKET {
	bool match;
	char character; //0: normal, 1: boss
	unsigned int match_time;
};

struct CS_MATCH_END_PACKET : BASE_PACKET {
};

struct CS_CHAT_PACKET : BASE_PACKET {
	int id;
	char name[NAME_SIZE];
	WCHAR chat[CHAT_SIZE];
	char chatType;	//0:All 1:Channel 2:Party
};

struct CS_SKILL_PACKET : BASE_PACKET {
	int id;
	char skillType;	// 1:LEFTCLICK, 2:RIGHTCLICK, 3:SHIFT, 4:Q, 5:R
	bool onOff;
};

struct CS_LOAD_COMPLETE_PACKET : BASE_PACKET {
	int id;
};

struct CS_SKILL_SELECT_PACKET : BASE_PACKET {
	int id;
	int storage;
	int skill;
};

struct CS_READY_PACKET : BASE_PACKET {
	int id;
	bool ready;
};

struct CS_JOB_SELECT_PACKET : BASE_PACKET {
	int id;
	short job;	// 0:Archer, 1:Figther, 2:Swordman, 3:Wizard, 4:Ogre, 5:Programmer
};

struct CS_SKILL_FINISH_PACKET : BASE_PACKET {

};

struct CS_CUSTOMIZE_PACKET : BASE_PACKET {
	ModelCustomize model;
};

struct CS_JUMP_PACKET : BASE_PACKET {

};

struct CS_TELEPORT_PACKET : BASE_PACKET {

};

struct CS_MINION_PATH_PACKET : BASE_PACKET {
	int path;
};

struct CS_TOWER_ACTIVATE_PACKET : BASE_PACKET {
	int num;
};

struct CS_NPC_ATTACK_FINISH_PACKET : BASE_PACKET {
	short id;
};

struct CS_SHOP_PACKET : BASE_PACKET {
	int shopType;
};

struct CS_SIGN_UP_PACKET : BASE_PACKET {
	char name[NAME_SIZE];
	char password[NAME_SIZE];
};

struct CS_REGISTER_AUCTION_PACKET : BASE_PACKET {
	char name[NAME_SIZE];
	short customizeType;
	short customizeNum;
	int buyPrice;
};

struct CS_GET_AUCTION_INFO_PACKET : BASE_PACKET {
	int pageNum;
};

struct CS_CHANGE_CHANNEL_PACKET : BASE_PACKET {
	short channel;
};

struct CS_PORT_NUM_PACKET : BASE_PACKET {
	unsigned short portNum;
};

struct CS_STAKE_TOKEN_PACKET : BASE_PACKET {
	int stakeNum;
	unsigned short stakeDays;
};

struct CS_CHANGE_NODE_PACKET : BASE_PACKET {
	bool fullNode; //true: Full Node, false: Light Node
};

struct CS_STAT_SELECT_PACKET : BASE_PACKET {
	int hp;
	int mp;
	int attack;
	int magic_attack;
	int defense;
	int magic_defense;
	int speed;
	int tenacity;
	int critical;
};

struct CS_BUY_ITEM_PACKET : BASE_PACKET {
	char itemType;
};

struct CS_BUY_STAT_PACKET : BASE_PACKET {
	char statType;
};

struct CS_USE_ITEM_PACKET : BASE_PACKET {
	short itemNum;
};

struct CS_DUMMY_CLIENT_PACKET : BASE_PACKET {
	int id;
};

struct CS_CREATE_TRANSACTION_PACKET : BASE_PACKET {
	char name[NAME_SIZE];
};

struct CS_OPEN_AUCTION_PACKET : BASE_PACKET {

};

struct CS_OPEN_CUSTOMIZE_PACKET : BASE_PACKET {

};

struct CS_OPEN_BLOCKCHAIN_PACKET : BASE_PACKET {

};

struct CS_BUY_AUCTION_PACKET : BASE_PACKET {
	char sellerName[NAME_SIZE];
	short customizeType;
	short customizeNum;
	unsigned short buyPrice;
};

struct CS_DEBUG_GOLD_PACKET : BASE_PACKET {

};

struct CS_LOGIN_COMPLETE_PACKET : BASE_PACKET {

};

struct CS_RTT_PACKET : BASE_PACKET {
	long long time;
	long long serverTime;
};


struct SC_LOGIN_INFO_PACKET : BASE_PACKET {
	int		id;
	float	x, y, z;
	ModelCustomize model;
};

struct SC_ADD_PLAYER_PACKET : BASE_PACKET {
	int id;
	float lookX, lookY, lookZ;
	float rightX, rightY, rightZ;
	ModelCustomize model;
};

struct SC_REMOVE_PLAYER_PACKET : BASE_PACKET {
	int id;
};

struct SC_MOVE_PLAYER_PACKET : BASE_PACKET {
	int id;
	float x, y, z;
	char direction;
	long long move_time;
};

struct SC_MOVE_NPC_PACKET : BASE_PACKET {
	int id;
	int npcType;
	float x, y, z;
	float lookX, lookY, lookZ;
	float rightX, rightY, rightZ;
	bool idle;
};

struct SC_MATCH_PACKET : BASE_PACKET {
	int id;
	char gameip[INET_ADDRSTRLEN];
	short gameport;
};

struct SC_MATCH_END_PACKET : BASE_PACKET {
	char lobbyip[INET_ADDRSTRLEN];
	short lobbyport;
	short earnToken;
	bool win;
};

struct SC_CHAT_PACKET : BASE_PACKET {
	char name[NAME_SIZE];
	WCHAR chat[CHAT_SIZE];
	char chatType;	//0:All 1:Channel 2:Party 3:Ready
};

struct SC_ROTATE_PLAYER_PACKET : BASE_PACKET {
	int id;
	float lookX, lookY, lookZ;
	float rightX, rightY, rightZ;
};

struct SC_COOLTIME_PACKET : BASE_PACKET {
	int skill1, skill2, skill3, skill4, skill5;
};

struct SC_SKILL_PACKET : BASE_PACKET {
	int id;
	char skillType; // 1:LEFTCLICK, 2:RIGHTCLICK, 3:SHIFT, 4:Q, 5:R
	short skillNum;
	bool onOff;
};

struct SC_ADD_NPC_PACKET : BASE_PACKET {
	int npcType;
	int id;
	float x, y, z;
	float lookX, lookY, lookZ;
	float rightX, rightY, rightZ;
};

struct SC_SKILL_SELECT_PACKET : BASE_PACKET {
	int id;
	int storage;
	int skill;
};

struct SC_GAME_TIME_PACKET : BASE_PACKET {
	char timeType; //0:ready, 1:game
	short time;
};

struct SC_READY_PACKET : BASE_PACKET {
	int id;
	bool ready;
};

struct SC_JOB_SELECT_PACKET : BASE_PACKET {
	int id;
	short job;
};

struct SC_GAME_START_PACKET : BASE_PACKET {
};

struct SC_MODEL_CUSTOMIZE_PACKET : BASE_PACKET {
	int id;
	ModelCustomize model;
};

struct SC_TELEPORT_PACKET : BASE_PACKET {
	int id;
	bool finish; //false: start, true: finish
};

struct SC_TOWER_ATTACK_PACKET : BASE_PACKET {
	int id;
	float x, y, z;
	float lookX, lookY, lookZ;
};

struct SC_TOWER_ATTACK_REMOVE_PACKET : BASE_PACKET {
	int id;
};

struct SC_TOWER_ATTACK_ADD_PACKET : BASE_PACKET {
	int id;
	float x, y, z;
};

struct SC_NPC_STAT_CHANGE_PACKET : BASE_PACKET {
	int id;
	int maxHp;
	int curHp;
};

struct SC_REMOVE_NPC_PACKET : BASE_PACKET {
	int id;
	int npcType;
};

struct SC_ADD_SKILL_OBJECT_PACKET : BASE_PACKET {
	short id;
	short objectType;
	float x, y, z;
	float lookX, lookY, lookZ;
};

struct SC_UPDATE_SKILL_OBJECT_PACKET : BASE_PACKET {
	short id;
	short objectType;
	float x, y, z;
};

struct SC_REMOVE_SKILL_OBJECT_PACKET : BASE_PACKET {
	short id;
	short objectType;
};

struct SC_NPC_ATTACK_PACKET : BASE_PACKET {
	short id;
};

struct SC_DAMAGE_PLAYER_PACKET : BASE_PACKET {
	short id;
	int curHp;
};

struct SC_PLAYER_STAT_CHANGE_PACKET : BASE_PACKET {
	short id;
	float speed;
};

struct SC_GIVE_GOLD_PACKET : BASE_PACKET {
	short id;
	short gold;
};

struct SC_STRUCTURE_STAT_PACKET : BASE_PACKET {
	short id;
	short hp;
};

struct SC_PLAYER_STATUS_CHANGE_PACKET : BASE_PACKET {
	int id;
	char statusType;	//0:SKILL_BUFF
	short statusNum;
};

struct SC_LOGIN_FAIL_PACKET : BASE_PACKET {
	char reason;	//0: No ID, 1: Wrong Password, 2: ID Exist, 3:Server Err
};

struct SC_SHOP_PACKET : BASE_PACKET {
	int customizePart;
	int customizeDetail;
};

struct SC_AUCTION_INFO_PACKET : BASE_PACKET {
	AuctionInfo auctionInfos[AUCTION_DATA_NUM];
};

struct SC_SKILL_FINISH_PACKET : BASE_PACKET {
	short id;
};

struct SC_HEALTH_MANA_PACKET : BASE_PACKET {
	short id;
	int maxHp;
	int curHp;
	int maxMp;
	int curMp;
};

struct SC_PLAYER_RESPAWN_PACKET : BASE_PACKET {
	short id;
	bool respawn;
};

struct SC_STRUCTURE_STAT_CHANGE_PACKET : BASE_PACKET {
	short id;	//0 ~ 3:Tower, 4:Nexus
	int maxHp;
	int curHp;
};

struct SC_STRUCTURE_STATUS_CHANGE_PACKET : BASE_PACKET {
	short id;	//0 ~ 3:Tower
	bool broken;
};

struct SC_CHANGE_CHANNEL_PACKET : BASE_PACKET {
	bool fail;
	int channel;
};

struct SC_PEER_INFO_PACKET : BASE_PACKET {
	char peerIP[INET_ADDRSTRLEN];
	unsigned short peerPortNum;
};

struct SC_CHANGE_NODE_PACKET : BASE_PACKET {
	bool fail;
	char reason;
};

struct SC_STAKE_TOKEN_PACKET : BASE_PACKET {
	bool fail;
	char reason;	//0:not enough days, 1:not enough tokens, 2:server Error
};

struct SC_TOKEN_NUM_PACKET : BASE_PACKET {
	int tokenNum;
};

struct SC_TRANSACTION_PACKET : BASE_PACKET {
	TransactionData transactions[7];
};

struct SC_MAGIC_EYE_POS_PACKET : BASE_PACKET {
	bool show;
};

struct SC_BLOCK_HEADER_PACKET : BASE_PACKET {
	char hash[65];
	int version;
	char timeStamp[21];
	char prevHash[65];
	char merkleRoot[65];
	int validatorID;
};

struct SC_BLOCK_BODY_PACKET : BASE_PACKET {
	char transaction[65];
};

struct SC_FULLNODE_PACKET : BASE_PACKET {
	bool fullNode;
};

struct SC_DUMMY_LOGIN_INFO_PACKET : BASE_PACKET {
	int id;
	float x, y, z;
};

struct SC_TELEPORT_ACTIVE_PACKET : BASE_PACKET {
	bool active;
};

struct SC_GAME_OVER_PACKET : BASE_PACKET {
	bool nexusDestroy;
};

struct SC_MONSTER_KILL_BUFF_PACKET : BASE_PACKET {
	char monsterType; //0:Unique(Red), 1:Green, 2:Golem
	int id;
};

struct SC_JUMP_FINISH_PACKET : BASE_PACKET {

};

struct SC_CUSTOMIZE_PARTS_PACKET : BASE_PACKET {
	unsigned char partType[CUSTOMIZE_PART_NUM_FROM_SERVER];
	unsigned char customizeNum[CUSTOMIZE_PART_NUM_FROM_SERVER];
	unsigned char count[CUSTOMIZE_PART_NUM_FROM_SERVER];
};

struct SC_AUCTION_PARTS_NUM_PACKET : BASE_PACKET {
	int num;
};

struct SC_STAKE_TOKEN_INFO_PACKET : BASE_PACKET {
	int stakedToken;
	int remainingDay;
};

struct SC_RTT_PACKET : BASE_PACKET {
	long long time;
};

struct LG_MATCH_START_PACKET : BASE_PACKET {
	short match_num;
};

struct LG_MATCH_PACKET : BASE_PACKET {
	char name[NAME_SIZE];
	short match_num;
	int id;
	ModelCustomize model;
};

struct GL_TRANSACTIONS_PACKET : BASE_PACKET {
	char name[NAME_SIZE];
	short token;
	char time[TIME_SIZE];
};

struct CS_TEST_CHANGE_SERVER_PACKET : BASE_PACKET {
	bool boss;
};

struct CS_TEST_INGAME_PACKET : BASE_PACKET {
	char name[NAME_SIZE];
};

struct CS_TEST_INGAME_PACKET2 : BASE_PACKET {

};

struct SC_TEST_CHANGE_SERVER_PACKET : BASE_PACKET {
	int id;
	char gameip[INET_ADDRSTRLEN];
	short gameport;
};

#pragma pack(pop)

constexpr char CS_TEST_CHANGE_SERVER = 40;
constexpr char CS_TEST_INGAME = 41;
constexpr char CS_TEST_INGAME2 = 42;
constexpr char SC_TEST_CHANGE_SERVER = 63;