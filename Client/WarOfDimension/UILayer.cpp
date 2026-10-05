#include "stdafx.h"
#include "UILayer.h"
#include "NetworkManager.h"
#include "SceneManager.h"
#include "Shader.h"
#include "Player.h"
#include "Util.h"


using namespace std;

UILayer* UILayer::s_instance = nullptr;

uniform_int_distribution<> uid2;
random_device rd2;
default_random_engine dre2(rd2());

UILayer::UILayer(UINT nFrames, UINT nTextBlocks, ID3D12Device* pd3dDevice, ID3D12CommandQueue* pd3dCommandQueue, ID3D12Resource** ppd3dRenderTargets, UINT nWidth, UINT nHeight)
{
    m_fWidth = static_cast<float>(nWidth);
    m_fHeight = static_cast<float>(nHeight);
    m_nRenderTargets = nFrames;
    m_ppd3d11WrappedRenderTargets = new ID3D11Resource * [nFrames];
    m_ppd2dRenderTargets = new ID2D1Bitmap1 * [nFrames];

    for (int i = 0; i < 4; ++i)
    {
        m_bReadyTextShow[i] = false;
        for (int j = 0; j < 4; ++j) {
            m_bReadyPlayerSkillRects[i][j] = true;
        }
    }
   

    InitializeDevice(pd3dDevice, pd3dCommandQueue, ppd3dRenderTargets);
}

UILayer::UILayer()
{
}

UILayer::~UILayer()
{
    for (int i = 0; i < m_vecGameExplanationText.size(); ++i)
    {
        delete[] m_vecGameExplanationText[i];
    }  
    
    for (int i = 0; i < LOADING_TEXT::LOADING_TEXT_COUNT; ++i)
    {
        delete[] m_LoadingExplanationText[i];
    }
}

UILayer* UILayer::Create(UINT nFrames, UINT nTextBlocks, ID3D12Device* pd3dDevice, ID3D12CommandQueue* pd3dCommandQueue, ID3D12Resource** ppd3dRenderTargets, UINT nWidth, UINT nHeight)
{
    UILayer* pInstance = new UILayer();
    s_instance = pInstance;
    if (FAILED(pInstance->Initialize(nFrames, nTextBlocks, pd3dDevice, pd3dCommandQueue, ppd3dRenderTargets, nWidth, nHeight)))
    {
        SafeDelete(pInstance);
        return nullptr;
    }

    return pInstance;
}

