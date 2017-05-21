#include "pch.h"

#include "Game.h"
#include "Helpers.h"
#include "WICTextureLoader.h"
#include <SpriteBatch.h>
#include "CommonStates.h"

#include <algorithm>

#pragma warning (disable : 4996)

extern void ExitGame();

using namespace DirectX;
using Microsoft::WRL::ComPtr;

XMVECTOR normalize_color(int32_t color)
{
	return {
		((color & 0xff000000) >> 24) / 255.f,
		((color & 0xff0000) >> 16) / 255.f,
		((color & 0xff00) >> 8) / 255.f,
		(color & 0xff) / 255.f
	};
}

static Game *L_GetGame(lua_State *L);

static int L_LoadMaps(lua_State *L);
static int L_SaveMap(lua_State *L);

static int L_LoadSprite(lua_State *L);
static int L_DrawSprite(lua_State *L);
static int L_DrawText(lua_State *L);

static int L_Print(lua_State *L);

Game::Game() :
	m_window(0),
	m_outputWidth(1280),
	m_outputHeight(720),
	m_featureLevel(D3D_FEATURE_LEVEL_9_1)
{
}

Game::~Game()
{
	if (L) lua_close(L);
}

// Initialize the Direct3D resources required to run.
void Game::Initialize(HWND window, int width, int height)
{
	m_window = window;
	m_outputWidth = std::max(width, 1);
	m_outputHeight = std::max(height, 1);

	CreateDevice();
	CreateResources();

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> texture;
	DX::ThrowIfFailed(CreateWICTextureFromFile(m_d3dDevice.Get(), L"Assets/Sprites.png",
		nullptr, texture.GetAddressOf()));

	m_spriteBatch = std::make_unique<SpriteBatch>(m_d3dContext.Get());
	D3D11_VIEWPORT vp;
	ZeroMemory(&vp, sizeof(vp));
	vp.Width = width;
	vp.Height = height;
	m_spriteBatch->SetViewport(vp);
	auto bb = m_renderTargetView.Get();
	m_d3dContext->OMSetRenderTargets(1, &bb, nullptr);
	keyboard = std::make_unique<Keyboard>();
	mouse = std::make_unique<Mouse>();

	m_spriteSheet = std::make_unique<SpriteSheet>();
	m_spriteSheet->Load(texture.Get(), L"Assets/Sprites.txt");

	m_spriteFont = std::make_unique<SpriteFont>(m_d3dDevice.Get(), L"Assets\\font16.bin");

	L = luaL_newstate();
	luaL_openlibs(L);

	// för att komma åt vår `Game` instance genom bara funktioner
	lua_pushlightuserdata(L, this);
	lua_setglobal(L, "_GAME");

	lua_pushnumber(L, width);
	lua_setglobal(L, "width");
	lua_pushnumber(L, height);
	lua_setglobal(L, "height");

	lua_pushcfunction(L, L_LoadSprite);
	lua_setglobal(L, "load_sprite");
	lua_pushcfunction(L, L_DrawSprite);
	lua_setglobal(L, "draw_sprite");
	lua_pushcfunction(L, L_DrawText);
	lua_setglobal(L, "draw_text");
	lua_pushcfunction(L, L_LoadMaps);
	lua_setglobal(L, "load_maps");
	lua_pushcfunction(L, L_SaveMap);
	lua_setglobal(L, "save_map");
	lua_pushcfunction(L, L_Print);
	lua_setglobal(L, "print");

	TRACE("Load script")

	luaL_loadfile(L, "Scripts/main.lua");
	if (lua_pcall(L, 0, 0, 0) != 0)
		throw LuaException(L);

	TRACE("Init")

	lua_getglobal(L, "init");
	if (lua_pcall(L, 0, 0, 0) != 0)
		throw LuaException(L);

}

// Executes the basic game loop.
void Game::Tick()
{
	m_timer.Tick([&]()
	{
		Update(m_timer);
	});

	Render();
}

