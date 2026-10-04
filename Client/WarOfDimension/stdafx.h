// stdafx.h : 자주 사용하지만 자주 변경되지는 않는
// 표준 시스템 포함 파일 및 프로젝트 관련 포함 파일이
// 들어 있는 포함 파일입니다.
//

#pragma once

#define Test

#define WIN32_LEAN_AND_MEAN             // 거의 사용되지 않는 내용은 Windows 헤더에서 제외합니다.
#define _CRT_SECURE_NO_WARNINGS
// Windows 헤더 파일:
#include <windows.h>

// C의 런타임 헤더 파일입니다.
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <math.h>

#include <string>
#include <wrl.h>
#include <shellapi.h>

#include <WS2tcpip.h> 
#include <MSWSock.h>
#include <iostream>
#include <thread>
#include <fstream>
#include <vector>
#include <list>
#include <array>
#include <map>
#include <unordered_map>
#include <set>
#include <functional>
#include <thread>
#include <chrono>
#include <random>
#include <mutex>
#include <shared_mutex>
#include <iomanip>
#include "../../Server/Game_Server/protocol.h"

#include <d3d12.h>
#include <dxgi1_4.h>
#include <dxgi1_6.h>
#include <d2d1_3.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <DirectXColors.h>
#include <DirectXCollision.h>

#include <dwrite.h>
#include <d3d11on12.h>

#include <Mmsystem.h>
#include <stdint.h>

#include <io.h>
#include "fmod.h"
#include "fmod.hpp"
#include "fmod_dsp.h"
#include "fmod_errors.h"

#include <codecvt>
#include <locale>

#ifdef _DEBUG
#pragma comment(lib,"fmodL_vc.lib")
#else
#pragma comment(lib,"fmod_vc.lib")
#endif // _DEBUG 

#ifdef _DEBUG
#include <dxgidebug.h>
#endif

#pragma comment(lib, "WS2_32.lib")
#pragma comment(lib, "MSWSock.lib")

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

#pragma comment(lib, "dxguid.lib")

#define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))

#define MAX_SOUNDCHANNEL 100
#define LOBBY_NPC 4
#define SECTION_NUM 10

using namespace std;
using namespace DirectX;
using namespace DirectX::PackedVector;

using Microsoft::WRL::ComPtr;

extern HINSTANCE						ghAppInstance;

//#define _WITH_SWAPCHAIN_FULLSCREEN_STATE

constexpr float SOUND_DISTANCE = 40.f;
constexpr float OBJECT_EFFECT_SOUND_DISTANCE = 30.0f;
constexpr float INIT_VOLUME_FLOAT = 0.f;
constexpr int INIT_VOLUME_INT = 50;
#define VOLUME_INT_TO_FLOAT(vol) INIT_VOLUME_FLOAT + static_cast<float>((vol - 50) * 2.0f) / 100.0f

constexpr int MAX_LIGHTS = 4;
constexpr int MAX_DEPTH_TEXTURES = 1;

constexpr int POINT_LIGHT = 1;
constexpr int SPOT_LIGHT = 2;
constexpr int DIRECTIONAL_LIGHT = 3;

constexpr int FRAME_BUFFER_WIDTH = 1920;
constexpr int FRAME_BUFFER_HEIGHT = 1080;
constexpr int FRAME_BUFFER_RESIZE = 1458;
constexpr int DEFERREDNUM = 6;

constexpr UINT NOT_HDR = 0;
constexpr UINT DURAND = 1;
constexpr UINT UNCHARTED = 2;
constexpr UINT HABLE_MACCANN = 3;
constexpr UINT ACES = 4;

constexpr int _DEPTH_BUFFER_WIDTH = FRAME_BUFFER_WIDTH * 4;
constexpr int _DEPTH_BUFFER_HEIGHT = FRAME_BUFFER_HEIGHT * 4;

constexpr int _PLANE_WIDTH = 1024;
constexpr int _PLANE_HEIGHT = 1024;

constexpr int INGAME_PLAYER = 4;
constexpr int WEAPON_KIND = 4;

constexpr XMFLOAT3 READY_SCENE_PLAYER1(-38.0f, 4.0f, 27.0f);
constexpr XMFLOAT3 READY_SCENE_PLAYER2(-40.0f, 4.0f, 28.0f);
constexpr XMFLOAT3 READY_SCENE_PLAYER3(-42.0f, 4.0f, 29.0f);
constexpr XMFLOAT3 READY_SCENE_BOSS(-25.5f, 3.0f, 33.0f);

constexpr array<XMFLOAT3, 4> READY_SCENE_PLAYER_POS = { READY_SCENE_PLAYER1, READY_SCENE_PLAYER2, READY_SCENE_PLAYER3, READY_SCENE_BOSS };
constexpr array<XMFLOAT3, 4> READY_SCENE_PLAYER_POS_BY_BOSS = { XMFLOAT3(-31.0f, 4.0f, 29.0f), XMFLOAT3(-33.0f, 4.0f, 29.5f), XMFLOAT3(-35.0f, 4.0f, 30.0f), READY_SCENE_BOSS };