HRESULT UILayer::Initialize(UINT nFrames, UINT nTextBlocks, ID3D12Device* pd3dDevice, ID3D12CommandQueue* pd3dCommandQueue, ID3D12Resource** ppd3dRenderTargets, UINT nWidth, UINT nHeight)
{
    //Title ID
    m_uiRects[0].push_back(UIRect(m_titleIDRect, [this]() -> bool {
        if (!SceneManager::GetInstance()->m_TitleInfo.pw) {
            SceneManager::GetInstance()->m_TitleInfo.Chat.bOnChat = !SceneManager::GetInstance()->m_TitleInfo.Chat.bOnChat;
        }
        else {
            SceneManager::GetInstance()->m_TitleInfo.Chat.bOnChat = true;
        }
        SceneManager::GetInstance()->m_TitleInfo.pw = false;
        m_bChatting = SceneManager::GetInstance()->m_TitleInfo.Chat.bOnChat;

        return true;
        }));

    //Title PW
    m_uiRects[0].push_back(UIRect(m_titlePWRect, [this]() -> bool {
        if (SceneManager::GetInstance()->m_TitleInfo.pw) {
            SceneManager::GetInstance()->m_TitleInfo.Chat.bOnChat = !SceneManager::GetInstance()->m_TitleInfo.Chat.bOnChat;
        }
        else {
            SceneManager::GetInstance()->m_TitleInfo.Chat.bOnChat = true;
        }
        SceneManager::GetInstance()->m_TitleInfo.pw = true;
        m_bChatting = SceneManager::GetInstance()->m_TitleInfo.Chat.bOnChat;

        return true;
        }));

    //Lobby Matching
    m_uiRects[1].push_back(UIRect(D2D1::RectF(FRAME_BUFFER_WIDTH * 0.7740625f, FRAME_BUFFER_HEIGHT * 0.8f, FRAME_BUFFER_WIDTH * 0.75f + FRAME_BUFFER_RESIZE * 0.2f, FRAME_BUFFER_HEIGHT * 0.9f), [this]() -> bool {
        cout << "매칭 눌림" << endl;
        if (!(m_bBossClick || m_bHeroClick) || m_bMatchingClick) {
            return false;
        }
        else {
            m_bMatchingClick = true;
            CTextureShader::GetInstance()->Click(2, true);
            NetworkManager::GetInstance()->SendMatchPacket(UILayer::GetInstance()->GetRole(), true);
            return true;
        }
        }));

    //Lobby Hero
    m_uiRects[1].push_back(UIRect(D2D1::RectF(FRAME_BUFFER_WIDTH * 0.7740625f, FRAME_BUFFER_HEIGHT * 0.75f, FRAME_BUFFER_WIDTH * 0.7740625f + FRAME_BUFFER_RESIZE * 0.1f, FRAME_BUFFER_HEIGHT * 0.8f), [this]() -> bool {
        cout << "영웅 눌림" << endl;
        if (!m_bBossClick) {
            m_bHeroClick = !m_bHeroClick;
            CTextureShader::GetInstance()->Click(0, UILayer::GetInstance()->m_bHeroClick);
            CTextureShader::GetInstance()->Click(1, false);
            m_bBossClick = false;
            m_role = 0;
            return true;
        }
        return true;
        }));

    //Lobby Boss
    m_uiRects[1].push_back(UIRect(D2D1::RectF(FRAME_BUFFER_WIDTH * 0.85f, FRAME_BUFFER_HEIGHT * 0.75f, FRAME_BUFFER_WIDTH * 0.85f + FRAME_BUFFER_RESIZE * 0.1f, FRAME_BUFFER_HEIGHT * 0.8f), [this]() -> bool {
        cout << "보스 눌림" << endl;
        if (!m_bHeroClick) {
            m_bBossClick = !m_bBossClick;
            CTextureShader::GetInstance()->Click(1, UILayer::GetInstance()->m_bBossClick);
            CTextureShader::GetInstance()->Click(0, false);
            m_bHeroClick = false;
            m_role = 1;
            return true;
        }
        return true;
        }));

    //Ready Ready
    m_uiRects[2].push_back(UIRect(D2D1::RectF(FRAME_BUFFER_RESIZE * 0.02f, FRAME_BUFFER_HEIGHT * 0.075f, FRAME_BUFFER_RESIZE * 0.15f, FRAME_BUFFER_HEIGHT * 0.135f), [this]() -> bool {
        ORDER ClientPos = SceneManager::GetInstance()->GetOrder();
        cout << "레디 눌림" << endl;
        if (CTextureShader::GetInstance()->ReadyPlayer(ClientPos)) {
            m_bReadyButtonClick = true;
            //test ready
            m_bReadyTextShow[static_cast<int>(ClientPos)] = !m_bReadyTextShow[static_cast<int>(ClientPos)];
            NetworkManager::GetInstance()->SendReadyPacket(UILayer::GetInstance()->m_bReadyTextShow[static_cast<int>(ClientPos)]);

            if (m_bReadyTextShow[static_cast<int>(ClientPos)]) {
                m_bSkillPopUpClick = false;
                m_bStackPopUpClick = false;
            }
        }

        return true;
        }));

    //Ready Skill PopUp
    m_uiRects[2].push_back(UIRect(D2D1::RectF(FRAME_BUFFER_RESIZE * 0.53f + 231.0f, FRAME_BUFFER_HEIGHT * 0.87f, FRAME_BUFFER_RESIZE * 0.57f + 231.0f, FRAME_BUFFER_HEIGHT * 0.92f), [this]() -> bool {
        ORDER ClientPos = SceneManager::GetInstance()->GetOrder();
        if (!m_bReadyTextShow[static_cast<int>(ClientPos)]) {
            m_bSkillPopUpClick = !m_bSkillPopUpClick;
        }

        return true;
        }));

    //Ready Stack PopUp
    m_uiRects[2].push_back(UIRect(D2D1::RectF(FRAME_BUFFER_RESIZE * 0.88f + 231.0f, FRAME_BUFFER_HEIGHT * 0.87f, FRAME_BUFFER_RESIZE * 0.92f + 231.0f, FRAME_BUFFER_HEIGHT * 0.92f), [this]() -> bool {
        ORDER ClientPos = SceneManager::GetInstance()->GetOrder();
        if (!m_bReadyTextShow[static_cast<int>(ClientPos)])
            m_bStackPopUpClick = !m_bStackPopUpClick;

        return true;
        }));

    for (int i = 0; i < 4; ++i)
    {
        m_PlayerSKillRects[0][i] = D2D1::RectF(FRAME_BUFFER_RESIZE * (0.32f) + 231.0f, FRAME_BUFFER_HEIGHT * (0.5f - i * 0.05f), FRAME_BUFFER_RESIZE * (0.36f) + 231.0f, FRAME_BUFFER_HEIGHT * (0.55f - i * 0.05f));
        m_PlayerSKillRects[1][i] = D2D1::RectF(FRAME_BUFFER_RESIZE * (0.62f) + 231.0f, FRAME_BUFFER_HEIGHT * (0.5f - i * 0.05f), FRAME_BUFFER_RESIZE * (0.66f) + 231.0f, FRAME_BUFFER_HEIGHT * (0.55f - i * 0.05f));
        m_PlayerSKillRects[2][i] = D2D1::RectF(FRAME_BUFFER_RESIZE * (0.9f) + 231.0f, FRAME_BUFFER_HEIGHT * (0.5f - i * 0.05f), FRAME_BUFFER_RESIZE * (0.94f) + 231.0f, FRAME_BUFFER_HEIGHT * (0.55f - i * 0.05f));
    }
    for (int i = 0; i < 4; ++i)
    {
        m_PlayerSKillRects[3][i] = D2D1::RectF(FRAME_BUFFER_RESIZE * (0.315f + i * 0.04f) + 231.0f, FRAME_BUFFER_HEIGHT * (0.46f), FRAME_BUFFER_RESIZE * (0.355f + i * 0.04f) + 231.0f, FRAME_BUFFER_HEIGHT * (0.51f));
    }

    m_fWidth = static_cast<float>(nWidth);
    m_fHeight = static_cast<float>(nHeight);
    m_nRenderTargets = nFrames;
    m_ppd3d11WrappedRenderTargets = new ID3D11Resource * [nFrames];
    m_ppd2dRenderTargets = new ID2D1Bitmap1 * [nFrames];

    m_nTextBlocks = nTextBlocks;
    m_pTextBlocks = new TextBlock[nTextBlocks];

    for (int i = 0; i < 4; ++i)
    {
        m_bReadyTextShow[i] = false;
        for (int j = 0; j < 4; ++j) {
            m_bReadyPlayerSkillRects[i][j] = true;
        }
    }

    //Shop Owner Text
    int mentNum = 3;
    WCHAR** pMent = new WCHAR * [mentNum];
    for (int i = 0; i < mentNum; ++i)
    {
        pMent[i] = new WCHAR[256];
    }

    wcscpy_s(pMent[0], 256, L"난 확률에 조작 같은 거 안해!\n다 독립시행이라니까?");
    wcscpy_s(pMent[1], 256, L"이 가격에 팔면 나는 도대체\n뭘 먹고 사니? 그냥 공짜구만!");
    wcscpy_s(pMent[2], 256, L"이런 거 하지 말고 착실하게 살아.\n가서 열심히 돈을 벌라고.");
    for (int i = 0; i < mentNum; ++i) {
        m_vecShopOwnerMent.push_back(pMent[i]);
    }

    // Shop Parts Name
    vector<wstring> items = {
        L"모자",
        L"얼굴 보호대",
        L"헬멧",
        L"머리카락",
        L"머리 장신구",
        L"망토",
        L"우측 어깨장식",
        L"좌측 어깨장식",
        L"우측 팔장식",
        L"좌측 팔장식",
        L"허리 장신구",
        L"우측 무릎장식",
        L"좌측 무릎장식",
        L"엘프 귀",

        L"머리",
        L"투구",
        L"눈썹",
        L"상의",
        L"우측 상단팔",
        L"좌측 상단팔",
        L"우측 하단팔",
        L"좌측 하단팔",
        L"오른손 장갑",
        L"왼손 장갑",
        L"바지",
        L"오른쪽 신발",
        L"왼쪽 신발",
    };

    m_shopPartsNames.reserve(items.size());
    for (const auto& item : items)
    {
        WCHAR* buffer = new WCHAR[item.size() + 1]; 
        wcscpy_s(buffer, item.size() + 1, item.c_str()); 
        m_shopPartsNames.push_back(buffer); 
    }

    //auction
    for (int i = 0; i < 7; ++i) {
        m_priceRec[i] = D2D1::RectF(FRAME_BUFFER_RESIZE * 0.30556f + 231.0f, FRAME_BUFFER_HEIGHT * (0.31125f + 0.06875f * i),
            FRAME_BUFFER_RESIZE * 0.370375f + 231.0f, FRAME_BUFFER_HEIGHT * (0.34f + 0.06875f * i));
        m_deadLineRec[i] = D2D1::RectF(FRAME_BUFFER_RESIZE * 0.3926f + 231.0f, FRAME_BUFFER_HEIGHT * (0.31f + 0.06875f * i),
            FRAME_BUFFER_RESIZE * 0.48704f + 231.0f, FRAME_BUFFER_HEIGHT * (0.33875f + 0.06875f * i));
        m_userName[i] = D2D1::RectF(FRAME_BUFFER_RESIZE * 0.1713f + 231.0f, FRAME_BUFFER_HEIGHT * (0.31f + 0.06875f * i),
            FRAME_BUFFER_RESIZE * 0.2583f + 231.0f, FRAME_BUFFER_HEIGHT * (0.33875f + 0.06875f * i));
    }

    //skill
    for (int i = 0; i < MAX_SKILL; ++i) {
        m_SkilKeyRec[i] = D2D1::RectF(FRAME_BUFFER_RESIZE * (0.5704f + (0.041667f * i)) + 231.0f, FRAME_BUFFER_HEIGHT * 0.9675f,
            FRAME_BUFFER_RESIZE * (0.610215f + (0.041667f * i)) + 231.0f, FRAME_BUFFER_HEIGHT * 0.98625f);
        m_SkilCoolTimeRec[i] = D2D1::RectF(FRAME_BUFFER_RESIZE * (0.5704f + (0.041667f * i)) + 231.0f, FRAME_BUFFER_HEIGHT * 0.95025f,
            FRAME_BUFFER_RESIZE * (0.610215f + (0.041667f * i)) + 231.0f, FRAME_BUFFER_HEIGHT * 0.97f);
    }

    //Loading Text
    int textNum = 11;
    WCHAR** pExplantionText = new WCHAR*[textNum];
    for (int i = 0; i < textNum; ++i)
    {
        pExplantionText[i] = new WCHAR[256];
    }


    wcscpy_s(pExplantionText[0], 256, L"War Of Dimension은 3인칭 비대칭 AOS 게임입니다.\n");
    wcscpy_s(pExplantionText[1], 256, L"영웅이 고를 수 있는 스킬은 12개, 보스는 10개 입니다.\n");
    wcscpy_s(pExplantionText[2], 256, L"Body Strength라는 기술을 쓰면 이동속도가 50% 증가합니다.\n");
    wcscpy_s(pExplantionText[3], 256, L"필드몬스터는 노멀, 레어, 유니크 세 등급으로 나눠져있습니다.\n");
    wcscpy_s(pExplantionText[4], 256, L"War Of Dimension은 총 6개의 직업과 68개의 스킬이 있습니다.\n");
    wcscpy_s(pExplantionText[5], 256, L"필드몬스터는 매번 랜덤한 위치에서 스폰됩니다.\n");
    wcscpy_s(pExplantionText[6], 256, L"사실 이 게임의 개발자 중 막내는 강현석입니다.\n");
    wcscpy_s(pExplantionText[7], 256, L"War Of Dimension은 약 700개의 커스터마이징 부품이 존재합니다.\n");
    wcscpy_s(pExplantionText[8], 256, L"War Of Dimension의 경제 시스템은 블록체인을 활용합니다.\n");
    wcscpy_s(pExplantionText[9], 256, L"스탯 물약 아이템은 1분간 지속됩니다.\n");
    wcscpy_s(pExplantionText[10], 256, L"스탯은 9종류로 체력, 마나, 공격력, 마법 공격력, 방어력, 마법 방어력, 속도, 강인함, 치명타가 있습니다.\n");


    for (int i = 0; i < textNum; ++i) {
        m_vecGameExplanationText.push_back(pExplantionText[i]);
    }

    m_iNumText = Util::GenerateRandomInt(0, textNum - 1);
    

    InitializeDevice(pd3dDevice, pd3dCommandQueue, ppd3dRenderTargets);

    m_brushes[BRUSH_COLOR::WHITE] = CreateBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f));
    m_brushes[BRUSH_COLOR::LIME_GREEN] = CreateBrush(D2D1::ColorF(1.0f, 1.0f, 0.501f, 1.0f));
    m_brushes[BRUSH_COLOR::SKY_BLUE] = CreateBrush(D2D1::ColorF(0.6f, 0.85f, 0.917f, 1.0f));
    m_brushes[BRUSH_COLOR::DARK_CRIMSON] = CreateBrush(D2D1::ColorF(0.96f, 0.19f, 0.03f, 1.0f));
    m_brushes[BRUSH_COLOR::DARK_GRAY] = CreateBrush(D2D1::ColorF(0.8f, 0.8f, 0.8f, 1.0f));
    m_brushes[BRUSH_COLOR::RED] = CreateBrush(D2D1::ColorF(1.0f, 0.14f, 0.14f, 1.0f));

    m_textFormats[TEXT_SIZE::SIZE_15] = CreateTextFormat(L"Koverwatch", 15.0f * 1.35f);
    m_textFormats[TEXT_SIZE::SIZE_18] = CreateTextFormat(L"Koverwatch", 18.0f* 1.35f);
    m_textFormats[TEXT_SIZE::SIZE_25] = CreateTextFormat(L"Koverwatch", 25.0f* 1.35f);
    m_textFormats[TEXT_SIZE::SIZE_30] = CreateTextFormat(L"Koverwatch", 30.0f* 1.35f);
    m_textFormats[TEXT_SIZE::SIZE_50] = CreateTextFormat(L"Koverwatch", 50.0f* 1.35f);
    m_textFormats[TEXT_SIZE::SIZE_60] = CreateTextFormat(L"Koverwatch", 60.0f* 1.35f);

    m_textLeftFormats[TEXT_SIZE::SIZE_15] = CreateTextFormatLeft(L"Koverwatch", 15.0f* 1.35f);
    m_textLeftFormats[TEXT_SIZE::SIZE_18] = CreateTextFormatLeft(L"Koverwatch", 18.0f* 1.35f);
    m_textLeftFormats[TEXT_SIZE::SIZE_25] = CreateTextFormatLeft(L"Koverwatch", 25.0f* 1.35f);
    m_textLeftFormats[TEXT_SIZE::SIZE_30] = CreateTextFormatLeft(L"Koverwatch", 30.0f* 1.35f);
    m_textLeftFormats[TEXT_SIZE::SIZE_50] = CreateTextFormatLeft(L"Koverwatch", 50.0f* 1.35f);
    m_textLeftFormats[TEXT_SIZE::SIZE_60] = CreateTextFormatLeft(L"Koverwatch", 60.0f* 1.35f);

   m_textRightFormats[TEXT_SIZE::SIZE_15] = CreateTextFormatRight(L"Koverwatch", 15.0f* 1.35f);
   m_textRightFormats[TEXT_SIZE::SIZE_18] = CreateTextFormatRight(L"Koverwatch", 18.0f* 1.35f);
   m_textRightFormats[TEXT_SIZE::SIZE_25] = CreateTextFormatRight(L"Koverwatch", 25.0f* 1.35f);
   m_textRightFormats[TEXT_SIZE::SIZE_30] = CreateTextFormatRight(L"Koverwatch", 30.0f* 1.35f);
   m_textRightFormats[TEXT_SIZE::SIZE_50] = CreateTextFormatRight(L"Koverwatch", 50.0f* 1.35f);
   m_textRightFormats[TEXT_SIZE::SIZE_60] = CreateTextFormatRight(L"Koverwatch", 60.0f* 1.35f);
    

    int LoadingExtextNum = static_cast<int>(LOADING_TEXT::LOADING_TEXT_COUNT);
    WCHAR** pLoadingExplantionText = new WCHAR * [LoadingExtextNum];
    for (int i = 0; i < LoadingExtextNum; ++i)
    {
        pLoadingExplantionText[i] = new WCHAR[256];
    }


    wcscpy_s(pLoadingExplantionText[LOADING_TEXT::MAP], 256, L"맵 로드 중\n");
    wcscpy_s(pLoadingExplantionText[LOADING_TEXT::CHARACTER], 256, L"캐릭터 로드 중\n");
    wcscpy_s(pLoadingExplantionText[LOADING_TEXT::SHADER], 256, L"셰이더 로드 중\n");

    m_LoadingExplanationText[LOADING_TEXT::MAP] = pLoadingExplantionText[LOADING_TEXT::MAP];
    m_LoadingExplanationText[LOADING_TEXT::CHARACTER] = pLoadingExplantionText[LOADING_TEXT::CHARACTER];
    m_LoadingExplanationText[LOADING_TEXT::SHADER] = pLoadingExplantionText[LOADING_TEXT::SHADER];

    // Ready Scene Stats
    const int extraPoint = 1;
    const int statTexts = extraPoint + 9; // points + stat value
    for (int i = 0; i < statTexts; ++i)
    {
        m_AdditionalStatsRect[i] = i < extraPoint ?
            D2D1::RectF(FRAME_BUFFER_RESIZE * (0.8411f) + 231.0f, FRAME_BUFFER_HEIGHT * (0.84179f),
                FRAME_BUFFER_RESIZE * (0.89338f) + 231.0f, FRAME_BUFFER_HEIGHT * (0.86697f)) :
            D2D1::RectF(FRAME_BUFFER_RESIZE * (0.7744f) + 231.0f, FRAME_BUFFER_HEIGHT* (0.513f + 0.0375f * (i-1)),
                FRAME_BUFFER_RESIZE * (0.8267f) + 231.0f, FRAME_BUFFER_HEIGHT* (0.53817f + 0.0375f * (i - 1)));
    }

    return NOERROR;
}

