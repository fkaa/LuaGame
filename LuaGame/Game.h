#pragma once

#include "pch.h"

#include "StepTimer.h"
#include "SpriteSheet.h"
#include "SpriteFont.h"
#include "MapFormat.h"
#include "Keyboard.h"
#include "Mouse.h"

#include <SpriteBatch.h>

using namespace DirectX;

class Game
{
public:

	Game();
	~Game();

	// Initialization and management
	void Initialize(HWND window, int width, int height);

	// Basic game loop
	void Tick();

	// Messages
	void OnActivated();
	void OnDeactivated();
	void OnSuspending();
	void OnResuming();
	void OnWindowSizeChanged(int width, int height);

	// Properties
	void GetDefaultSize(int& width, int& height) const;
	Keyboard::KeyboardStateTracker tracker;
	std::unique_ptr<Keyboard> keyboard;

	std::unique_ptr<Mouse> mouse;

	std::unique_ptr<SpriteBatch>                    m_spriteBatch;
	std::unique_ptr<SpriteSheet>                    m_spriteSheet;
	std::unique_ptr<SpriteFont>                     m_spriteFont;
private:

	void Update(DX::StepTimer const& timer);
	void Render();

	void Clear();
	void Present();

	void CreateDevice();
	void CreateResources();

	void OnDeviceLost();

	// Device resources.
	HWND                                            m_window;
	int                                             m_outputWidth;
	int                                             m_outputHeight;

	D3D_FEATURE_LEVEL                               m_featureLevel;
	Microsoft::WRL::ComPtr<ID3D11Device>            m_d3dDevice;
	Microsoft::WRL::ComPtr<ID3D11Device1>           m_d3dDevice1;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext>     m_d3dContext;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext1>    m_d3dContext1;

	Microsoft::WRL::ComPtr<IDXGISwapChain>          m_swapChain;
	Microsoft::WRL::ComPtr<IDXGISwapChain1>         m_swapChain1;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView>  m_renderTargetView;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView>  m_depthStencilView;

	// Rendering loop timer.
	DX::StepTimer                                   m_timer;
	lua_State *L;
};