constexpr std::array<XMFLOAT2, 2> TELEPORT_POS = { XMFLOAT2(-97.5f, -30.1f), XMFLOAT2(-24.5f, -103.5f) };
constexpr std::array<XMFLOAT3, PATH_NUM> TOWER_POS = { XMFLOAT3(-68.47f, 9.17f, -134.98f), XMFLOAT3(-78.08f, 9.17f, -103.54f), XMFLOAT3(-102.89f, 9.17f, -78.76f), XMFLOAT3(-129.93f, 9.17f, -70.61f) };

constexpr int WORLD_WIDTH = 300;
constexpr int WORLD_HEIGHT = 300;

constexpr float FLOOR_HEIGHT = 5.f;
constexpr float WORLD_GRAVITY = -250.f;
//#define WITH_DATABASE

//enum types for server
enum class OP_TYPE { OP_ACCEPT, OP_RECV, OP_SEND, OP_DISCONNECT, OP_CONNECT };

enum class SCENEKIND
{
	NONE,
	TITLE,
	LOBBY,
	READY,
	INGAME
};

enum class SKILLKIND
{
	NONE,
	LEFTCLICK,
	RIGHTCLICK,
	SHIFT,
	Q,
	R
};

//enum types for Client
enum class CHAT
{
	ALL,
	CHANNEL,
};

enum class ORDER
{
	PLAYER1,
	PLAYER2,
	PLAYER3,
	BOSS
};

enum class JOB
{
	ARCHER,
	FIGHTER,
	SWORDMAN,
	WIZARD,
	NONE
};

enum class BOSSJOB
{
	OGRE,
	PROGRAMMER,
	NONE
};

enum class TEXTURETYPE
{
	NONE = -1,
	BUTTON,
	ALPHA,
	PROGRESSBAR,
	PROGRESSBARR,
	COOLTIMEICON,
	PLAYERSKILL,
	BOSSSKILL,
	JOBKIND,
	ITEMKIND,
	MINIMAPICON,
	CHANNELBUTTON,
	CUSTOMPARTS,
	TWINKLE,
	CALCKEYBOARD,
	SCREENEFFECTBLOOD,
	SCREENEFFECTSPEED,
	SCREENEFFECTWIN,
	SCREENEFFECTDEFEAT,
	SKILLINFO
};

enum class OBJ_TYPE
{
	PLAYER,
	OBJECT,
	TEXTURE,
};

enum class CHANNELID { BGM, PLAYER, SKILL, NPC, EFFECT, END };

enum class ANI_ON_SERVER { NONE, IDLE, DEAD };

enum ANIM_TYPE {ARCHER_BACKSTEP, ARCHER_DODGE, ARCHER_VAULT  };

enum BRUSH_COLOR {WHITE, LIME_GREEN, SKY_BLUE, DARK_CRIMSON, DARK_GRAY, RED, BRUSH_COUNT};

enum TEXT_SIZE { SIZE_15, SIZE_18, SIZE_25, SIZE_30, SIZE_50, SIZE_60, TEXT_COUNT};

enum LOADING_TEXT {MAP, CHARACTER, SHADER, LOADING_TEXT_COUNT};

enum class MINION_ANIM { IDLE, WALK, RUN, HIT, DIE, ATTACK1, ATTACK2, VICTORY, COUNT };

enum class UIOBJECTSTATE { DEFAULT, HOVER, CLICK };

enum PARTICLE_TYPE { FOG, DROPARROW, XYEMITTER, XZEMITTER, TAILSTAR, TORNADO, INSIDEMOVE, SPINSCENTER, FIRE, BUFF, PHOENIX,STORMARROW, ATTACK, BALL,BASH,AURABLADE,BILLBOARD,MAGICBALL, RAY, SPHERE, BIGBANG, BILLBOARDFRONT,ROUNDRANGE,JUMPEFEECT,COINBOMB,CRUSH, FENCE , NONE};

enum PARTICLE_SITUATION {ID1_SKILL, ID2_SKILL, ID3_SKILL, ID4_SKILL, JUMP, COINBYDEATH, FENCEEFFECT};

enum PARTICLE_ADDRESS { ROUND, ARROW, NOISE, DUST, FIGHTATTACK,COUNTER, SHIELD ,HELLBLADE, SWORD, REFLECT,BLACKSPHERE, DIMENSION,SPRITEJUMP, COIN,ELECTRONIC, ADDRESS_COUNT };



struct BBInfo
{
	float center[3];
	float extent[3];
	float position[3];
	float quaternion[4];
	float scale[3];
};