void UILayer::Reset()
{
    m_fPlayerMaxHp = 0.f;
    m_fPlayerCurHp = 0.f;
    m_fPlayerMaxMp = 0.f;
    m_fPlayerCurMp = 0.f;
    m_bChatting = false;
    m_bFlicker = false;
    m_bMatchingClick = false;
    m_bHeroClick = false;
    m_bBossClick = false;
    m_bReadyButtonClick = false;
    m_bSkillPopUpClick = false;
    for (int i = 0; i < 4; ++i)
    {
        m_bReadyTextShow[i] = false;
        for (int j = 0; j < 4; ++j) {
            m_bReadyPlayerSkillRects[i][j] = true;
        }
    }
    for (int i = 0; i < 5; ++i) {
        m_skillCoolRemainingTime[i] = 0;
    }
    m_fRemainingTime = 60.0f;
}

void UILayer::ProcessMouseClick(SCENEKIND sceneKind, POINT clickPos)
{
    for (UIRect& rect : m_uiRects[static_cast<int>(sceneKind) - 1]) {
        if (rect.ClickCollide(clickPos))
            break;
    }
}

void UILayer::ResetLoadingValue()
{
    m_fTotalTime = 0.f;
    m_iNumText = Util::GenerateRandomInt(0, static_cast<int>(m_vecGameExplanationText.size() - 1));
    m_fLoadingPreProgressPercent = 0.f;
    m_fLoadingNowProgressPercent = 0.f;
    m_fLerpProgressing = 0.f;
    m_bProgressing = false;
    m_fProgress = 0.f;
    m_fLerpProgress = 0.f;
}

void UILayer::GenRandomMent()
{
    m_nMent = Util::GenerateRandomInt(0, static_cast<int>(m_vecShopOwnerMent.size() - 1));
}

void UILayer::InitializeDevice(ID3D12Device* pd3dDevice, ID3D12CommandQueue* pd3dCommandQueue, ID3D12Resource** ppd3dRenderTargets)
{
    UINT d3d11DeviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
    D2D1_FACTORY_OPTIONS d2dFactoryOptions = { };

#if defined(_DEBUG) || defined(DBG)
    d2dFactoryOptions.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
    d3d11DeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    ID3D11Device* pd3d11Device = NULL;
    ID3D12CommandQueue* ppd3dCommandQueues[] = { pd3dCommandQueue };
    HRESULT hr = ::D3D11On12CreateDevice(pd3dDevice, d3d11DeviceFlags, nullptr, 0, reinterpret_cast<IUnknown**>(ppd3dCommandQueues), _countof(ppd3dCommandQueues), 0, (ID3D11Device**)&pd3d11Device, (ID3D11DeviceContext**)&m_pd3d11DeviceContext, nullptr);
    pd3d11Device->QueryInterface(__uuidof(ID3D11On12Device), (void**)&m_pd3d11On12Device);
    pd3d11Device->Release();

#if defined(_DEBUG) || defined(DBG)
    ID3D12InfoQueue* pd3dInfoQueue;
    if (SUCCEEDED(pd3dDevice->QueryInterface(IID_PPV_ARGS(&pd3dInfoQueue))))
    {
        D3D12_MESSAGE_SEVERITY pd3dSeverities[] = { D3D12_MESSAGE_SEVERITY_INFO };
        D3D12_MESSAGE_ID pd3dDenyIds[] = { D3D12_MESSAGE_ID_INVALID_DESCRIPTOR_HANDLE };

        D3D12_INFO_QUEUE_FILTER d3dInforQueueFilter = { };
        d3dInforQueueFilter.DenyList.NumSeverities = _countof(pd3dSeverities);
        d3dInforQueueFilter.DenyList.pSeverityList = pd3dSeverities;
        d3dInforQueueFilter.DenyList.NumIDs = _countof(pd3dDenyIds);
        d3dInforQueueFilter.DenyList.pIDList = pd3dDenyIds;

        pd3dInfoQueue->PushStorageFilter(&d3dInforQueueFilter);
    }
    pd3dInfoQueue->Release();
#endif

    IDXGIDevice* pdxgiDevice = NULL;
    m_pd3d11On12Device->QueryInterface(__uuidof(IDXGIDevice), (void**)&pdxgiDevice);

    ::D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory3), &d2dFactoryOptions, (void**)&m_pd2dFactory);
    HRESULT hResult = m_pd2dFactory->CreateDevice(pdxgiDevice, (ID2D1Device2**)&m_pd2dDevice);
    m_pd2dDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, (ID2D1DeviceContext2**)&m_pd2dDeviceContext);

    m_pd2dDeviceContext->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);

    ::DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), (IUnknown**)&m_pd2dWriteFactory);
    pdxgiDevice->Release();

    D2D1_BITMAP_PROPERTIES1 d2dBitmapProperties = D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW, D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED));

    for (UINT i = 0; i < m_nRenderTargets; i++)
    {
        D3D11_RESOURCE_FLAGS d3d11Flags = { D3D11_BIND_RENDER_TARGET };
        m_pd3d11On12Device->CreateWrappedResource(ppd3dRenderTargets[i], &d3d11Flags, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT, IID_PPV_ARGS(&m_ppd3d11WrappedRenderTargets[i]));
        IDXGISurface* pdxgiSurface = NULL;
        m_ppd3d11WrappedRenderTargets[i]->QueryInterface(__uuidof(IDXGISurface), (void**)&pdxgiSurface);
        m_pd2dDeviceContext->CreateBitmapFromDxgiSurface(pdxgiSurface, &d2dBitmapProperties, &m_ppd2dRenderTargets[i]);
        pdxgiSurface->Release();
    }
}

void UILayer::PushChating(WCHAR* chat)
{
    if (m_pListReadyChating.size() >= 7)
    {
        WCHAR* temp = m_pListReadyChating.front();
        m_pListReadyChating.pop_front();
        delete[] temp;
    }
    m_pListReadyChating.push_back(chat);
}

ID2D1SolidColorBrush* UILayer::CreateBrush(D2D1::ColorF d2dColor)
{
    ID2D1SolidColorBrush* pd2dDefaultTextBrush = NULL;
    m_pd2dDeviceContext->CreateSolidColorBrush(d2dColor, &pd2dDefaultTextBrush);

    return(pd2dDefaultTextBrush);
}

IDWriteTextFormat* UILayer::CreateTextFormat(const WCHAR* pszFontName, float fFontSize)
{
    IDWriteTextFormat* pdwDefaultTextFormat = NULL;
    m_pd2dWriteFactory->CreateTextFormat(pszFontName, nullptr, DWRITE_FONT_WEIGHT_BOLD, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, fFontSize, L"en-us", &pdwDefaultTextFormat);

    pdwDefaultTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    pdwDefaultTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);    

    return(pdwDefaultTextFormat);
}