// Updates the world.
void Game::Update(DX::StepTimer const& timer)
{
	float elapsedTime = float(timer.GetElapsedSeconds());

	auto mb = mouse->GetState();

	lua_pushnumber(L, mb.x);
	lua_setglobal(L, "mx");
	lua_pushnumber(L, mb.y);
	lua_setglobal(L, "my");

	lua_pushnumber(L, mb.x / (m_outputWidth / 16));
	lua_setglobal(L, "nmx");
	lua_pushnumber(L, mb.y / (m_outputHeight / 9));
	lua_setglobal(L, "nmy");

	lua_pushboolean(L, mb.leftButton);
	lua_setglobal(L, "mbLeft");

	auto kb = keyboard->GetState();
	tracker.Update(kb);

#define KEY(name) { \
	lua_pushboolean(L, kb.name); \
	lua_setglobal(L, _STRINGIZE(name)); \
	lua_pushboolean(L, tracker.pressed.name); \
	lua_setglobal(L, _STRINGIZE(name) "P"); \
}

	KEY(D0);
	KEY(D1);
	KEY(D2);
	KEY(D3);
	KEY(D4);
	KEY(D5);

	KEY(Q);

	KEY(W);
	KEY(A);
	KEY(S);
	KEY(D);

	KEY(Up);
	KEY(Left);
	KEY(Down);
	KEY(Right);

	KEY(E);
	KEY(Space);
	KEY(NumPad0);

	lua_getglobal(L, "update");
	lua_pushnumber(L, elapsedTime);
	if (lua_pcall(L, 1, 0, 0) != 0)
		throw LuaException(L);

}

// Draws the scene.
void Game::Render()
{
	// Don't try to render anything before the first Update.
	if (m_timer.GetFrameCount() == 0)
	{
		return;
	}

	Clear();

	float elapsedTime = float(m_timer.GetElapsedSeconds());

	CommonStates states(m_d3dDevice.Get());
	

	m_spriteBatch->Begin(SpriteSortMode_Deferred, states.AlphaBlend(), states.PointClamp());

	lua_getglobal(L, "render");
	lua_pushnumber(L, elapsedTime);
	if (lua_pcall(L, 1, 0, 0) != 0)
		throw LuaException(L);

	m_spriteBatch->End();

	Present();
}

void Game::Clear()
{
	// Clear the views.
	m_d3dContext->ClearRenderTargetView(m_renderTargetView.Get(), Colors::Azure);
	m_d3dContext->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	m_d3dContext->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(), m_depthStencilView.Get());

	// Set the viewport.
	CD3D11_VIEWPORT viewport(0.0f, 0.0f, static_cast<float>(m_outputWidth), static_cast<float>(m_outputHeight));
	m_d3dContext->RSSetViewports(1, &viewport);
}

// Presents the back buffer contents to the screen.
void Game::Present()
{
	// The first argument instructs DXGI to block until VSync, putting the application
	// to sleep until the next VSync. This ensures we don't waste any cycles rendering
	// frames that will never be displayed to the screen.
	HRESULT hr = m_swapChain->Present(1, 0);

	// If the device was reset we must completely reinitialize the renderer.
	if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
	{
		OnDeviceLost();
	}
	else
	{
		DX::ThrowIfFailed(hr);
	}
}

// Message handlers
void Game::OnActivated()
{
	// TODO: Game is becoming active window.
}

void Game::OnDeactivated()
{
	// TODO: Game is becoming background window.
}

void Game::OnSuspending()
{
	// TODO: Game is being power-suspended (or minimized).
}

void Game::OnResuming()
{
	m_timer.ResetElapsedTime();

	// TODO: Game is being power-resumed (or returning from minimize).
}

void Game::OnWindowSizeChanged(int width, int height)
{
	m_outputWidth = std::max(width, 1);
	m_outputHeight = std::max(height, 1);

	CreateResources();

	// TODO: Game window is being resized.
}

// Properties
void Game::GetDefaultSize(int& width, int& height) const
{
	// TODO: Change to desired default window size (note minimum size is 320x200).
	width = 1280;
	height = 720;
}