struct SERVERCHAT
{
	CHAT op;
	WCHAR* chating;
	char name[NAME_SIZE];
};

struct CollisionBox {
	float left;
	float right;
	float top;
	float bottom;

	CollisionBox() : left(0.0f), right(0.0f), top(0.0f), bottom(0.0f) {
	}

	CollisionBox(float x, float y, float width, float height)
		: left(x), right(x + width), top(y), bottom(y + height) {
	}
};

struct Additional_Stats
{
	int point = 15;
	int hp = 0;
	int mp = 0;
	int attack = 0;
	int magic_attack = 0;
	int defense = 0;
	int magic_defense = 0;
	int speed = 0;
	int tenacity = 0;
	int critical = 0;
};

struct Stat_Level
{
	int hp = 0;
	int mp = 0;
	int attack = 0;
	int magic_attack = 0;
	int defense = 0;
	int magic_defense = 0;
	int speed = 0;
	int tenacity = 0;
	int critical = 0;

	int hp_price = 10;
	int mp_price = 10;
	int attack_price = 10;
	int magic_attack_price = 10;
	int defense_price = 10;
	int magic_defense_price = 10;
	int speed_price = 10;
	int tenacity_price = 10;
	int critical_price = 10;
};

struct LobbyAuctionInfo {
	int curParts;
	int curIndex;
	int curPage;
	int maxPageNum;
	int sellPrice;
	array<int, 7> productPrice;
	array<int, 7> productParts;
	array<int, 7> productNum;
	array<int, 7> closingTime;
	array<char[NAME_SIZE], 7> username;

	void initialize() {
		curParts = 0;
		curIndex = 0;
		curPage = 0;
		maxPageNum = 0;
		sellPrice = 0;
		productPrice = { 0, 0, 0, 0, 0, 0, 0 };
		productParts = { 0, 0, 0, 0, 0, 0, 0 };
		productNum = { 0, 0, 0, 0, 0, 0, 0 };
		closingTime = { 0, 0, 0, 0, 0, 0, 0 };
		username = {"", "", "", "", "", "", ""};
	}
};

struct LobbyBlockChainInfo {
	int stakingPeriod;
	int inputTokens;
	int myTokens;
	int stakingTokens;
	int unstakingDeadline;

	void initialize() {
		stakingPeriod = 0;
		inputTokens = 0;
		myTokens = 0;
		stakingTokens = 0;
		unstakingDeadline = 0;
	}
};

//Singleton design pattern macro
#define SINGLETON(ClassName)									\
public:															\
	ClassName() {};												\
	~ClassName() {};											\
	ClassName(ClassName const&) = delete;						\
	ClassName& operator=(const ClassName&) = delete;			\
	static ClassName* GetInstance()	{							\
		if(!m_instance)											\
			m_instance.reset(new ClassName);					\
		return m_instance.get();								\
	}															\
																\
	void DestroyInstance()										\
	{															\
		if (m_instance)											\
			m_instance.reset(nullptr);							\
	}															\
private:														\
	static std::unique_ptr<ClassName> m_instance;

// TODO: 프로그램에 필요한 추가 헤더는 여기에서 참조합니다.

extern UINT	gnCbvSrvDescriptorIncrementSize;
extern UINT	gnRtvDescriptorIncrementSize;
extern UINT gnDsvDescriptorIncrementSize;

extern void SynchronizeResourceTransition(ID3D12GraphicsCommandList* pd3dCommandList, ID3D12Resource* pd3dResource, D3D12_RESOURCE_STATES d3dStateBefore, D3D12_RESOURCE_STATES d3dStateAfter);
extern void WaitForGpuComplete(ID3D12CommandQueue* pd3dCommandQueue, ID3D12Fence* pd3dFence, UINT64 nFenceValue, HANDLE hFenceEvent);
extern void ExecuteCommandList(ID3D12GraphicsCommandList* pd3dCommandList, ID3D12CommandQueue* pd3dCommandQueue, ID3D12Fence* pd3dFence, UINT64 nFenceValue, HANDLE hFenceEvent);

extern void SwapResourcePointer(ID3D12Resource** ppd3dResourceA, ID3D12Resource** ppd3dResourceB);