IDWriteTextFormat* UILayer::CreateTextFormatLeft(const WCHAR* pszFontName, float fFontSize)
{
    IDWriteTextFormat* pdwDefaultTextFormat = NULL;
    //m_pd2dWriteFactory->CreateTextFormat(pszFontName, nullptr, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, fFontSize, L"en-us", &pdwDefaultTextFormat);
    m_pd2dWriteFactory->CreateTextFormat(pszFontName, nullptr, DWRITE_FONT_WEIGHT_BOLD, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, fFontSize, L"en-us", &pdwDefaultTextFormat);

    pdwDefaultTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
    pdwDefaultTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    //m_pd2dWriteFactory->CreateTextFormat(L"Arial", nullptr, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, fSmallFontSize, L"en-us", &m_pdwDefaultTextFormat);

    return(pdwDefaultTextFormat);
}

IDWriteTextFormat* UILayer::CreateTextFormatRight(const WCHAR* pszFontName, float fFontSize)
{
    IDWriteTextFormat* pdwDefaultTextFormat = NULL;
    m_pd2dWriteFactory->CreateTextFormat(pszFontName, nullptr, DWRITE_FONT_WEIGHT_BOLD, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, fFontSize, L"en-us", &pdwDefaultTextFormat);

    pdwDefaultTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
    pdwDefaultTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    
    return(pdwDefaultTextFormat);
}

void UILayer::UpdateTextOutputs(UINT nIndex, const WCHAR* pstrUIText, D2D1_RECT_F* pd2dLayoutRect, IDWriteTextFormat* pdwFormat, ID2D1SolidColorBrush* pd2dTextBrush)
{
    if (pstrUIText) wcscpy_s(m_pTextBlocks[nIndex].m_pstrText, 256, pstrUIText);
    if (pd2dLayoutRect) m_pTextBlocks[nIndex].m_d2dLayoutRect = *pd2dLayoutRect;
    if (pdwFormat) m_pTextBlocks[nIndex].m_pdwFormat = pdwFormat;
    if (pd2dTextBrush) m_pTextBlocks[nIndex].m_pd2dTextBrush = pd2dTextBrush;
}