// These are the resources that depend on the device.
void Game::CreateDevice()
{
	UINT creationFlags = 0;

#ifdef _DEBUG
	creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	static const D3D_FEATURE_LEVEL featureLevels[] =
	{
		// TODO: Modify for supported Direct3D feature levels (see code below related to 11.1 fallback handling).
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_3,
		D3D_FEATURE_LEVEL_9_2,
		D3D_FEATURE_LEVEL_9_1,
	};

	// Create the DX11 API device object, and get a corresponding context.
	HRESULT hr = D3D11CreateDevice(
		nullptr,                                // specify nullptr to use the default adapter
		D3D_DRIVER_TYPE_HARDWARE,
		nullptr,
		creationFlags,
		featureLevels,
		_countof(featureLevels),
		D3D11_SDK_VERSION,
		m_d3dDevice.ReleaseAndGetAddressOf(),   // returns the Direct3D device created
		&m_featureLevel,                        // returns feature level of device created
		m_d3dContext.ReleaseAndGetAddressOf()   // returns the device immediate context
	);

	if (hr == E_INVALIDARG)
	{
		// DirectX 11.0 platforms will not recognize D3D_FEATURE_LEVEL_11_1 so we need to retry without it.
		hr = D3D11CreateDevice(nullptr,
			D3D_DRIVER_TYPE_HARDWARE,
			nullptr,
			creationFlags,
			&featureLevels[1],
			_countof(featureLevels) - 1,
			D3D11_SDK_VERSION,
			m_d3dDevice.ReleaseAndGetAddressOf(),
			&m_featureLevel,
			m_d3dContext.ReleaseAndGetAddressOf()
		);
	}

	DX::ThrowIfFailed(hr);

#ifndef NDEBUG
	ComPtr<ID3D11Debug> d3dDebug;
	if (SUCCEEDED(m_d3dDevice.As(&d3dDebug)))
	{
		ComPtr<ID3D11InfoQueue> d3dInfoQueue;
		if (SUCCEEDED(d3dDebug.As(&d3dInfoQueue)))
		{
#ifdef _DEBUG
			d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, true);
			d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, true);
#endif
			D3D11_MESSAGE_ID hide[] =
			{
				D3D11_MESSAGE_ID_SETPRIVATEDATA_CHANGINGPARAMS,
				// TODO: Add more message IDs here as needed.
			};
			D3D11_INFO_QUEUE_FILTER filter = {};
			filter.DenyList.NumIDs = _countof(hide);
			filter.DenyList.pIDList = hide;
			d3dInfoQueue->AddStorageFilterEntries(&filter);
		}
	}
#endif

	// DirectX 11.1 if present
	if (SUCCEEDED(m_d3dDevice.As(&m_d3dDevice1)))
		(void)m_d3dContext.As(&m_d3dContext1);

	// TODO: Initialize device dependent objects here (independent of window size).
}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateResources()
{
	// Clear the previous window size specific context.
	ID3D11RenderTargetView* nullViews[] = { nullptr };
	m_d3dContext->OMSetRenderTargets(_countof(nullViews), nullViews, nullptr);
	m_renderTargetView.Reset();
	m_depthStencilView.Reset();
	m_d3dContext->Flush();

	UINT backBufferWidth = static_cast<UINT>(m_outputWidth);
	UINT backBufferHeight = static_cast<UINT>(m_outputHeight);
	DXGI_FORMAT backBufferFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
	DXGI_FORMAT depthBufferFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	UINT backBufferCount = 2;

	// If the swap chain already exists, resize it, otherwise create one.
	if (m_swapChain)
	{
		HRESULT hr = m_swapChain->ResizeBuffers(backBufferCount, backBufferWidth, backBufferHeight, backBufferFormat, 0);

		if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
		{
			// If the device was removed for any reason, a new device and swap chain will need to be created.
			OnDeviceLost();

			// Everything is set up now. Do not continue execution of this method. OnDeviceLost will reenter this method 
			// and correctly set up the new device.
			return;
		}
		else
		{
			DX::ThrowIfFailed(hr);
		}
	}
	else
	{
		// First, retrieve the underlying DXGI Device from the D3D Device.
		ComPtr<IDXGIDevice1> dxgiDevice;
		DX::ThrowIfFailed(m_d3dDevice.As(&dxgiDevice));

		// Identify the physical adapter (GPU or card) this device is running on.
		ComPtr<IDXGIAdapter> dxgiAdapter;
		DX::ThrowIfFailed(dxgiDevice->GetAdapter(dxgiAdapter.GetAddressOf()));

		// And obtain the factory object that created it.
		ComPtr<IDXGIFactory1> dxgiFactory;
		DX::ThrowIfFailed(dxgiAdapter->GetParent(IID_PPV_ARGS(dxgiFactory.GetAddressOf())));

		ComPtr<IDXGIFactory2> dxgiFactory2;
		if (SUCCEEDED(dxgiFactory.As(&dxgiFactory2)))
		{
			// DirectX 11.1 or later

			// Create a descriptor for the swap chain.
			DXGI_SWAP_CHAIN_DESC1 swapChainDesc = { 0 };
			swapChainDesc.Width = backBufferWidth;
			swapChainDesc.Height = backBufferHeight;
			swapChainDesc.Format = backBufferFormat;
			swapChainDesc.SampleDesc.Count = 1;
			swapChainDesc.SampleDesc.Quality = 0;
			swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			swapChainDesc.BufferCount = backBufferCount;

			DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapChainDesc = { 0 };
			fsSwapChainDesc.Windowed = TRUE;

			// Create a SwapChain from a Win32 window.
			DX::ThrowIfFailed(dxgiFactory2->CreateSwapChainForHwnd(
				m_d3dDevice.Get(),
				m_window,
				&swapChainDesc,
				&fsSwapChainDesc,
				nullptr,
				m_swapChain1.ReleaseAndGetAddressOf()
			));

			DX::ThrowIfFailed(m_swapChain1.As(&m_swapChain));
		}
		else
		{
			DXGI_SWAP_CHAIN_DESC swapChainDesc = { 0 };
			swapChainDesc.BufferCount = backBufferCount;
			swapChainDesc.BufferDesc.Width = backBufferWidth;
			swapChainDesc.BufferDesc.Height = backBufferHeight;
			swapChainDesc.BufferDesc.Format = backBufferFormat;
			swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			swapChainDesc.OutputWindow = m_window;
			swapChainDesc.SampleDesc.Count = 1;
			swapChainDesc.SampleDesc.Quality = 0;
			swapChainDesc.Windowed = TRUE;

			DX::ThrowIfFailed(dxgiFactory->CreateSwapChain(m_d3dDevice.Get(), &swapChainDesc, m_swapChain.ReleaseAndGetAddressOf()));
		}

		// This template does not support exclusive fullscreen mode and prevents DXGI from responding to the ALT+ENTER shortcut.
		DX::ThrowIfFailed(dxgiFactory->MakeWindowAssociation(m_window, DXGI_MWA_NO_ALT_ENTER));
	}

	// Obtain the backbuffer for this window which will be the final 3D rendertarget.
	ComPtr<ID3D11Texture2D> backBuffer;
	DX::ThrowIfFailed(m_swapChain->GetBuffer(0, IID_PPV_ARGS(backBuffer.GetAddressOf())));

	// Create a view interface on the rendertarget to use on bind.
	DX::ThrowIfFailed(m_d3dDevice->CreateRenderTargetView(backBuffer.Get(), nullptr, m_renderTargetView.ReleaseAndGetAddressOf()));

	// Allocate a 2-D surface as the depth/stencil buffer and
	// create a DepthStencil view on this surface to use on bind.
	CD3D11_TEXTURE2D_DESC depthStencilDesc(depthBufferFormat, backBufferWidth, backBufferHeight, 1, 1, D3D11_BIND_DEPTH_STENCIL);

	ComPtr<ID3D11Texture2D> depthStencil;
	DX::ThrowIfFailed(m_d3dDevice->CreateTexture2D(&depthStencilDesc, nullptr, depthStencil.GetAddressOf()));

	CD3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc(D3D11_DSV_DIMENSION_TEXTURE2D);
	DX::ThrowIfFailed(m_d3dDevice->CreateDepthStencilView(depthStencil.Get(), &depthStencilViewDesc, m_depthStencilView.ReleaseAndGetAddressOf()));

	auto bb = m_renderTargetView.Get();
	m_d3dContext->OMSetRenderTargets(1, &bb, nullptr);
	// TODO: Initialize windows-size dependent objects here.
}

