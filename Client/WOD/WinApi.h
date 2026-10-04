#if defined(DEBUG) || defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "stdafx.h"
//#include "d3dUtil.h"
#include "FrameManager.h"
#include "GameTimer.h"

#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")

class CFrameManager;

class CWinApi
{
protected:

	CWinApi(HINSTANCE hInstance);
	CWinApi(const CWinApi& rhs) = delete;
	CWinApi& operator=(const CWinApi& rhs) = delete;
	virtual ~CWinApi();

public:
	static CWinApi* GetApp();

	HINSTANCE AppInst()const;		// 응용 프로그램 인스턴스 핸들의 복사본 리턴
	HWND      MainWnd()const;		// 메인 윈도우 창 핸들의 복사본 리턴
	float     AspectRatio()const;	// 후면 버퍼의 종횡비 리턴

	bool Get4xMsaaState()const;		// 4X MSAA가 활성화 되어있는지 여부 리턴
	void Set4xMsaaState(bool value);// 4X MSAA를 활성화/비활성화

	int Run();						// 응용 프로그램 메시지 루프

	virtual bool Initialize();
	virtual LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

protected:
	virtual void CreateRtvAndDsvDescriptorHeaps();
	virtual void OnResize();
	virtual void Update(const CGameTimer& gt) = 0;
	virtual void Draw(const CGameTimer& gt) = 0;

	// 마우스
	virtual void OnMouseDown(WPARAM btnState, int x, int y) { }
	virtual void OnMouseUp(WPARAM btnState, int x, int y) { }
	virtual void OnMouseMove(WPARAM btnState, int x, int y) { }

protected:

	bool InitMainWindow();
	bool InitDirect3D();
	void CreateCommandObjects();
	void CreateSwapChain();

	void FlushCommandQueue();

	ID3D12Resource* CurrentBackBuffer()const;
	D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView()const;
	D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView()const;

	void CalculateFrameStats();

	void LogAdapters();
	void LogAdapterOutputs(IDXGIAdapter* adapter);
	void LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format);

protected:

	static CWinApi* mApp;

	HINSTANCE mhAppInst = nullptr; // 인스턴스 핸들
	HWND      mhMainWnd = nullptr; // 메인 윈도우 핸들
	bool      mAppPaused = false;  // 프로그램 일시정지 상태
	bool      mMinimized = false;  // 최소화 버튼을 눌러서 최소화 된 상태
	bool      mMaximized = false;  // 최대화 버튼 눌러서 최대화 된 상태
	bool      mResizing = false;   // 창을 사용자가 크기 변경을 하고 있는지
	bool      mFullscreenState = false;// 전체화면 활성화

	// 4x MSAA를 킬지 말지
	bool      m4xMsaaState = false;    // 4X MSAA 활성화
	UINT      m4xMsaaQuality = 0;      // 4X MSAA 품질 수준

	// 타이머
	CGameTimer mTimer;
	CFrameManager* m_pFrameMgr = nullptr;

	Microsoft::WRL::ComPtr<IDXGIFactory4> mdxgiFactory;
	Microsoft::WRL::ComPtr<IDXGISwapChain> mSwapChain;
	Microsoft::WRL::ComPtr<ID3D12Device> md3dDevice;

	Microsoft::WRL::ComPtr<ID3D12Fence> mFence;
	UINT64 mCurrentFence = 0;

	Microsoft::WRL::ComPtr<ID3D12CommandQueue> mCommandQueue;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> mDirectCmdListAlloc;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> mCommandList;

	static const int SwapChainBufferCount = 2;
	int mCurrBackBuffer = 0;
	Microsoft::WRL::ComPtr<ID3D12Resource> mSwapChainBuffer[SwapChainBufferCount];
	Microsoft::WRL::ComPtr<ID3D12Resource> mDepthStencilBuffer;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mRtvHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mDsvHeap;

	D3D12_VIEWPORT mScreenViewport;
	D3D12_RECT mScissorRect;

	UINT mRtvDescriptorSize = 0;
	UINT mDsvDescriptorSize = 0;
	UINT mCbvSrvUavDescriptorSize = 0;

	// 해당 클래스의 자식 클래스에서 초기화 시 설정해야 하는 멤버 변수
	std::wstring mMainWndCaption = L"War Of Dimmension";
	D3D_DRIVER_TYPE md3dDriverType = D3D_DRIVER_TYPE_HARDWARE;
	DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	int mClientWidth = 800;
	int mClientHeight = 600;
};

