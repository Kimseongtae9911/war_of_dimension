#pragma once

struct TextBlock
{
    WCHAR                           m_pstrText[256];
    D2D1_RECT_F                     m_d2dLayoutRect;
    IDWriteTextFormat* m_pdwFormat;
    ID2D1SolidColorBrush* m_pd2dTextBrush;
};

class UIRect
{
public:
    UIRect() {}
    UIRect(D2D1_RECT_F r, function<bool()> f) { m_rect = r; m_function = f; }
    ~UIRect() {}

    bool ClickCollide(POINT clickPos);

private:
    D2D1_RECT_F m_rect;
    function<bool()> m_function;
};

class UILayer
{
public:
    UILayer(UINT nFrames, UINT nTextBlocks, ID3D12Device* pd3dDevice, ID3D12CommandQueue* pd3dCommandQueue, ID3D12Resource** ppd3dRenderTargets, UINT nWidth, UINT nHeight);
    UILayer();
    ~UILayer();

    static UILayer* Create(UINT nFrames, UINT nTextBlocks, ID3D12Device* pd3dDevice, ID3D12CommandQueue* pd3dCommandQueue, ID3D12Resource** ppd3dRenderTargets, UINT nWidth, UINT nHeight);
    HRESULT Initialize(UINT nFrames, UINT nTextBlocks, ID3D12Device* pd3dDevice, ID3D12CommandQueue* pd3dCommandQueue, ID3D12Resource** ppd3dRenderTargets, UINT nWidth, UINT nHeight);
    void Reset();

    static UILayer* GetInstance() { return s_instance; }

    void UpdateTextOutputs(UINT nIndex, const WCHAR* pstrUIText, D2D1_RECT_F* pd2dLayoutRect, IDWriteTextFormat* pdwFormat, ID2D1SolidColorBrush* pd2dTextBrush);
    void Render(UINT nFrame, bool LoadingRender = false);
    void ReleaseResources();

    ID2D1SolidColorBrush* CreateBrush(D2D1::ColorF d2dColor);
    IDWriteTextFormat* CreateTextFormat(const WCHAR* pszFontName, float fFontSize);
    IDWriteTextFormat* CreateTextFormatLeft(const WCHAR* pszFontName, float fFontSize);
    IDWriteTextFormat* CreateTextFormatRight(const WCHAR* pszFontName, float fFontSize);

    void SetPlayerMaxHp(float nMaxHp) { m_fPlayerMaxHp = nMaxHp; }
    void SetPlayerCurHp(float nCurHp) { m_fPlayerCurHp = nCurHp; }
    void SetPlayerHp(float nMaxHp, float nCurHp) { m_fPlayerMaxHp = nMaxHp; m_fPlayerCurHp = nCurHp; }

    //Ready
    D2D1_RECT_F GetSkillClickRect() { return m_SkillClickFirstRec; }
    D2D1_RECT_F* GetPlayerSkillRect(ORDER order) { return m_PlayerSKillRects[static_cast<int>(order)]; }

    void SetChatTypingText(WCHAR* pText) {wcscpy_s(m_pChatTypingText, 256, pText);   }
    void SetChatTypingTempText(WCHAR* pText) { wcscpy_s(m_pChatTypingTempText, 2, pText); }

    void SetChatOption(CHAT enumChat) { eChatOption = enumChat; }
    CHAT GetChatOption() { return eChatOption; }

    void SetRole(char role) { m_role = role; }
    char GetRole() const { return m_role; }

    void SetElapseTime(float time) { m_fElapseTime = time; }

    void CheckTime(float time) { if (m_fRemainingTime > 0) m_fRemainingTime -= time; else m_fRemainingTime = 0; }

    void ProcessMouseClick(SCENEKIND sceneKind, POINT clickPos);

    float GetLoadingPreProgressPercent() { return m_fLoadingPreProgressPercent; }
    float GetLerpProgressing() { return m_fLerpProgressing; }
    void  ResetLoadingValue();

    void GenRandomMent();

public:
    void InitializeDevice(ID3D12Device* pd3dDevice, ID3D12CommandQueue* pd3dCommandQueue, ID3D12Resource** ppd3dRenderTargets);

    void PushChating(WCHAR* chat);

    float                           m_fWidth = 0.0f;
    float                           m_fHeight = 0.0f;