void Game::OnDeviceLost()
{
	// TODO: Add Direct3D resource cleanup here.

	m_depthStencilView.Reset();
	m_renderTargetView.Reset();
	m_swapChain1.Reset();
	m_swapChain.Reset();
	m_d3dContext1.Reset();
	m_d3dContext.Reset();
	m_d3dDevice1.Reset();
	m_d3dDevice.Reset();

	CreateDevice();

	CreateResources();
}

static Game *L_GetGame(lua_State *L)
{
	lua_getglobal(L, "_GAME");
	return reinterpret_cast<Game*>(lua_touserdata(L, -1));
}

int L_LoadMaps(lua_State * L)
{
	std::vector<MapEntry> entries = {};
	LoadMaps(entries);

	lua_newtable(L);

	lua_pushnumber(L, 2);
	lua_setfield(L, -2, "test");

	for (auto entry : entries) {
		lua_newtable(L);
		int i = 1;

		for (int j = 0; j < 2; ++j) {
			lua_pushinteger(L, entry.map.spawn[j].x);
			lua_rawseti(L, -2, i++);

			lua_pushinteger(L, entry.map.spawn[j].y);
			lua_rawseti(L, -2, i++);
		}
		for (int y = 0; y < 9; ++y) {

		for (int x = 0; x < 16; ++x) {
				lua_pushinteger(L, entry.map.tiles[x][y]);
				lua_rawseti(L, -2, i++);
			}
		}

		lua_setfield(L, -2, entry.name.c_str());
	}

	return 1;
}