void UILayer::Render(UINT nFrame, bool LoadingRender)
{
    ID3D11Resource* ppResources[] = { m_ppd3d11WrappedRenderTargets[nFrame] };

    m_pd2dDeviceContext->SetTarget(m_ppd2dRenderTargets[nFrame]);
    m_pd3d11On12Device->AcquireWrappedResources(ppResources, _countof(ppResources));

    WCHAR wLoadingChat[256] = L"";
    WCHAR pDotText[256] = L"";
    WCHAR pPercentText[256] = L"";
    WCHAR pLoadingExplanationText[256] = L"";

    switch (SceneManager::GetInstance()->m_nCurScene)
    {
    case SCENEKIND::TITLE:
    {
        m_pd2dDeviceContext->BeginDraw();
        WCHAR wChat[256] = L"";
        WCHAR wPass[256] = L"";
        wcscat(wChat, SceneManager::GetInstance()->m_TitleInfo.Chat.ChatBuf);

        for (int i = 0; i < BUF_SIZE - 1; ++i) {
            if (SceneManager::GetInstance()->m_TitleInfo.passBuf[i + 1] != '\0') {
                wPass[i] = '*';
            }
            else {
                if (SceneManager::GetInstance()->m_TitleInfo.passShow[i])
                    wPass[i] = SceneManager::GetInstance()->m_TitleInfo.passBuf[i];
                else
                    wPass[i] = '*';
                wPass[i + 1] = '\0';
                break;
            }
        }

        if (SceneManager::GetInstance()->m_TitleInfo.Chat.bOnChat)
        {
            if (m_bFlicker) {
                if (SceneManager::GetInstance()->m_TitleInfo.pw) {
                    wcscat(wPass, L"I");
                }
                else {
                    wcscat(wChat, L"I");
                }   
            }
        }

        m_pd2dDeviceContext->DrawText(wChat, (UINT)wcslen(wChat), m_textLeftFormats[TEXT_SIZE::SIZE_25], m_titleIDRect, m_brushes[BRUSH_COLOR::WHITE]);
        m_pd2dDeviceContext->DrawText(wPass, (UINT)wcslen(wPass), m_textLeftFormats[TEXT_SIZE::SIZE_25], m_titlePWRect, m_brushes[BRUSH_COLOR::WHITE]);
        m_pd2dDeviceContext->EndDraw();
        break;
    }
    case SCENEKIND::LOBBY:
    {
        if (!LoadingRender)
        {
            D2D1_RECT_F ChatingRectArray[7];
            float RectHeight = m_ChatingRec.bottom - m_ChatingRec.top;
            for (int i = 0; i < 7; ++i)
            {
                ChatingRectArray[i] = D2D1::RectF(m_ChatingRec.left, m_ChatingRec.bottom - RectHeight / 7 * (i + 1), m_ChatingRec.right, m_ChatingRec.bottom - RectHeight / 7 * i);
            }

            m_pd2dDeviceContext->BeginDraw();

            WCHAR wChat[256] = L"";
            WCHAR wChatOption[256] = L"";

            wcscat(wChat, SceneManager::GetInstance()->m_LobbyInfo.Chat.ChatBuf);
            wcscat(wChat, SceneManager::GetInstance()->m_LobbyInfo.Chat.TempChatBuf);

            if (!CTextureShader::GetInstance()->IsCustomize()) {
                m_bChatting = SceneManager::GetInstance()->m_LobbyInfo.Chat.bOnChat;
                if (SceneManager::GetInstance()->m_LobbyInfo.Chat.bOnChat)
                {
                    if (m_bFlicker)
                    {
                        wcscat(wChat, L"I");
                    }
                }

                switch (SceneManager::GetInstance()->m_LobbyInfo.Chat.eChatOption)
                {
                case CHAT::ALL:
                    wcscpy_s(wChatOption, 256, L"전 체\n");
                    m_pd2dDeviceContext->DrawText(wChatOption, (UINT)wcslen(wChatOption), m_textFormats[TEXT_SIZE::SIZE_15], m_ChatOptionTextRec, m_brushes[BRUSH_COLOR::LIME_GREEN]);
                    m_pd2dDeviceContext->DrawText(wChat, (UINT)wcslen(wChat), m_textLeftFormats[TEXT_SIZE::SIZE_15], m_ChatTypingRec, m_brushes[BRUSH_COLOR::LIME_GREEN]);
                    break;
                case CHAT::CHANNEL:
                    wcscpy_s(wChatOption, 256, L"채 널\n");
                    m_pd2dDeviceContext->DrawText(wChatOption, (UINT)wcslen(wChatOption), m_textFormats[TEXT_SIZE::SIZE_15], m_ChatOptionTextRec, m_brushes[BRUSH_COLOR::WHITE]);
                    m_pd2dDeviceContext->DrawText(wChat, (UINT)wcslen(wChat), m_textLeftFormats[TEXT_SIZE::SIZE_15], m_ChatTypingRec, m_brushes[BRUSH_COLOR::WHITE]);
                    break;
              /*  case CHAT::PARTY:
                    wcscpy_s(wChatOption, 256, L"파 티\n");
                    m_pd2dDeviceContext->DrawText(wChatOption, (UINT)wcslen(wChatOption), m_textFormats[TEXT_SIZE::SIZE_15], m_ChatOptionTextRec, m_brushes[BRUSH_COLOR::SKY_BLUE]);
                    m_pd2dDeviceContext->DrawText(wChat, (UINT)wcslen(wChat), m_textLeftFormats[TEXT_SIZE::SIZE_15], m_ChatTypingRec, m_brushes[BRUSH_COLOR::SKY_BLUE]);
                    break;*/
                default:
                    break;
                }

                int count = 0;
                list<SERVERCHAT> ListChating = NetworkManager::GetInstance()->ListChating;
                for (auto iter = ListChating.rbegin(); iter != ListChating.rend(); ++iter)
                {
                    switch (iter->op)
                    {
                    case CHAT::ALL:
                        m_pd2dDeviceContext->DrawText(iter->chating, (UINT)wcslen(iter->chating), m_textLeftFormats[TEXT_SIZE::SIZE_15], ChatingRectArray[count], m_brushes[BRUSH_COLOR::LIME_GREEN]);
                        break;
                    case CHAT::CHANNEL:
                        m_pd2dDeviceContext->DrawText(iter->chating, (UINT)wcslen(iter->chating), m_textLeftFormats[TEXT_SIZE::SIZE_15], ChatingRectArray[count], m_brushes[BRUSH_COLOR::WHITE]);
                        break;
                    /*case CHAT::PARTY:
                        m_pd2dDeviceContext->DrawText(iter->chating, (UINT)wcslen(iter->chating), m_textLeftFormats[TEXT_SIZE::SIZE_15], ChatingRectArray[count], m_brushes[BRUSH_COLOR::SKY_BLUE]);
                        break;*/
                    default:
                        break;
                    }
                    ++count;
                }

                WCHAR pstrToken[256] = L"";
                int token = NetworkManager::GetInstance()->GetTokenNum();
                wstring tokenStrig = to_wstring(token);
                wcscpy_s(pstrToken, tokenStrig.c_str());

                m_pd2dDeviceContext->DrawText(pstrToken, (UINT)wcslen(pstrToken), m_textLeftFormats[TEXT_SIZE::SIZE_30], m_tokenRec, m_brushes[BRUSH_COLOR::WHITE]);

            }
            WCHAR pstrToken[256] = L"";
            int token = NetworkManager::GetInstance()->GetTokenNum();
            bool isRenderSetting = CTextureShader::GetInstance()->IsSetting();
            if (isRenderSetting)
            {

                float exposure = SceneManager::GetInstance()->m_fExposure;
                float saturation = SceneManager::GetInstance()->m_fSaturation;
                float contrast = SceneManager::GetInstance()->m_fContrast;
                float vibrance = SceneManager::GetInstance()->m_fVibrance;
                int volume = CTextureShader::GetInstance()->GetVolume();

                wstringstream valStream;

                valStream << fixed << setprecision(1) << exposure;
                wstring valString =  valStream.str();
                wcscpy_s(pstrToken, valString.c_str());
                m_pd2dDeviceContext->DrawText(pstrToken, (UINT)wcslen(pstrToken), m_textFormats[TEXT_SIZE::SIZE_18], m_exposureRec, m_brushes[BRUSH_COLOR::WHITE]);

                valStream.str(L"");
                valStream << fixed << setprecision(1) << saturation;
                valString = valStream.str();
                wcscpy_s(pstrToken, valString.c_str());
                m_pd2dDeviceContext->DrawText(pstrToken, (UINT)wcslen(pstrToken), m_textFormats[TEXT_SIZE::SIZE_18], m_saturationRec, m_brushes[BRUSH_COLOR::WHITE]);

                valStream.str(L"");
                valStream << fixed << setprecision(1) << contrast;
                valString = valStream.str();
                wcscpy_s(pstrToken, valString.c_str());
                m_pd2dDeviceContext->DrawText(pstrToken, (UINT)wcslen(pstrToken), m_textFormats[TEXT_SIZE::SIZE_18], m_contrastRec, m_brushes[BRUSH_COLOR::WHITE]);

                valStream.str(L"");
                valStream << fixed << setprecision(1) << vibrance;
                valString = valStream.str();
                wcscpy_s(pstrToken, valString.c_str());
                m_pd2dDeviceContext->DrawText(pstrToken, (UINT)wcslen(pstrToken), m_textFormats[TEXT_SIZE::SIZE_18], m_vibranceRec, m_brushes[BRUSH_COLOR::WHITE]);

                valString = to_wstring(volume);
                wcscpy_s(pstrToken, valString.c_str());
                m_pd2dDeviceContext->DrawText(pstrToken, (UINT)wcslen(pstrToken), m_textFormats[TEXT_SIZE::SIZE_30], m_volumeRec, m_brushes[BRUSH_COLOR::WHITE]);
            }

            bool isRenderShop = CTextureShader::GetInstance()->IsRandomShop();
            if (isRenderShop)
            {
                m_pd2dDeviceContext->DrawText(m_vecShopOwnerMent[m_nMent], (UINT)wcslen(m_vecShopOwnerMent[m_nMent]),
                    m_textLeftFormats[TEXT_SIZE::SIZE_25], m_shopRec, m_brushes[BRUSH_COLOR::WHITE]);

                int curParts = CTextureShader::GetInstance()->GetCurParts();
                m_pd2dDeviceContext->DrawText(m_shopPartsNames[curParts], (UINT)wcslen(m_shopPartsNames[curParts]),
                    m_textFormats[TEXT_SIZE::SIZE_25], m_partsRec, m_brushes[BRUSH_COLOR::WHITE]);
            }

            bool isRenderAuction = CTextureShader::GetInstance()->IsAuction();
            LobbyAuctionInfo* info = CTextureShader::GetInstance()->m_auctionInfo;
            wstring infoString;
            if (isRenderAuction)
            {
                infoString = to_wstring(token);
                wcscpy_s(pstrToken, infoString.c_str());
                m_pd2dDeviceContext->DrawText(pstrToken, (UINT)wcslen(pstrToken), m_textLeftFormats[TEXT_SIZE::SIZE_15], m_auctionGoldRec, m_brushes[BRUSH_COLOR::WHITE]);

                for (int i = 0; i < 7; ++i) {
                    if (info->closingTime[i] != 0) {
                        infoString = to_wstring(info->productPrice[i]);
                        wcscpy_s(pstrToken, infoString.c_str());
                        m_pd2dDeviceContext->DrawText(pstrToken, (UINT)wcslen(pstrToken), m_textLeftFormats[TEXT_SIZE::SIZE_18], m_priceRec[i], m_brushes[BRUSH_COLOR::WHITE]);

                        int time = info->closingTime[i];
                        int hour = time / 60;
                        int minute = time % 60;
                        swprintf(pstrToken, L"%d시간 %d분", hour, minute);
                        m_pd2dDeviceContext->DrawText(pstrToken, (UINT)wcslen(pstrToken), m_textLeftFormats[TEXT_SIZE::SIZE_18], m_deadLineRec[i], m_brushes[BRUSH_COLOR::WHITE]);

                        std::string charArray = info->username[i];

                        int wideStrLength = MultiByteToWideChar(CP_UTF8, 0, charArray.c_str(), -1, NULL, 0);
                        MultiByteToWideChar(CP_UTF8, 0, charArray.c_str(), -1, &infoString[0], wideStrLength);
                        wcscpy_s(pstrToken, infoString.c_str());
                        m_pd2dDeviceContext->DrawText(pstrToken, (UINT)wcslen(pstrToken), m_textLeftFormats[TEXT_SIZE::SIZE_18], m_userName[i], m_brushes[BRUSH_COLOR::WHITE]);
                    }
                }
                infoString = to_wstring(info->curPage + 1);
                wcscpy_s(pstrToken, infoString.c_str());
                m_pd2dDeviceContext->DrawText(pstrToken, (UINT)wcslen(pstrToken), m_textFormats[TEXT_SIZE::SIZE_50], m_pageRec, m_brushes[BRUSH_COLOR::WHITE]);
                m_pd2dDeviceContext->DrawText(m_shopPartsNames[info->curParts], (UINT)wcslen(m_shopPartsNames[info->curParts]),
                    m_textFormats[TEXT_SIZE::SIZE_18], m_auctionPartsRec, m_brushes[BRUSH_COLOR::WHITE]);

                infoString = to_wstring(info->curIndex + 1);
                wcscpy_s(pstrToken, infoString.c_str());
                m_pd2dDeviceContext->DrawText(pstrToken, (UINT)wcslen(pstrToken), m_textFormats[TEXT_SIZE::SIZE_18],
                    m_auctionNumRec, m_brushes[BRUSH_COLOR::WHITE]);

                infoString = to_wstring(info->sellPrice);
                wcscpy_s(pstrToken, infoString.c_str());
                m_pd2dDeviceContext->DrawText(pstrToken, (UINT)wcslen(pstrToken), m_textLeftFormats[TEXT_SIZE::SIZE_30],
                    m_myPriceRec, m_brushes[BRUSH_COLOR::WHITE]);
            }

            bool isRenderBlockChain = CTextureShader::GetInstance()->IsBlockchain();
            LobbyBlockChainInfo* blockChainInfo = CTextureShader::GetInstance()->m_blockChainInfo;
            if (isRenderBlockChain)
            {
                infoString = to_wstring(token);
                wcscpy_s(pstrToken, infoString.c_str());
                m_pd2dDeviceContext->DrawText(pstrToken, (UINT)wcslen(pstrToken), m_textLeftFormats[TEXT_SIZE::SIZE_30], 
                    m_blockChainMyGoldRec, m_brushes[BRUSH_COLOR::WHITE]);

                infoString = to_wstring(blockChainInfo->stakingPeriod);
                infoString += L"일";
                wcscpy_s(pstrToken, infoString.c_str());
                m_pd2dDeviceContext->DrawText(pstrToken, (UINT)wcslen(pstrToken), m_textLeftFormats[TEXT_SIZE::SIZE_30],
                    m_periodRec, m_brushes[BRUSH_COLOR::WHITE]);

                infoString = to_wstring(blockChainInfo->inputTokens);
                wcscpy_s(pstrToken, infoString.c_str());
                m_pd2dDeviceContext->DrawText(pstrToken, (UINT)wcslen(pstrToken), m_textLeftFormats[TEXT_SIZE::SIZE_30],
                    m_inputTokenRec, m_brushes[BRUSH_COLOR::WHITE]);

                infoString = to_wstring(blockChainInfo->stakingTokens);
                wcscpy_s(pstrToken, infoString.c_str());
                m_pd2dDeviceContext->DrawText(pstrToken, (UINT)wcslen(pstrToken), m_textLeftFormats[TEXT_SIZE::SIZE_30],
                    m_stakingTokenRec, m_brushes[BRUSH_COLOR::WHITE]);

                infoString = to_wstring(blockChainInfo->unstakingDeadline);
                infoString += L"일";
                wcscpy_s(pstrToken, infoString.c_str());
                m_pd2dDeviceContext->DrawText(pstrToken, (UINT)wcslen(pstrToken), m_textLeftFormats[TEXT_SIZE::SIZE_30],
                    m_unstakingPeriodRec, m_brushes[BRUSH_COLOR::WHITE]);

            }

            bool isRenderCustomize = CTextureShader::GetInstance()->IsCustomize();
            if (isRenderCustomize)
            { 
                m_pd2dDeviceContext->DrawText(m_shopPartsNames[CTextureShader::GetInstance()->m_curParts], 
                    (UINT)wcslen(m_shopPartsNames[CTextureShader::GetInstance()->m_curParts]), m_textFormats[TEXT_SIZE::SIZE_50],
                    m_customizePartsRec, m_brushes[BRUSH_COLOR::WHITE]);

                infoString = to_wstring(CTextureShader::GetInstance()->m_nCurParts); 
                wcscpy_s(pstrToken, infoString.c_str());  
                m_pd2dDeviceContext->DrawText(pstrToken, (UINT)wcslen(pstrToken) + 1, m_textFormats[TEXT_SIZE::SIZE_50], 
                    m_customizeNumRec, m_brushes[BRUSH_COLOR::WHITE]);
            }

            m_pd2dDeviceContext->EndDraw();
            
        }
        else
        {
            wcscpy_s(pLoadingExplanationText, 256, L"로비 씬 ");
            if (m_fLoadingPreProgressPercent <= 0.f)
                wcscat(pLoadingExplanationText, m_LoadingExplanationText[LOADING_TEXT::MAP]);
            else if (m_fLoadingPreProgressPercent < SceneManager::GetInstance()->ToLobbyPercent[LOADING_TEXT::CHARACTER])
                wcscat(pLoadingExplanationText, m_LoadingExplanationText[LOADING_TEXT::CHARACTER]);
            else if (m_fLoadingPreProgressPercent < SceneManager::GetInstance()->ToLobbyPercent[LOADING_TEXT::SHADER])
                wcscat(pLoadingExplanationText, m_LoadingExplanationText[LOADING_TEXT::SHADER]);
            else if (m_fLoadingPreProgressPercent >= SceneManager::GetInstance()->ToLobbyPercent[LOADING_TEXT::SHADER])
                wcscat(pLoadingExplanationText, L"로드 완료");
        }
        break;
    }
    case SCENEKIND::READY:
    {
        if (!LoadingRender)
        {
            if (SceneManager::GetInstance()->GetOrder() != ORDER::BOSS)
            {
                //Chating
                D2D1_RECT_F ReadySceneChating = D2D1::RectF(FRAME_BUFFER_RESIZE * 0.02f, FRAME_BUFFER_HEIGHT * 0.59f, FRAME_BUFFER_RESIZE * 0.25f, FRAME_BUFFER_HEIGHT * 0.87f);
                D2D1_RECT_F ChatTypingRec = D2D1::RectF(FRAME_BUFFER_RESIZE * 0.02f, FRAME_BUFFER_HEIGHT * 0.88f, FRAME_BUFFER_RESIZE * 0.25f, FRAME_BUFFER_HEIGHT * 0.92f);

                D2D1_RECT_F ChatingRectArray[7];
                float RectHeight = ReadySceneChating.bottom - ReadySceneChating.top;
                for (int i = 0; i < 7; ++i)
                {
                    ChatingRectArray[i] = D2D1::RectF(ReadySceneChating.left, ReadySceneChating.bottom - RectHeight / 7 * (i + 1), ReadySceneChating.right, ReadySceneChating.bottom - RectHeight / 7 * i);
                }

                //Game Ready
                float ReadyPRectSizeX = FRAME_BUFFER_RESIZE * 0.16f;
                float ReadyPRectSizeY = FRAME_BUFFER_HEIGHT * 0.06f;
                D2D1_RECT_F ReadyPlayer1TextRec = D2D1::RectF(FRAME_BUFFER_RESIZE * 0.14f + 231.0f, FRAME_BUFFER_HEIGHT * 0.4f, FRAME_BUFFER_RESIZE * 0.14f + ReadyPRectSizeX + 231.0f, FRAME_BUFFER_HEIGHT * 0.4f + ReadyPRectSizeY);
                D2D1_RECT_F ReadyPlayer2TextRec = D2D1::RectF(FRAME_BUFFER_RESIZE * 0.45f + 231.0f, FRAME_BUFFER_HEIGHT * 0.4f, FRAME_BUFFER_RESIZE * 0.45f + ReadyPRectSizeX + 231.0f, FRAME_BUFFER_HEIGHT * 0.4f + ReadyPRectSizeY);
                D2D1_RECT_F ReadyPlayer3TextRec = D2D1::RectF(FRAME_BUFFER_RESIZE * 0.73f + 231.0f, FRAME_BUFFER_HEIGHT * 0.4f, FRAME_BUFFER_RESIZE * 0.73f + ReadyPRectSizeX + 231.0f, FRAME_BUFFER_HEIGHT * 0.4f + ReadyPRectSizeY);
                D2D1_RECT_F ReadyBossTextRec = D2D1::RectF(FRAME_BUFFER_RESIZE * 0.02f, FRAME_BUFFER_HEIGHT * 0.15f, FRAME_BUFFER_RESIZE * 0.15f, FRAME_BUFFER_HEIGHT * 0.26f);

                m_pd2dDeviceContext->BeginDraw();

                WCHAR wChat[256] = L"";
                WCHAR pstrReadyPlayerText[256] = L"";
                WCHAR pstrReadyBossText[256] = L"";
                WCHAR pstrWaitBossText[256] = L"";
                WCHAR pstrTime[256] = L"";

                wcscpy_s(pstrReadyPlayerText, 256, L"Ready\n");
                wcscpy_s(pstrReadyBossText, 256, L"Boss Ready\n");
                wcscpy_s(pstrWaitBossText, 256, L"Boss Waiting\n");

                wcscat(wChat, SceneManager::GetInstance()->m_ReadyInfo.Chat.ChatBuf);
                wcscat(wChat, SceneManager::GetInstance()->m_ReadyInfo.Chat.TempChatBuf);
                m_bChatting = SceneManager::GetInstance()->m_ReadyInfo.Chat.bOnChat;

                if (SceneManager::GetInstance()->m_ReadyInfo.Chat.bOnChat)
                {
                    if (m_bFlicker)
                    {
                        wcscat(wChat, L"I");
                    }
                }

                _itow_s(NetworkManager::GetInstance()->readySceneInfo->time, pstrTime, 10);

                if (m_bReadyTextShow[static_cast<int>(ORDER::PLAYER1)])
                    m_pd2dDeviceContext->DrawText(pstrReadyPlayerText, (UINT)wcslen(pstrReadyPlayerText), m_textFormats[TEXT_SIZE::SIZE_50], ReadyPlayer1TextRec, m_brushes[BRUSH_COLOR::DARK_CRIMSON]);
                if (m_bReadyTextShow[static_cast<int>(ORDER::PLAYER2)])
                    m_pd2dDeviceContext->DrawText(pstrReadyPlayerText, (UINT)wcslen(pstrReadyPlayerText), m_textFormats[TEXT_SIZE::SIZE_50], ReadyPlayer2TextRec, m_brushes[BRUSH_COLOR::DARK_CRIMSON]);
                if (m_bReadyTextShow[static_cast<int>(ORDER::PLAYER3)])
                    m_pd2dDeviceContext->DrawText(pstrReadyPlayerText, (UINT)wcslen(pstrReadyPlayerText), m_textFormats[TEXT_SIZE::SIZE_50], ReadyPlayer3TextRec, m_brushes[BRUSH_COLOR::DARK_CRIMSON]);
                if (m_bReadyTextShow[static_cast<int>(ORDER::BOSS)])
                    m_pd2dDeviceContext->DrawText(pstrReadyBossText, (UINT)wcslen(pstrReadyBossText), m_textFormats[TEXT_SIZE::SIZE_30], ReadyBossTextRec, m_brushes[BRUSH_COLOR::DARK_CRIMSON]);
                else
                    m_pd2dDeviceContext->DrawText(pstrWaitBossText, (UINT)wcslen(pstrWaitBossText), m_textFormats[TEXT_SIZE::SIZE_30], ReadyBossTextRec, m_brushes[BRUSH_COLOR::DARK_CRIMSON]);

                //chating
                m_pd2dDeviceContext->DrawText(wChat, (UINT)wcslen(wChat), m_textLeftFormats[TEXT_SIZE::SIZE_15], ChatTypingRec, m_brushes[BRUSH_COLOR::SKY_BLUE]);
                int count = 0;

                list<SERVERCHAT> ListChating = NetworkManager::GetInstance()->ListChating;
                for (auto iter = ListChating.rbegin(); iter != ListChating.rend(); ++iter)
                {
                    m_pd2dDeviceContext->DrawText(iter->chating, (UINT)wcslen(iter->chating), m_textLeftFormats[TEXT_SIZE::SIZE_15], ChatingRectArray[count], m_brushes[BRUSH_COLOR::SKY_BLUE]);
                    ++count;
                }

                //Time
                m_pd2dDeviceContext->DrawText(pstrTime, (UINT)wcslen(pstrTime), m_textFormats[TEXT_SIZE::SIZE_30], m_timeRect, m_brushes[BRUSH_COLOR::WHITE]);
                
                // Stats
                if (CTextureShader::GetInstance()->isRenderStats()) {
                    m_nAdditionalStats = NetworkManager::GetInstance()->myClient->GetAdditionalStats();
                    const int extraPoint = 1;
                    const int statTexts = extraPoint + 9; // points + stat value
                    for (int i = 0; i < 10; ++i)
                    {
                        int statValue = 0;

                        switch (i)
                        {
                        case 0: statValue = m_nAdditionalStats.point; break;
                        case 1: statValue = m_nAdditionalStats.hp; break;
                        case 2: statValue = m_nAdditionalStats.mp; break;
                        case 3: statValue = m_nAdditionalStats.attack; break;
                        case 4: statValue = m_nAdditionalStats.magic_attack; break;
                        case 5: statValue = m_nAdditionalStats.defense; break;
                        case 6: statValue = m_nAdditionalStats.magic_defense; break;
                        case 7: statValue = m_nAdditionalStats.speed; break;
                        case 8: statValue = m_nAdditionalStats.tenacity; break;
                        case 9: statValue = m_nAdditionalStats.critical; break;
                        }

                        TEXT_SIZE size = i < extraPoint ? TEXT_SIZE::SIZE_18 : TEXT_SIZE::SIZE_15;

                        _itow_s(statValue, m_statText, 10);
                        m_pd2dDeviceContext->DrawText(m_statText, (UINT)wcslen(m_statText),
                            m_textFormats[size], m_AdditionalStatsRect[i], m_brushes[BRUSH_COLOR::DARK_GRAY]);
                    }
                }
                m_pd2dDeviceContext->EndDraw();
            }
            else
            {
                //Game Ready
                D2D1_RECT_F Ready_ButtonTextRec = D2D1::RectF(FRAME_BUFFER_RESIZE * 0.02f + 231.0f, FRAME_BUFFER_HEIGHT * 0.075f, FRAME_BUFFER_RESIZE * 0.15f + 231.0f, FRAME_BUFFER_HEIGHT * 0.12f);
                D2D1_RECT_F ReadyPlayer1TextRec = D2D1::RectF(FRAME_BUFFER_RESIZE * 0.47f + 231.0f, FRAME_BUFFER_HEIGHT * 0.42f, FRAME_BUFFER_RESIZE * 0.63f + 231.0f, FRAME_BUFFER_HEIGHT * 0.48f);
                D2D1_RECT_F ReadyPlayer2TextRec = D2D1::RectF(FRAME_BUFFER_RESIZE * 0.6f + 231.0f, FRAME_BUFFER_HEIGHT * 0.42f, FRAME_BUFFER_RESIZE * 0.76f + 231.0f, FRAME_BUFFER_HEIGHT * 0.48f);
                D2D1_RECT_F ReadyPlayer3TextRec = D2D1::RectF(FRAME_BUFFER_RESIZE * 0.7f + 231.0f, FRAME_BUFFER_HEIGHT * 0.42f, FRAME_BUFFER_RESIZE * 0.86f + 231.0f, FRAME_BUFFER_HEIGHT * 0.48f);
                D2D1_RECT_F ReadyBossTextRec = D2D1::RectF(FRAME_BUFFER_RESIZE * 0.05f + 231.0f, FRAME_BUFFER_HEIGHT * 0.75f, FRAME_BUFFER_RESIZE * 0.25f + 231.0f, FRAME_BUFFER_HEIGHT * 0.85f);

                m_pd2dDeviceContext->BeginDraw();

                WCHAR wChat[256] = L"";
                WCHAR pstrReadyPlayerText[256] = L"";
                WCHAR pstrTime[256] = L"";

                wcscpy_s(pstrReadyPlayerText, 256, L"Ready\n");
                wcscat(wChat, SceneManager::GetInstance()->m_ReadyInfo.Chat.ChatBuf);
                wcscat(wChat, SceneManager::GetInstance()->m_ReadyInfo.Chat.TempChatBuf);
                _itow_s(NetworkManager::GetInstance()->readySceneInfo->time, pstrTime, 10);

                if (m_bReadyTextShow[static_cast<int>(ORDER::PLAYER1)])
                    m_pd2dDeviceContext->DrawText(pstrReadyPlayerText, (UINT)wcslen(pstrReadyPlayerText), m_textFormats[TEXT_SIZE::SIZE_25], ReadyPlayer1TextRec, m_brushes[BRUSH_COLOR::DARK_CRIMSON]);
                if (m_bReadyTextShow[static_cast<int>(ORDER::PLAYER2)])
                    m_pd2dDeviceContext->DrawText(pstrReadyPlayerText, (UINT)wcslen(pstrReadyPlayerText), m_textFormats[TEXT_SIZE::SIZE_25], ReadyPlayer2TextRec, m_brushes[BRUSH_COLOR::DARK_CRIMSON]);
                if (m_bReadyTextShow[static_cast<int>(ORDER::PLAYER3)])
                    m_pd2dDeviceContext->DrawText(pstrReadyPlayerText, (UINT)wcslen(pstrReadyPlayerText), m_textFormats[TEXT_SIZE::SIZE_25], ReadyPlayer3TextRec, m_brushes[BRUSH_COLOR::DARK_CRIMSON]);
                if (m_bReadyTextShow[static_cast<int>(ORDER::BOSS)])
                    m_pd2dDeviceContext->DrawText(pstrReadyPlayerText, (UINT)wcslen(pstrReadyPlayerText), m_textFormats[TEXT_SIZE::SIZE_60], ReadyBossTextRec, m_brushes[BRUSH_COLOR::DARK_CRIMSON]);

                //Time
                m_pd2dDeviceContext->DrawText(pstrTime, (UINT)wcslen(pstrTime), m_textFormats[TEXT_SIZE::SIZE_30], m_timeRect, m_brushes[BRUSH_COLOR::WHITE]);
                
                // Stats
                if (CTextureShader::GetInstance()->isRenderStats()) {
                    m_nAdditionalStats = NetworkManager::GetInstance()->myClient->GetAdditionalStats();
                    const int extraPoint = 1;
                    const int statTexts = extraPoint + 9; // points + stat value
                    for (int i = 0; i < 10; ++i)
                    {
                        int statValue = 0;

                        switch (i)
                        {
                        case 0: statValue = m_nAdditionalStats.point; break;
                        case 1: statValue = m_nAdditionalStats.hp; break;
                        case 2: statValue = m_nAdditionalStats.mp; break;
                        case 3: statValue = m_nAdditionalStats.attack; break;
                        case 4: statValue = m_nAdditionalStats.magic_attack; break;
                        case 5: statValue = m_nAdditionalStats.defense; break;
                        case 6: statValue = m_nAdditionalStats.magic_defense; break;
                        case 7: statValue = m_nAdditionalStats.speed; break;
                        case 8: statValue = m_nAdditionalStats.tenacity; break;
                        case 9: statValue = m_nAdditionalStats.critical; break;
                        }

                        TEXT_SIZE size = i < extraPoint ? TEXT_SIZE::SIZE_18 : TEXT_SIZE::SIZE_15;

                        _itow_s(statValue, m_statText, 10);
                        m_pd2dDeviceContext->DrawText(m_statText, (UINT)wcslen(m_statText),
                            m_textFormats[size], m_AdditionalStatsRect[i], m_brushes[BRUSH_COLOR::DARK_GRAY]);
                    }
                }
                m_pd2dDeviceContext->EndDraw();

            }
        }
        else
        {
            wcscpy_s(pLoadingExplanationText, 256, L"레디 씬 ");
            if (m_fLoadingPreProgressPercent <= 0.f)
                wcscat(pLoadingExplanationText, m_LoadingExplanationText[LOADING_TEXT::MAP]);
            else if (m_fLoadingPreProgressPercent < SceneManager::GetInstance()->ToReadyPercent[LOADING_TEXT::CHARACTER])
                wcscat(pLoadingExplanationText, m_LoadingExplanationText[LOADING_TEXT::CHARACTER]);
            else if (m_fLoadingPreProgressPercent < SceneManager::GetInstance()->ToReadyPercent[LOADING_TEXT::SHADER])
                wcscat(pLoadingExplanationText, m_LoadingExplanationText[LOADING_TEXT::SHADER]);
            else if (m_fLoadingPreProgressPercent >= SceneManager::GetInstance()->ToReadyPercent[LOADING_TEXT::SHADER])
                wcscat(pLoadingExplanationText, L"로드 완료");
        }
        break;
    }
    case SCENEKIND::INGAME:
    {               
        if (!LoadingRender)
        {
            m_pd2dDeviceContext->BeginDraw();

            WCHAR pstrMinuteTime[256] = L"";
            WCHAR pstrSecondTime[256] = L"";
            WCHAR pstrTime[256] = L"";
            WCHAR pstrSkillCoolTime[4][256] = {};

            int time = NetworkManager::GetInstance()->gameSceneInfo->time;
            int minute = time / 60;
            int second = time - minute * 60;


            _itow_s(minute, pstrMinuteTime, 10);
            _itow_s(second, pstrSecondTime, 10);

            if (minute == 0)
                wcscat(pstrTime, L"00");
            else
                wcscat(pstrTime, pstrMinuteTime);
            wcscat(pstrTime, L" : ");
            if (second < 10)
                wcscat(pstrTime, L"0");
            wcscat(pstrTime, pstrSecondTime);


            //Time
            m_pd2dDeviceContext->DrawText(pstrTime, (UINT)wcslen(pstrTime), m_textFormats[TEXT_SIZE::SIZE_30], m_timeRect, m_brushes[BRUSH_COLOR::WHITE]);

            bool isRenderShop = CTextureShader::GetInstance()->isRenderShop();

            WCHAR pstrGold[256] = L"";
            short gold = reinterpret_cast<CGamePlayer*>(NetworkManager::GetInstance()->myClient)->GetGold();

            wstring goldString = to_wstring(gold);
            goldString += L"G";
            wcscpy_s(pstrGold, goldString.c_str());

            m_GoldRect = isRenderShop ? D2D1::RectF(FRAME_BUFFER_RESIZE * 0.14907f + 231.0f, FRAME_BUFFER_HEIGHT * 0.71875f,
                FRAME_BUFFER_RESIZE * 0.20647f + 231.0f, FRAME_BUFFER_HEIGHT * 0.75125f) :
                D2D1::RectF(FRAME_BUFFER_RESIZE * 0.7806f + 231.0f, FRAME_BUFFER_HEIGHT * 0.95f,
                    FRAME_BUFFER_RESIZE * 0.83893f + 231.0f, FRAME_BUFFER_HEIGHT * 0.975f);

            int size = isRenderShop ? TEXT_SIZE::SIZE_18 : TEXT_SIZE::SIZE_15;
           
            for (int i = 0; i < MAX_SKILL; ++i) {
                wstring temp = i == 0 ? L"R Click" :
                    i == 1 ? L"Shift" :
                    i == 2 ? L"Q" :
                    L"R";
                wcscpy_s(pstrGold, temp.c_str());
                m_pd2dDeviceContext->DrawText(pstrGold, (UINT)wcslen(pstrGold), m_textLeftFormats[TEXT_SIZE::SIZE_15], m_SkilKeyRec[i],
                    m_brushes[BRUSH_COLOR::WHITE]);
                if (m_skillCoolRemainingTime[i + 1] > 0)
                {
                    _itow_s(m_skillCoolRemainingTime[i + 1], pstrSkillCoolTime[i], 10);
                    m_pd2dDeviceContext->DrawText(pstrSkillCoolTime[i], (UINT)wcslen(pstrSkillCoolTime[i]), m_textFormats[TEXT_SIZE::SIZE_25], m_SkilCoolTimeRec[i],
                        m_brushes[BRUSH_COLOR::RED]);
                }

            }

            goldString = to_wstring(gold);
            goldString += L"G";
            wcscpy_s(pstrGold, goldString.c_str());
            m_pd2dDeviceContext->DrawText(pstrGold, (UINT)wcslen(pstrGold), m_textRightFormats[size], m_GoldRect, m_brushes[BRUSH_COLOR::WHITE]);

            if (isRenderShop)
            {
                D2D1_RECT_F rect;
                for (int i = 0; i < 9; ++i)
                {
                    rect = D2D1::RectF(FRAME_BUFFER_RESIZE * 0.80926f + 231.0f, FRAME_BUFFER_HEIGHT * (0.24875f + i * 0.05875f),
                        FRAME_BUFFER_RESIZE * 0.86667f + 231.0f, FRAME_BUFFER_HEIGHT * (0.28125f + i * 0.05875f));
                    int price = CTextureShader::GetInstance()->GetPrice(i);
                    goldString = to_wstring(price);
                    goldString += L"G";
                    wcscpy_s(pstrGold, goldString.c_str());

                    m_pd2dDeviceContext->DrawText(pstrGold, (UINT)wcslen(pstrGold), m_textRightFormats[TEXT_SIZE::SIZE_25], rect,
                        m_brushes[BRUSH_COLOR::WHITE]);

                    rect = D2D1::RectF(FRAME_BUFFER_RESIZE * 0.7037f + 231.0f, FRAME_BUFFER_HEIGHT * (0.24875f + i * 0.05875f),
                        FRAME_BUFFER_RESIZE * 0.77407f + 231.0f, FRAME_BUFFER_HEIGHT * (0.28125f + i * 0.05875f));
                    
                    int level = CTextureShader::GetInstance()->GetStatLevel(i);
                    goldString = level < 10 ? L"LV.0" : L"LV.";
                    goldString += to_wstring(level);
                    wcscpy_s(pstrGold, goldString.c_str());

                    m_pd2dDeviceContext->DrawText(pstrGold, (UINT)wcslen(pstrGold), m_textRightFormats[TEXT_SIZE::SIZE_25], rect,
                        m_brushes[BRUSH_COLOR::WHITE]);
                }
            }

            m_pd2dDeviceContext->EndDraw();
        }
        else
        {
            wcscpy_s(pLoadingExplanationText, 256, L"인게임 씬 ");
            if (m_fLoadingPreProgressPercent <= 0.f)
                wcscat(pLoadingExplanationText, m_LoadingExplanationText[LOADING_TEXT::MAP]);
            else if (m_fLoadingPreProgressPercent < SceneManager::GetInstance()->ToIngamePercent[LOADING_TEXT::CHARACTER])
                wcscat(pLoadingExplanationText, m_LoadingExplanationText[LOADING_TEXT::CHARACTER]);
            else if (m_fLoadingPreProgressPercent < SceneManager::GetInstance()->ToIngamePercent[LOADING_TEXT::SHADER])
                wcscat(pLoadingExplanationText, m_LoadingExplanationText[LOADING_TEXT::SHADER]);
            else if (m_fLoadingPreProgressPercent >= SceneManager::GetInstance()->ToIngamePercent[LOADING_TEXT::SHADER])
                wcscat(pLoadingExplanationText, L"로드 완료");
        }

        break;
    }

    }

    if (LoadingRender) {
        if (!m_bProgressing)
        {
            m_fLoadingNowProgressPercent = SceneManager::GetInstance()->m_fLoadingProgressPercent;
            m_bProgressing = true;
            m_fLerpProgressing = 0.f;
        }
        else
        {
            m_fLerpProgress = lerp(m_fLoadingPreProgressPercent, m_fLoadingNowProgressPercent, m_fLerpProgressing);
            m_fProgress = ProgressBarWidth * m_fLerpProgress;

            m_fLerpProgressing += m_fElapseTime;
            if (m_fLerpProgressing > 1.f)
            {
                m_fLerpProgressing = 1.f;
                m_fLoadingPreProgressPercent = m_fLoadingNowProgressPercent;
                m_bProgressing = false;
            }

        }

        D2D1_RECT_F LoadingProgressBarRec = D2D1::RectF(FRAME_BUFFER_RESIZE * (ProgressBarStartWidth)+231.0f, FRAME_BUFFER_HEIGHT * ProgressBarStartHeight, FRAME_BUFFER_RESIZE * (ProgressBarStartWidth + m_fProgress) + 231.0f, FRAME_BUFFER_HEIGHT * (ProgressBarStartHeight + ProgressBarHeight));


        m_pd2dDeviceContext->BeginDraw();

        wcscpy_s(wLoadingChat, 256, L"Loading\n");

        //Percent
        int iPercent = static_cast<int>(m_fLerpProgress * 100);
        _itow_s(iPercent, pPercentText, 10);
        wcscat(pPercentText, L"%");

        m_fTotalTime += m_fElapseTime;
        if (m_fTotalTime > 4.f)
        {
            m_fTotalTime = 0.f;
            wcscpy_s(pDotText, 256, L"\n");
        }
        else if (m_fTotalTime > 3.f)
        {
            wcscpy_s(pDotText, 256, L". . .\n");
        }
        else if (m_fTotalTime > 2.f)
        {
            wcscpy_s(pDotText, 256, L". .\n");
        }
        else if (m_fTotalTime > 1.f)
        {
            wcscpy_s(pDotText, 256, L".\n");
        }
        else
        {
            wcscpy_s(pDotText, 256, L"\n");
        }

        //Progress Bar
        m_pd2dDeviceContext->FillRectangle(LoadingProgressBarRec, m_brushes[BRUSH_COLOR::DARK_CRIMSON]);
        m_pd2dDeviceContext->DrawRectangle(LoadingTotalProgressBarRec, m_brushes[BRUSH_COLOR::DARK_GRAY], 5.f, NULL);


        m_pd2dDeviceContext->DrawText(pLoadingExplanationText, (UINT)wcslen(pLoadingExplanationText), m_textLeftFormats[TEXT_SIZE::SIZE_15], LoadingExplanationTexRec, m_brushes[BRUSH_COLOR::WHITE]);
        m_pd2dDeviceContext->DrawText(pPercentText, (UINT)wcslen(pPercentText), m_textLeftFormats[TEXT_SIZE::SIZE_15], LoadingProgressPercentTextRec, m_brushes[BRUSH_COLOR::WHITE]);
        m_pd2dDeviceContext->DrawText(m_vecGameExplanationText[m_iNumText], (UINT)wcslen(m_vecGameExplanationText[m_iNumText]), m_textFormats[TEXT_SIZE::SIZE_18], GameExPlanationTextRec, m_brushes[BRUSH_COLOR::WHITE]);

        m_pd2dDeviceContext->DrawText(wLoadingChat, (UINT)wcslen(wLoadingChat), m_textFormats[TEXT_SIZE::SIZE_50], LoadingTextRec, m_brushes[BRUSH_COLOR::WHITE]);

        m_pd2dDeviceContext->DrawText(pDotText, (UINT)wcslen(pDotText), m_textLeftFormats[TEXT_SIZE::SIZE_50], LoadingDotTextRec, m_brushes[BRUSH_COLOR::WHITE]);
        m_pd2dDeviceContext->EndDraw();
    }

    m_pd3d11On12Device->ReleaseWrappedResources(ppResources, _countof(ppResources));
    m_pd3d11DeviceContext->Flush();

}