    ID3D11DeviceContext* m_pd3d11DeviceContext = NULL;
    ID3D11On12Device* m_pd3d11On12Device = NULL;
    IDWriteFactory* m_pd2dWriteFactory = NULL;
    ID2D1Factory3* m_pd2dFactory = NULL;
    ID2D1Device2* m_pd2dDevice = NULL;
    ID2D1DeviceContext2* m_pd2dDeviceContext = NULL;


    UINT                            m_nRenderTargets = 0;
    ID3D11Resource** m_ppd3d11WrappedRenderTargets = NULL;
    ID2D1Bitmap1** m_ppd2dRenderTargets = NULL;

    UINT                            m_nTextBlocks = 0;
    TextBlock* m_pTextBlocks = NULL;

    float                             m_fPlayerMaxHp = 0;
    float                             m_fPlayerCurHp = 0;
    float                             m_fPlayerMaxMp = 0;
    float                             m_fPlayerCurMp = 0;

    bool m_bChatting = false;
    bool m_bFlicker = false;

    //Lobby
    bool m_bMatchingClick = false;
    bool m_bHeroClick = false;
    bool m_bBossClick = false;

    //Ready 
    bool m_bReadyButtonClick = false;
    bool m_bSkillPopUpClick = false;
    bool m_bStackPopUpClick = false;
    bool m_bReadyTextShow[4];
    bool m_bReadyPlayerSkillRects[4][4];
    array<int, 5> m_skillCoolRemainingTime = {};
    

private:
    array<vector<UIRect>, 3> m_uiRects;
    //Lobby
    D2D1_RECT_F m_ChatingRec = D2D1::RectF(FRAME_BUFFER_WIDTH * 0.02f, FRAME_BUFFER_HEIGHT * 0.62f, FRAME_BUFFER_WIDTH * 0.4f, FRAME_BUFFER_HEIGHT * 0.9f);
    D2D1_RECT_F m_ChatOptionRec = D2D1::RectF(FRAME_BUFFER_WIDTH * 0.02f, FRAME_BUFFER_HEIGHT * 0.91f, FRAME_BUFFER_WIDTH * 0.09f, FRAME_BUFFER_HEIGHT * 0.95f);
    D2D1_RECT_F m_ChatOptionTextRec = D2D1::RectF(FRAME_BUFFER_WIDTH * 0.02f, FRAME_BUFFER_HEIGHT * 0.9175f, FRAME_BUFFER_WIDTH * 0.09f, FRAME_BUFFER_HEIGHT * 0.95f);
    D2D1_RECT_F m_ChatTypingRec = D2D1::RectF(FRAME_BUFFER_WIDTH * 0.08075f, FRAME_BUFFER_HEIGHT * 0.91f, FRAME_BUFFER_WIDTH * 0.4f, FRAME_BUFFER_HEIGHT * 0.95f);

    WCHAR m_pChatTypingText[256] = L"";
    WCHAR m_pChatTypingTempText[2] = L"";

    CHAT eChatOption = CHAT::CHANNEL;
    char m_role = 0;

    D2D1_RECT_F m_exposureRec = D2D1::RectF(FRAME_BUFFER_RESIZE * 0.558333f + 231.0f, FRAME_BUFFER_HEIGHT * 0.4425f,
        FRAME_BUFFER_RESIZE * 0.610185f + 231.0f, FRAME_BUFFER_HEIGHT * 0.47125f);
    D2D1_RECT_F m_saturationRec = D2D1::RectF(FRAME_BUFFER_RESIZE * 0.558333f + 231.0f, FRAME_BUFFER_HEIGHT * (0.4425f + (1.f * 0.03625f)),
        FRAME_BUFFER_RESIZE * 0.610185f + 231.0f, FRAME_BUFFER_HEIGHT * (0.47125f + (1.f * 0.03625f)));
    D2D1_RECT_F m_contrastRec = D2D1::RectF(FRAME_BUFFER_RESIZE * 0.558333f + 231.0f, FRAME_BUFFER_HEIGHT * (0.4425f + (2.f * 0.03625f)),
        FRAME_BUFFER_RESIZE * 0.610185f + 231.0f, FRAME_BUFFER_HEIGHT * (0.47125f + (2.f * 0.03625f)));
    D2D1_RECT_F m_vibranceRec = D2D1::RectF(FRAME_BUFFER_RESIZE * 0.558333f + 231.0f, FRAME_BUFFER_HEIGHT * (0.4425f + (3.f * 0.03625f)),
        FRAME_BUFFER_RESIZE * 0.610185f + 231.0f, FRAME_BUFFER_HEIGHT * (0.47125f + (3.f * 0.03625f)));
    D2D1_RECT_F m_volumeRec = D2D1::RectF(FRAME_BUFFER_RESIZE * 0.461111f + 231.0f, FRAME_BUFFER_HEIGHT * 0.5975f,
        FRAME_BUFFER_RESIZE * 0.564111f + 231.0f, FRAME_BUFFER_HEIGHT * 0.64125f);
    D2D1_RECT_F m_tokenRec = D2D1::RectF(FRAME_BUFFER_WIDTH * 0.84537f, FRAME_BUFFER_HEIGHT * 0.005f,
        FRAME_BUFFER_WIDTH * 0.84537f + FRAME_BUFFER_RESIZE * 0.14722f, FRAME_BUFFER_HEIGHT * 0.05875f);