extern ID3D12Resource* CreateBufferResource(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, void* pData, UINT nBytes, D3D12_HEAP_TYPE d3dHeapType = D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATES d3dResourceStates = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, ID3D12Resource** ppd3dUploadBuffer = NULL);
extern ID3D12Resource* CreateTextureResourceFromDDSFile(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, const wchar_t* pszFileName, ID3D12Resource** ppd3dUploadBuffer, D3D12_RESOURCE_STATES d3dResourceStates = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
extern ID3D12Resource* CreateTexture2DResource(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, UINT nWidth, UINT nHeight, UINT nElements, UINT nMipLevels, DXGI_FORMAT dxgiFormat, D3D12_RESOURCE_FLAGS d3dResourceFlags, D3D12_RESOURCE_STATES d3dResourceStates, D3D12_CLEAR_VALUE* pd3dClearValue);
extern ID3D12Resource* CreateTextureResourceFromWICFile(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, const wchar_t* pszFileName, ID3D12Resource** ppd3dUploadBuffer, D3D12_RESOURCE_STATES d3dResourceStates = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

extern BYTE ReadStringFromFile(FILE *pInFile, char *pstrToken);
extern int ReadIntegerFromFile(FILE *pInFile);
extern float ReadFloatFromFile(FILE *pInFile);

bool LoadMap(std::string filename, int* nObject, std::vector<BBInfo> &vecBox);

XMFLOAT2 CalculateScreenResolutionSize(float widthPercentage, float heightPercentage); // Function to calculate adjusted size based on percentage of screen size
XMFLOAT2 CalculateScreenResolutionPos(float widthPercentage, float heightPercentage); // Function to calculate adjusted size based on percentage of screen size
D3D12_SHADER_BYTECODE CompileShaderFromFile(const WCHAR* pszFileName, LPCSTR pszShaderName, LPCSTR pszShaderProfile, ID3DBlob** ppd3dShaderBlob);

#define RANDOM_COLOR			XMFLOAT4(rand() / float(RAND_MAX), rand() / float(RAND_MAX), rand() / float(RAND_MAX), rand() / float(RAND_MAX))

#define EPSILON					1.0e-10f

inline bool IsZero(float fValue) { return((fabsf(fValue) < EPSILON)); }
inline bool IsEqual(float fA, float fB) { return(::IsZero(fA - fB)); }
inline bool IsZero(float fValue, float fEpsilon) { return((fabsf(fValue) < fEpsilon)); }
inline bool IsEqual(float fA, float fB, float fEpsilon) { return(::IsZero(fA - fB, fEpsilon)); }
inline float InverseSqrt(float fValue) { return 1.0f / sqrtf(fValue); }
inline void Swap(float *pfS, float *pfT) { float fTemp = *pfS; *pfS = *pfT; *pfT = fTemp; }

#define ANIMATION_TYPE_ONCE				0
#define ANIMATION_TYPE_LOOP				1
#define ANIMATION_TYPE_PINGPONG			2

#define ANIMATION_CALLBACK_EPSILON		0.00165f


//Thread Variable
//CRITICAL_SECTION g_cs;

template <typename T>
void SafeDelete(T& ptr)
{
	if (ptr != nullptr)
	{
		delete ptr;
		ptr = nullptr;
	}
}

namespace Vector3
{
	inline XMFLOAT3 XMVectorToFloat3(XMVECTOR& xmvVector)
	{
		XMFLOAT3 xmf3Result;
		XMStoreFloat3(&xmf3Result, xmvVector);
		return(xmf3Result);
	}

	inline XMFLOAT3 ScalarProduct(XMFLOAT3& xmf3Vector, float fScalar, bool bNormalize = true)
	{
		XMFLOAT3 xmf3Result;
		if (bNormalize)
			XMStoreFloat3(&xmf3Result, XMVector3Normalize(XMLoadFloat3(&xmf3Vector)) * fScalar);
		else
			XMStoreFloat3(&xmf3Result, XMLoadFloat3(&xmf3Vector) * fScalar);
		return(xmf3Result);
	}
	inline XMFLOAT3 ScalarProduct(XMFLOAT3&& xmf3Vector, float fScalar, bool bNormalize = true)
	{
		XMFLOAT3 xmf3Result;
		if (bNormalize)
			XMStoreFloat3(&xmf3Result, XMVector3Normalize(XMLoadFloat3(&xmf3Vector)) * fScalar);
		else
			XMStoreFloat3(&xmf3Result, XMLoadFloat3(&xmf3Vector) * fScalar);
		return(xmf3Result);
	}

	inline XMFLOAT3 Add(const XMFLOAT3& xmf3Vector1, const XMFLOAT3& xmf3Vector2)
	{
		XMFLOAT3 xmf3Result;
		XMStoreFloat3(&xmf3Result, XMLoadFloat3(&xmf3Vector1) + XMLoadFloat3(&xmf3Vector2));
		return(xmf3Result);
	}

	inline XMFLOAT3 Add(XMFLOAT3& xmf3Vector1, XMFLOAT3& xmf3Vector2, float fScalar)
	{
		XMFLOAT3 xmf3Result;
		XMStoreFloat3(&xmf3Result, XMLoadFloat3(&xmf3Vector1) + (XMLoadFloat3(&xmf3Vector2) * fScalar));
		return(xmf3Result);
	}

	inline XMFLOAT3 Subtract(XMFLOAT3& xmf3Vector1, XMFLOAT3& xmf3Vector2)
	{
		XMFLOAT3 xmf3Result;
		XMStoreFloat3(&xmf3Result, XMLoadFloat3(&xmf3Vector1) - XMLoadFloat3(&xmf3Vector2));
		return(xmf3Result);
	}
	inline XMFLOAT3 Subtract(XMFLOAT3& xmf3Vector1, XMFLOAT3&& xmf3Vector2)
	{
		XMFLOAT3 xmf3Result;
		XMStoreFloat3(&xmf3Result, XMLoadFloat3(&xmf3Vector1) - XMLoadFloat3(&xmf3Vector2));
		return(xmf3Result);
	}

	inline float DotProduct(XMFLOAT3& xmf3Vector1, XMFLOAT3& xmf3Vector2)
	{
		XMFLOAT3 xmf3Result;
		XMStoreFloat3(&xmf3Result, XMVector3Dot(XMLoadFloat3(&xmf3Vector1), XMLoadFloat3(&xmf3Vector2)));
		return(xmf3Result.x);
	}
	inline float DotProduct(XMFLOAT3&& xmf3Vector1, XMFLOAT3& xmf3Vector2)
	{
		XMFLOAT3 xmf3Result;
		XMStoreFloat3(&xmf3Result, XMVector3Dot(XMLoadFloat3(&xmf3Vector1), XMLoadFloat3(&xmf3Vector2)));
		return(xmf3Result.x);
	}

	inline XMFLOAT3 CrossProduct(XMFLOAT3& xmf3Vector1, XMFLOAT3& xmf3Vector2, bool bNormalize = true)
	{
		XMFLOAT3 xmf3Result;
		if (bNormalize)
			XMStoreFloat3(&xmf3Result, XMVector3Normalize(XMVector3Cross(XMLoadFloat3(&xmf3Vector1), XMLoadFloat3(&xmf3Vector2))));
		else
			XMStoreFloat3(&xmf3Result, XMVector3Cross(XMLoadFloat3(&xmf3Vector1), XMLoadFloat3(&xmf3Vector2)));
		return(xmf3Result);
	}
	inline XMFLOAT3 CrossProduct(XMFLOAT3& xmf3Vector1, XMFLOAT3&& xmf3Vector2, bool bNormalize = true)
	{
		XMFLOAT3 xmf3Result;
		if (bNormalize)
			XMStoreFloat3(&xmf3Result, XMVector3Normalize(XMVector3Cross(XMLoadFloat3(&xmf3Vector1), XMLoadFloat3(&xmf3Vector2))));
		else
			XMStoreFloat3(&xmf3Result, XMVector3Cross(XMLoadFloat3(&xmf3Vector1), XMLoadFloat3(&xmf3Vector2)));
		return(xmf3Result);
	}
	inline XMFLOAT3 CrossProduct(const XMFLOAT3& xmf3Vector1, const XMFLOAT3& xmf3Vector2, bool bNormalize = true)
	{
		XMFLOAT3 xmf3Result;
		if (bNormalize)
			XMStoreFloat3(&xmf3Result, XMVector3Normalize(XMVector3Cross(XMLoadFloat3(&xmf3Vector1), XMLoadFloat3(&xmf3Vector2))));
		else
			XMStoreFloat3(&xmf3Result, XMVector3Cross(XMLoadFloat3(&xmf3Vector1), XMLoadFloat3(&xmf3Vector2)));
		return(xmf3Result);
	}

	inline XMFLOAT3 Normalize(XMFLOAT3& xmf3Vector)
	{
		XMFLOAT3 m_xmf3Normal;
		XMStoreFloat3(&m_xmf3Normal, XMVector3Normalize(XMLoadFloat3(&xmf3Vector)));
		return(m_xmf3Normal);
	}
	inline XMFLOAT3 Normalize(XMFLOAT3&& xmf3Vector)
	{
		XMFLOAT3 m_xmf3Normal;
		XMStoreFloat3(&m_xmf3Normal, XMVector3Normalize(XMLoadFloat3(&xmf3Vector)));
		return(m_xmf3Normal);
	}

	inline float Length(XMFLOAT3& xmf3Vector)
	{
		XMFLOAT3 xmf3Result;
		XMStoreFloat3(&xmf3Result, XMVector3Length(XMLoadFloat3(&xmf3Vector)));
		return(xmf3Result.x);
	}

	inline bool IsZero(XMFLOAT3& xmf3Vector)
	{
		XMFLOAT3 xmf3Result;
		XMStoreFloat3(&xmf3Result, XMVector3Length(XMLoadFloat3(&xmf3Vector)));
		return(::IsZero(xmf3Result.x));
	}

	inline float Angle(XMVECTOR& xmvVector1, XMVECTOR& xmvVector2)
	{
		XMVECTOR xmvAngle = XMVector3AngleBetweenNormals(xmvVector1, xmvVector2);
		return(XMConvertToDegrees(XMVectorGetX(xmvAngle)));
	}

	inline float Angle(XMVECTOR&& xmvVector1, XMVECTOR&& xmvVector2)
	{
		XMVECTOR xmvAngle = XMVector3AngleBetweenNormals(xmvVector1, xmvVector2);
		return(XMConvertToDegrees(XMVectorGetX(xmvAngle)));
	}

	inline float Angle(XMFLOAT3& xmf3Vector1, XMFLOAT3& xmf3Vector2)
	{
		return(Angle(XMLoadFloat3(&xmf3Vector1), XMLoadFloat3(&xmf3Vector2)));
	}
	inline float Angle(XMFLOAT3&& xmf3Vector1, XMFLOAT3& xmf3Vector2)
	{
		return(Angle(XMLoadFloat3(&xmf3Vector1), XMLoadFloat3(&xmf3Vector2)));
	}

	inline XMFLOAT3 TransformNormal(XMFLOAT3& xmf3Vector, XMMATRIX& xmmtxTransform)
	{
		XMFLOAT3 xmf3Result;
		XMStoreFloat3(&xmf3Result, XMVector3TransformNormal(XMLoadFloat3(&xmf3Vector), xmmtxTransform));
		return(xmf3Result);
	}

	inline XMFLOAT3 TransformCoord(XMFLOAT3& xmf3Vector, XMMATRIX& xmmtxTransform)
	{
		XMFLOAT3 xmf3Result;
		XMStoreFloat3(&xmf3Result, XMVector3TransformCoord(XMLoadFloat3(&xmf3Vector), xmmtxTransform));
		return(xmf3Result);
	}
	inline XMFLOAT3 TransformCoord(XMFLOAT3& xmf3Vector, XMMATRIX&& xmmtxTransform)
	{
		XMFLOAT3 xmf3Result;
		XMStoreFloat3(&xmf3Result, XMVector3TransformCoord(XMLoadFloat3(&xmf3Vector), xmmtxTransform));
		return(xmf3Result);
	}

	inline XMFLOAT3 TransformCoord(XMFLOAT3& xmf3Vector, XMFLOAT4X4& xmmtx4x4Matrix)
	{
		return(TransformCoord(xmf3Vector, XMLoadFloat4x4(&xmmtx4x4Matrix)));
	}
}

namespace Vector4
{
	inline XMFLOAT4 Add(XMFLOAT4& xmf4Vector1, XMFLOAT4& xmf4Vector2)
	{
		XMFLOAT4 xmf4Result;
		XMStoreFloat4(&xmf4Result, XMLoadFloat4(&xmf4Vector1) + XMLoadFloat4(&xmf4Vector2));
		return(xmf4Result);
	}
	inline XMFLOAT4 Add(XMFLOAT4&& xmf4Vector1, XMFLOAT4& xmf4Vector2)
	{
		XMFLOAT4 xmf4Result;
		XMStoreFloat4(&xmf4Result, XMLoadFloat4(&xmf4Vector1) + XMLoadFloat4(&xmf4Vector2));
		return(xmf4Result);
	}

	inline XMFLOAT4 Multiply(XMFLOAT4& xmf4Vector1, XMFLOAT4& xmf4Vector2)
	{
		XMFLOAT4 xmf4Result;
		XMStoreFloat4(&xmf4Result, XMLoadFloat4(&xmf4Vector1) * XMLoadFloat4(&xmf4Vector2));
		return(xmf4Result);
	}

	inline XMFLOAT4 Multiply(float fScalar, XMFLOAT4& xmf4Vector)
	{
		XMFLOAT4 xmf4Result;
		XMStoreFloat4(&xmf4Result, fScalar * XMLoadFloat4(&xmf4Vector));
		return(xmf4Result);
	}
}

namespace Matrix4x4
{
	inline XMFLOAT4X4 Identity()
	{
		XMFLOAT4X4 xmf4x4Result;
		XMStoreFloat4x4(&xmf4x4Result, XMMatrixIdentity());
		return(xmf4x4Result);
	}

	inline XMFLOAT4X4 Zero()
	{
		XMFLOAT4X4 xmf4x4Result;
		XMStoreFloat4x4(&xmf4x4Result, XMMatrixSet(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f));
		return(xmf4x4Result);
	}

	inline XMFLOAT4X4 Multiply(XMFLOAT4X4& xmmtx4x4Matrix1, XMFLOAT4X4& xmmtx4x4Matrix2)
	{
		XMFLOAT4X4 xmf4x4Result;
		XMStoreFloat4x4(&xmf4x4Result, XMMatrixMultiply(XMLoadFloat4x4(&xmmtx4x4Matrix1), XMLoadFloat4x4(&xmmtx4x4Matrix2)));
		return(xmf4x4Result);
	}
	inline XMFLOAT4X4 Multiply(XMFLOAT4X4&& xmmtx4x4Matrix1, XMFLOAT4X4& xmmtx4x4Matrix2)
	{
		XMFLOAT4X4 xmf4x4Result;
		XMStoreFloat4x4(&xmf4x4Result, XMMatrixMultiply(XMLoadFloat4x4(&xmmtx4x4Matrix1), XMLoadFloat4x4(&xmmtx4x4Matrix2)));
		return(xmf4x4Result);
	}
	inline XMFLOAT4X4 Multiply(XMFLOAT4X4&& xmmtx4x4Matrix1, XMFLOAT4X4&& xmmtx4x4Matrix2)
	{
		XMFLOAT4X4 xmf4x4Result;
		XMStoreFloat4x4(&xmf4x4Result, XMMatrixMultiply(XMLoadFloat4x4(&xmmtx4x4Matrix1), XMLoadFloat4x4(&xmmtx4x4Matrix2)));
		return(xmf4x4Result);
	}

	inline XMFLOAT4X4 Scale(XMFLOAT4X4& xmf4x4Matrix, float fScale)
	{
		XMFLOAT4X4 xmf4x4Result;
		XMStoreFloat4x4(&xmf4x4Result, XMLoadFloat4x4(&xmf4x4Matrix) * fScale);
/*
		XMVECTOR S, R, T;
		XMMatrixDecompose(&S, &R, &T, XMLoadFloat4x4(&xmf4x4Matrix));
		S = XMVectorScale(S, fScale);
		T = XMVectorScale(T, fScale);
		R = XMVectorScale(R, fScale);
		//R = XMQuaternionMultiply(R, XMVectorSet(0, 0, 0, fScale));
		XMStoreFloat4x4(&xmf4x4Result, XMMatrixAffineTransformation(S, XMVectorZero(), R, T));
*/
		return(xmf4x4Result);
	}

	inline XMFLOAT4X4 Add(XMFLOAT4X4& xmmtx4x4Matrix1, XMFLOAT4X4& xmmtx4x4Matrix2)
	{
		XMFLOAT4X4 xmf4x4Result;
		XMStoreFloat4x4(&xmf4x4Result, XMLoadFloat4x4(&xmmtx4x4Matrix1) + XMLoadFloat4x4(&xmmtx4x4Matrix2));
		return(xmf4x4Result);
	}
	inline XMFLOAT4X4 Add(XMFLOAT4X4& xmmtx4x4Matrix1, XMFLOAT4X4&& xmmtx4x4Matrix2)
	{
		XMFLOAT4X4 xmf4x4Result;
		XMStoreFloat4x4(&xmf4x4Result, XMLoadFloat4x4(&xmmtx4x4Matrix1) + XMLoadFloat4x4(&xmmtx4x4Matrix2));
		return(xmf4x4Result);
	}

	inline XMFLOAT4X4 Multiply(XMFLOAT4X4& xmmtx4x4Matrix1, XMMATRIX& xmmtxMatrix2)
	{
		XMFLOAT4X4 xmf4x4Result;
		XMStoreFloat4x4(&xmf4x4Result, XMLoadFloat4x4(&xmmtx4x4Matrix1) * xmmtxMatrix2);
		return(xmf4x4Result);
	}

	inline XMFLOAT4X4 RotateAxis(XMFLOAT3& xmf3Axis, float fAngle)
	{
		XMFLOAT4X4 xmf4x4Result;
		XMStoreFloat4x4(&xmf4x4Result, XMMatrixRotationAxis(XMLoadFloat3(&xmf3Axis), XMConvertToRadians(fAngle)));
		return(xmf4x4Result);
	}

	inline XMFLOAT4X4 Rotate(float x, float y, float z)
	{
		XMFLOAT4X4 xmf4x4Result;
		XMStoreFloat4x4(&xmf4x4Result, XMMatrixRotationRollPitchYaw(XMConvertToRadians(x), XMConvertToRadians(y), XMConvertToRadians(z)));
		return(xmf4x4Result);
	}

	inline XMFLOAT4X4 AffineTransformation(XMFLOAT3& xmf3Scaling, XMFLOAT3& xmf3RotateOrigin, XMFLOAT3& xmf3Rotation, XMFLOAT3& xmf3Translation)
	{
		XMFLOAT4X4 xmf4x4Result;
		XMStoreFloat4x4(&xmf4x4Result, XMMatrixAffineTransformation(XMLoadFloat3(&xmf3Scaling), XMLoadFloat3(&xmf3RotateOrigin), XMQuaternionRotationRollPitchYaw(XMConvertToRadians(xmf3Rotation.x), XMConvertToRadians(xmf3Rotation.y), XMConvertToRadians(xmf3Rotation.z)), XMLoadFloat3(&xmf3Translation)));
		return(xmf4x4Result);
	}

	inline XMFLOAT4X4 Multiply(XMMATRIX& xmmtxMatrix1, XMFLOAT4X4& xmmtx4x4Matrix2)
	{
		XMFLOAT4X4 xmf4x4Result;
		XMStoreFloat4x4(&xmf4x4Result, xmmtxMatrix1 * XMLoadFloat4x4(&xmmtx4x4Matrix2));
		return(xmf4x4Result);
	}
	inline XMFLOAT4X4 Multiply(XMMATRIX&& xmmtxMatrix1, XMFLOAT4X4& xmmtx4x4Matrix2)
	{
		XMFLOAT4X4 xmf4x4Result;
		XMStoreFloat4x4(&xmf4x4Result, xmmtxMatrix1 * XMLoadFloat4x4(&xmmtx4x4Matrix2));
		return(xmf4x4Result);
	}

	inline XMFLOAT4X4 Interpolate(XMFLOAT4X4& xmf4x4Matrix1, XMFLOAT4X4& xmf4x4Matrix2, float t)
	{
		XMFLOAT4X4 xmf4x4Result;
		XMVECTOR S0, R0, T0, S1, R1, T1;
		XMMatrixDecompose(&S0, &R0, &T0, XMLoadFloat4x4(&xmf4x4Matrix1));
		XMMatrixDecompose(&S1, &R1, &T1, XMLoadFloat4x4(&xmf4x4Matrix2));
		XMVECTOR S = XMVectorLerp(S0, S1, t);
		XMVECTOR T = XMVectorLerp(T0, T1, t);
		XMVECTOR R = XMQuaternionSlerp(R0, R1, t);
		XMStoreFloat4x4(&xmf4x4Result, XMMatrixAffineTransformation(S, XMVectorZero(), R, T));
		return(xmf4x4Result);
	}

	inline XMFLOAT4X4 Inverse(XMFLOAT4X4& xmmtx4x4Matrix)
	{
		XMFLOAT4X4 xmf4x4Result;
		XMStoreFloat4x4(&xmf4x4Result, XMMatrixInverse(NULL, XMLoadFloat4x4(&xmmtx4x4Matrix)));
		return(xmf4x4Result);
	}

	inline XMFLOAT4X4 Transpose(XMFLOAT4X4& xmmtx4x4Matrix)
	{
		XMFLOAT4X4 xmf4x4Result;
		XMStoreFloat4x4(&xmf4x4Result, XMMatrixTranspose(XMLoadFloat4x4(&xmmtx4x4Matrix)));
		return(xmf4x4Result);
	}

	inline XMFLOAT4X4 PerspectiveFovLH(float FovAngleY, float AspectRatio, float NearZ, float FarZ)
	{
		XMFLOAT4X4 xmf4x4Result;
		XMStoreFloat4x4(&xmf4x4Result, XMMatrixPerspectiveFovLH(FovAngleY, AspectRatio, NearZ, FarZ));
		return(xmf4x4Result);
	}

	inline XMFLOAT4X4 LookAtLH(XMFLOAT3& xmf3EyePosition, XMFLOAT3& xmf3LookAtPosition, XMFLOAT3& xmf3UpDirection)
	{
		XMFLOAT4X4 xmf4x4Result;
		XMStoreFloat4x4(&xmf4x4Result, XMMatrixLookAtLH(XMLoadFloat3(&xmf3EyePosition), XMLoadFloat3(&xmf3LookAtPosition), XMLoadFloat3(&xmf3UpDirection)));
		return(xmf4x4Result);
	}
	inline XMFLOAT4X4 LookAtLH(XMFLOAT3& xmf3EyePosition, XMFLOAT3& xmf3LookAtPosition, XMFLOAT3&& xmf3UpDirection)
	{
		XMFLOAT4X4 xmf4x4Result;
		XMStoreFloat4x4(&xmf4x4Result, XMMatrixLookAtLH(XMLoadFloat3(&xmf3EyePosition), XMLoadFloat3(&xmf3LookAtPosition), XMLoadFloat3(&xmf3UpDirection)));
		return(xmf4x4Result);
	}
}

namespace Triangle
{
	inline bool Intersect(XMFLOAT3& xmf3RayPosition, XMFLOAT3& xmf3RayDirection, XMFLOAT3& v0, XMFLOAT3& v1, XMFLOAT3& v2, float& fHitDistance)
	{
		return(TriangleTests::Intersects(XMLoadFloat3(&xmf3RayPosition), XMLoadFloat3(&xmf3RayDirection), XMLoadFloat3(&v0), XMLoadFloat3(&v1), XMLoadFloat3(&v2), fHitDistance));
	}
}

namespace Plane
{
	inline XMFLOAT4 Normalize(XMFLOAT4& xmf4Plane)
	{
		XMFLOAT4 xmf4Result;
		XMStoreFloat4(&xmf4Result, XMPlaneNormalize(XMLoadFloat4(&xmf4Plane)));
		return(xmf4Result);
	}
}