void UILayer::ReleaseResources()
{
    for (UINT i = 0; i < m_nTextBlocks; i++)
    {
        m_pTextBlocks[i].m_pdwFormat->Release();
        m_pTextBlocks[i].m_pd2dTextBrush->Release();
    }
    delete[] m_pTextBlocks;
    m_pTextBlocks = nullptr;

    // Render가 Acquire/Release를 쌍으로 완료한다. 종료 시 이미 PRESENT인
    // 여러 swapchain buffer에 다시 transition을 기록하지 않는다.

    m_pd2dDeviceContext->SetTarget(nullptr);
    m_pd3d11DeviceContext->Flush();

    for (UINT i = 0; i < m_nRenderTargets; i++)
    {
        m_ppd2dRenderTargets[i]->Release();
        m_ppd3d11WrappedRenderTargets[i]->Release();
    }

    for (int i = 0; i < m_brushes.size(); ++i) {
        if (m_brushes[i]) {
            m_brushes[i]->Release();
        }
    }

    for (int i = 0; i < m_textFormats.size(); ++i) {
        if (m_textFormats[i]) {
            m_textFormats[i]->Release();
        }
    }

    for (int i = 0; i < m_textLeftFormats.size(); ++i) {
        if (m_textLeftFormats[i]) {
            m_textLeftFormats[i]->Release();
        }
    }

    for (int i = 0; i < m_textRightFormats.size(); ++i) {
        if (m_textRightFormats[i]) {
            m_textRightFormats[i]->Release();
        }
    }

    for (auto& name : m_shopPartsNames)
    {
        delete[] name;
    }

    m_pd2dDeviceContext->Release();
    m_pd2dWriteFactory->Release();
    m_pd2dDevice->Release();
    m_pd2dFactory->Release();
    m_pd3d11DeviceContext->Release();
    m_pd3d11On12Device->Release();
}

bool UIRect::ClickCollide(POINT clickPos)
{
    if (m_rect.left <= clickPos.x && m_rect.right >= clickPos.x && m_rect.top <= clickPos.y && m_rect.bottom >= clickPos.y) {
        return m_function();
    }
    return false;
}