    //for shop
    D2D1_RECT_F m_shopRec = D2D1::RectF(FRAME_BUFFER_RESIZE * 0.377778f + 231.0f, FRAME_BUFFER_HEIGHT * 0.7675f,
        FRAME_BUFFER_RESIZE * 0.772222f + 231.0f, FRAME_BUFFER_HEIGHT * 0.86f);
    vector<WCHAR*> m_vecShopOwnerMent;
    int m_nMent = 0;

    D2D1_RECT_F m_partsRec = D2D1::RectF(FRAME_BUFFER_RESIZE * 0.414815f + 231.0f, FRAME_BUFFER_HEIGHT * 0.27f,
        FRAME_BUFFER_RESIZE * 0.584255f + 231.0f, FRAME_BUFFER_HEIGHT * 0.315f);

    //for auction
    D2D1_RECT_F m_auctionGoldRec = D2D1::RectF(FRAME_BUFFER_RESIZE * 0.4074f + 231.0f, FRAME_BUFFER_HEIGHT * 0.225f,
        FRAME_BUFFER_RESIZE * 0.4898f + 231.0f, FRAME_BUFFER_HEIGHT * 0.25375f);
    D2D1_RECT_F m_myPriceRec = D2D1::RectF(FRAME_BUFFER_RESIZE * 0.680556f + 231.0f, FRAME_BUFFER_HEIGHT * 0.4325f,
        FRAME_BUFFER_RESIZE * 0.825926f + 231.0f, FRAME_BUFFER_HEIGHT * 0.48375f);
    D2D1_RECT_F m_priceRec[7];
    D2D1_RECT_F m_deadLineRec[7];
    D2D1_RECT_F m_userName[7];
    D2D1_RECT_F m_pageRec = D2D1::RectF(FRAME_BUFFER_RESIZE * 0.23426f + 231.0f, FRAME_BUFFER_HEIGHT * 0.81f,
        FRAME_BUFFER_RESIZE * 0.37126f + 231.0f, FRAME_BUFFER_HEIGHT * 0.8725f);
    D2D1_RECT_F m_auctionPartsRec = D2D1::RectF(FRAME_BUFFER_RESIZE * 0.55556f + 231.0f, FRAME_BUFFER_HEIGHT * 0.26875f,
        FRAME_BUFFER_RESIZE * 0.65648f + 231.0f, FRAME_BUFFER_HEIGHT * 0.29625f);
    D2D1_RECT_F m_auctionNumRec = D2D1::RectF(FRAME_BUFFER_RESIZE * 0.55556f + 231.0f, FRAME_BUFFER_HEIGHT * 0.36125f,
        FRAME_BUFFER_RESIZE * 0.65648f + 231.0f, FRAME_BUFFER_HEIGHT * 0.38875f);