static void stackDump(lua_State *L) {
	int i = lua_gettop(L);
	TRACE(" ----------------  Stack Dump ----------------");
	while (i) {
		int t = lua_type(L, i);
		switch (t) {
		case LUA_TSTRING:
			TRACE(i << ": " << lua_tostring(L, i));
			break;
		case LUA_TBOOLEAN:
			TRACE(i << ": " << (lua_toboolean(L, i) ? "true" : "false"));
			break;
		case LUA_TNUMBER:
			TRACE(i << ": " << lua_tonumber(L, i));
			break;
		default: TRACE(i << ": " << lua_typename(L, t)); break;
		}
		i--;
	}
	TRACE("--------------- Stack Dump Finished ---------------");
}

int L_SaveMap(lua_State * L)
{
	luaL_checkstack(L, 2, "t");
	luaL_checktype(L, 1, LUA_TTABLE);
	luaL_checktype(L, 2, LUA_TSTRING);

	auto file = std::string(lua_tostring(L, 2));

	Map map;

	lua_rawgeti(L, 1, 1);
	map.spawn[0].x = lua_tonumber(L, -1);
	lua_rawgeti(L, 1, 2);
	map.spawn[0].y = lua_tonumber(L, -1);
	lua_rawgeti(L, 1, 3);
	map.spawn[1].x = lua_tonumber(L, -1);
	lua_rawgeti(L, 1, 4);
	map.spawn[1].y = lua_tonumber(L, -1);

	lua_pop(L, 4);

	stackDump(L);

	for (int i = 0; i < 16 * 9; ++i) {
		lua_rawgeti(L, 1, i + 5);
		int x = i % 16;
		int y = i / 16;
		map.tiles[x][y] = lua_tonumber(L, -1);
		lua_pop(L, 1);
	}

	stackDump(L);

	SerializeMap(&map, file);

	return 0;
}

static int L_LoadSprite(lua_State *L)
{
	luaL_checkstack(L, 1, "test");
	const char *name = luaL_checkstring(L, -1);

	Game *game = L_GetGame(L);

	int len = strlen(name);
	std::wstring wc(len, L'#');
	mbstowcs(&wc[0], name, len);

	const SpriteSheet::SpriteFrame *frame = game->m_spriteSheet->Find(wc.c_str());

	if (frame) {
		lua_pushlightuserdata(L, const_cast<void*>(reinterpret_cast<const void*>(frame)));
		return 1;
	}
	else {
		luaL_error(L, "Could not find sprite `%s`", name);
		return 0;
	}
}

static int L_DrawSprite(lua_State *L)
{
	luaL_checkstack(L, 7, "ttetst");

	luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
	auto frame = reinterpret_cast<SpriteSheet::SpriteFrame*>(lua_touserdata(L, 1));
	if (!frame) {
		luaL_error(L, "Invalid sprite");
		return 0;
	}

	auto x = luaL_checknumber(L, 2);
	auto y = luaL_checknumber(L, 3);
	auto w = luaL_checknumber(L, 4);
	auto h = luaL_checknumber(L, 5);
	auto col = luaL_checkinteger(L, 6);
	auto rot = luaL_checknumber(L, 7);

	Game *game = L_GetGame(L);
	game->m_spriteSheet->Draw(game->m_spriteBatch.get(), *frame, XMFLOAT4(x, y, w, h), normalize_color(col), rot);

	return 0;
}

static int L_DrawText(lua_State *L)
{
	luaL_checkstack(L, 4, "ttetst");

	auto str = luaL_checkstring(L, 1);
	auto x = luaL_checknumber(L, 2);
	auto y = luaL_checknumber(L, 3);
	auto col = luaL_checkinteger(L, 4);

	int len = strlen(str);
	std::wstring wc(len, L'#');
	mbstowcs(&wc[0], str, len);

	Game *game = L_GetGame(L);
	game->m_spriteFont->DrawString(game->m_spriteBatch.get(), wc.c_str(), XMFLOAT2(x, y), normalize_color(col));

	return 0;
}

static int L_Print(lua_State *L)
{
	int nargs = lua_gettop(L);
	std::ostringstream luaout;
	for (int i = 1; i <= nargs; i++) {
		int t = lua_type(L, i);
		switch (t) {
		case LUA_TSTRING: { /* strings */
			luaout << lua_tostring(L, i);
			break;
		}
		case LUA_TBOOLEAN: { /* booleans */
			luaout << (lua_toboolean(L, i) ? "true" : "false");
			break;
		}
		case LUA_TNUMBER: { /* numbers */
			luaout << lua_tonumber(L, i);
			break;
		}
		default: { /* other values */
			luaout << lua_typename(L, t);
			break;
		}
		}
		if (i != nargs) {
			luaout << "\t";
		}
	}
	luaout << std::endl;
	OutputDebugStringA(luaout.str().c_str());
	return 0;
}