    //for BlockChain
    D2D1_RECT_F m_blockChainMyGoldRec = D2D1::RectF(FRAME_BUFFER_RESIZE * 0.2926f + 231.0f, FRAME_BUFFER_HEIGHT * 0.225f,
        FRAME_BUFFER_RESIZE * 0.49075f + 231.0f, FRAME_BUFFER_HEIGHT * 0.285f);
    D2D1_RECT_F m_periodRec = D2D1::RectF(FRAME_BUFFER_RESIZE * 0.51111f + 231.0f, FRAME_BUFFER_HEIGHT * 0.36875f,
        FRAME_BUFFER_RESIZE * 0.70926f + 231.0f, FRAME_BUFFER_HEIGHT * 0.42875f);
    D2D1_RECT_F m_inputTokenRec = D2D1::RectF(FRAME_BUFFER_RESIZE * 0.51111f + 231.0f, FRAME_BUFFER_HEIGHT * 0.225f,
        FRAME_BUFFER_RESIZE * 0.70926f + 231.0f, FRAME_BUFFER_HEIGHT * 0.285f);
    D2D1_RECT_F m_stakingTokenRec = D2D1::RectF(FRAME_BUFFER_RESIZE * 0.2926f + 231.0f, FRAME_BUFFER_HEIGHT * 0.4375f,
        FRAME_BUFFER_RESIZE * 0.49075f + 231.0f, FRAME_BUFFER_HEIGHT * 0.4975f);
    D2D1_RECT_F m_unstakingPeriodRec = D2D1::RectF(FRAME_BUFFER_RESIZE * 0.2926f + 231.0f, FRAME_BUFFER_HEIGHT * 0.65125f,
        FRAME_BUFFER_RESIZE * 0.49075f + 231.0f, FRAME_BUFFER_HEIGHT * 0.71125f);

    //for Customize
    D2D1_RECT_F m_customizePartsRec = D2D1::RectF(FRAME_BUFFER_RESIZE * 0.6287f + 231.0f, FRAME_BUFFER_HEIGHT * 0.1575f,
        FRAME_BUFFER_RESIZE * 0.8361f + 231.0f, FRAME_BUFFER_HEIGHT * 0.21625f);
    D2D1_RECT_F m_customizeNumRec = D2D1::RectF(FRAME_BUFFER_RESIZE * 0.6287f + 231.0f, FRAME_BUFFER_HEIGHT * 0.34125f,
        FRAME_BUFFER_RESIZE * 0.8361f + 231.0f, FRAME_BUFFER_HEIGHT * 0.49875f);

    vector<WCHAR*> m_shopPartsNames;
    //Ready
    std::list<WCHAR*> m_pListReadyChating;

    D2D1_RECT_F m_SkillClickFirstRec = D2D1::RectF(FRAME_BUFFER_RESIZE * 0.31f + 231.0f, FRAME_BUFFER_HEIGHT * 0.60f, FRAME_BUFFER_RESIZE * 0.37f + 231.0f, FRAME_BUFFER_HEIGHT * 0.68f);

    D2D1_RECT_F m_PlayerSKillRects[4][4];

    float m_fRemainingTime = 60.f;

    Additional_Stats m_nAdditionalStats;
    D2D1_RECT_F m_AdditionalStatsRect[10];
    WCHAR m_statText[256] = L"";

    //Loading
    float m_fElapseTime = 0.f;
    float m_fTotalTime = 0.f;
    vector<WCHAR*> m_vecGameExplanationText;
    int   m_iNumText = 0;
    float m_fLoadingPreProgressPercent = 0.f;
    float m_fLoadingNowProgressPercent = 0.f;
    float m_fLerpProgressing = 0.f;
    bool  m_bProgressing = false;
    float m_fProgress = 0.f;
    float m_fLerpProgress = 0.f;

 private:
     static UILayer* s_instance;
     array<ID2D1SolidColorBrush*, BRUSH_COLOR::BRUSH_COUNT> m_brushes;
     array<IDWriteTextFormat*, TEXT_SIZE::TEXT_COUNT> m_textFormats;
     array<IDWriteTextFormat*, TEXT_SIZE::TEXT_COUNT> m_textLeftFormats;
     array<IDWriteTextFormat*, TEXT_SIZE::TEXT_COUNT> m_textRightFormats;
     array<WCHAR*, LOADING_TEXT::LOADING_TEXT_COUNT> m_LoadingExplanationText;

     //Title
     D2D1_RECT_F m_titleIDRect = D2D1::RectF(FRAME_BUFFER_RESIZE * 0.24f+ 231.0f, FRAME_BUFFER_HEIGHT * 0.45f, FRAME_BUFFER_RESIZE * 0.78f + 231.0f, FRAME_BUFFER_HEIGHT * 0.52f);
     D2D1_RECT_F m_titlePWRect = D2D1::RectF(FRAME_BUFFER_RESIZE * 0.24f+ 231.0f, FRAME_BUFFER_HEIGHT * 0.635f, FRAME_BUFFER_RESIZE * 0.78f + 231.0f, FRAME_BUFFER_HEIGHT * 0.7125f);

     //Ready, Game
     D2D1_RECT_F m_timeRect = D2D1::RectF(FRAME_BUFFER_RESIZE * 0.4f + 231.0f, 0.f, FRAME_BUFFER_RESIZE * 0.6f + 231.0f, FRAME_BUFFER_HEIGHT * 0.1f);

     //Ingame
     D2D1_RECT_F m_GoldRect = D2D1::RectF(FRAME_BUFFER_RESIZE * 0.7806f + 231.0f, FRAME_BUFFER_HEIGHT * 0.95f, FRAME_BUFFER_RESIZE * 0.83893f + 231.0f, FRAME_BUFFER_HEIGHT * 0.975f);
     D2D1_RECT_F m_SkilKeyRec[MAX_SKILL];
     D2D1_RECT_F m_SkilCoolTimeRec[MAX_SKILL];



     //Loading
     float LoadingTextWidth = 0.2f;
     float LoadingTextHeight = 0.15f;
     D2D1_RECT_F LoadingTextRec = D2D1::RectF(FRAME_BUFFER_RESIZE * 0.65f + 231.0f, FRAME_BUFFER_HEIGHT * 0.85f, FRAME_BUFFER_RESIZE * (0.65f + LoadingTextWidth) + 231.0f, FRAME_BUFFER_HEIGHT * (0.85f + LoadingTextHeight));
     D2D1_RECT_F LoadingDotTextRec = D2D1::RectF(FRAME_BUFFER_RESIZE * (0.65f + LoadingTextWidth) + 231.0f, FRAME_BUFFER_HEIGHT * 0.85f, FRAME_BUFFER_RESIZE * (0.65f + LoadingTextWidth + 0.1f) + 231.0f, FRAME_BUFFER_HEIGHT * (0.85f + LoadingTextHeight));

     float ExplanationStartWidth = 0.25f;
     float ExplanationStartHeight = 0.2f;
     D2D1_RECT_F GameExPlanationTextRec = D2D1::RectF(FRAME_BUFFER_RESIZE * (ExplanationStartWidth)+231.0f, FRAME_BUFFER_HEIGHT * ExplanationStartHeight, FRAME_BUFFER_RESIZE * (1.f - ExplanationStartWidth) + 231.0f, FRAME_BUFFER_HEIGHT * (ExplanationStartHeight + 0.1f));

     float ProgressBarWidth = 0.5f;
     float ProgressBarStartWidth = 0.25f;
     float ProgressBarHeight = 0.05f;
     float ProgressBarStartHeight = 0.6f;
     D2D1_RECT_F LoadingTotalProgressBarRec = D2D1::RectF(FRAME_BUFFER_RESIZE * (ProgressBarStartWidth)+231.0f, FRAME_BUFFER_HEIGHT * ProgressBarStartHeight, FRAME_BUFFER_RESIZE * (ProgressBarStartWidth + ProgressBarWidth) + 231.0f, FRAME_BUFFER_HEIGHT * (ProgressBarStartHeight + ProgressBarHeight));
    
     D2D1_RECT_F LoadingProgressPercentTextRec = D2D1::RectF(FRAME_BUFFER_RESIZE * (ProgressBarStartWidth + ProgressBarWidth + 0.01f) + 231.0f, FRAME_BUFFER_HEIGHT * ProgressBarStartHeight, FRAME_BUFFER_RESIZE * (ProgressBarStartWidth + ProgressBarWidth + 0.01f + 0.05f) + 231.0f, FRAME_BUFFER_HEIGHT * (ProgressBarStartHeight + ProgressBarHeight));

     D2D1_RECT_F LoadingExplanationTexRec = D2D1::RectF(FRAME_BUFFER_RESIZE * (ProgressBarStartWidth)+231.0f, FRAME_BUFFER_HEIGHT * (ProgressBarStartHeight + ProgressBarHeight + 0.01f), FRAME_BUFFER_RESIZE * (ProgressBarStartWidth + ProgressBarWidth) + 231.0f, FRAME_BUFFER_HEIGHT * (ProgressBarStartHeight + ProgressBarHeight + 0.01f + 0.05f));

};