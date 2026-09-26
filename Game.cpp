//
// Game.cpp
//

#include "pch.h"
#include "Game.h"

extern void ExitGame() noexcept;

using namespace DirectX;
using namespace DirectX::SimpleMath;// ③追加

using Microsoft::WRL::ComPtr;

Game::Game() noexcept(false)
{
    m_deviceResources = std::make_unique<DX::DeviceResources>();
    // TODO: Provide parameters for swapchain format, depth/stencil format, and backbuffer count.
    //   Add DX::DeviceResources::c_AllowTearing to opt-in to variable rate displays.
    //   Add DX::DeviceResources::c_EnableHDR for HDR10 display.
    m_deviceResources->RegisterDeviceNotify(this);
}

// Initialize the Direct3D resources required to run.
void Game::Initialize(HWND window, int width, int height)
{
    m_deviceResources->SetWindow(window, width, height);

    m_deviceResources->CreateDeviceResources();
    CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();

    // TODO: Change the timer settings if you want something other than the default variable timestep mode.
    // e.g. for 60 FPS fixed timestep update logic, call:
    /*
    m_timer.SetFixedTimeStep(true);
    m_timer.SetTargetElapsedSeconds(1.0 / 60);
    */
}

#pragma region Frame Update
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

    // TODO: Add your game logic here.
    elapsedTime;
}
#pragma endregion

#pragma region Frame Render
// Draws the scene.
void Game::Render()
{
    // Don't try to render anything before the first Update.
    if (m_timer.GetFrameCount() == 0)
    {
        return;
    }

    Clear();

    m_deviceResources->PIXBeginEvent(L"Render");
    auto context = m_deviceResources->GetD3DDeviceContext();

    // ⑤置換
    //描画の設定
    context->OMSetBlendState(
        m_states->Opaque(),
        nullptr,
        0xFFFFFFFF
    );

    context->OMSetDepthStencilState(m_states->DepthNone(),0);

    context->RSSetState(m_states->CullNone());

    // サンプリング方法をGPUに伝える
    auto sampler = m_states->LinearClamp();
    context->PSSetSamplers(0, 1, &sampler);

    // Direct3Dにvertexに含まれる情報を伝える
    context->IASetInputLayout(m_inputLayout.Get());

    // どのように描画するかを決める
    m_effect->Apply(context);

    // 図形
    VertexType topLeft(
        Vector3(-0.7f, 0.7f, 0.5f),
        Vector2(0.0f, 0.0f)
    );

    VertexType topRight(
        Vector3(0.7f, 0.7f, 0.5f),
        Vector2(1.0f, 0.0f)
    );

    VertexType bottomRight(
        Vector3(0.7f, -0.7f, 0.5f),
        Vector2(1.0f, 1.0f)
    );

    VertexType bottomLeft(
        Vector3(-0.7f, -0.7f, 0.5f),
        Vector2(0.0f, 1.0f)
    );

    // 2つの三角形を描画することで、四角形を描画する
    m_batch->Begin();

    m_batch->DrawTriangle(topLeft, topRight, bottomRight);

    m_batch->DrawTriangle(topLeft, bottomRight, bottomLeft);

    m_batch->End();

    m_deviceResources->PIXEndEvent();

    // Show the new frame.
    m_deviceResources->Present();
}

// Helper method to clear the back buffers.
void Game::Clear()
{
    m_deviceResources->PIXBeginEvent(L"Clear");

    // Clear the views.
    auto context = m_deviceResources->GetD3DDeviceContext();
    auto renderTarget = m_deviceResources->GetRenderTargetView();
    auto depthStencil = m_deviceResources->GetDepthStencilView();

    context->ClearRenderTargetView(renderTarget, Colors::Beige);
    context->ClearDepthStencilView(depthStencil, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    context->OMSetRenderTargets(1, &renderTarget, depthStencil);

    // Set the viewport.
    const auto viewport = m_deviceResources->GetScreenViewport();
    context->RSSetViewports(1, &viewport);

    m_deviceResources->PIXEndEvent();
}
#pragma endregion

#pragma region Message Handlers
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

void Game::OnWindowMoved()
{
    const auto r = m_deviceResources->GetOutputSize();
    m_deviceResources->WindowSizeChanged(r.right, r.bottom);
}

void Game::OnDisplayChange()
{
    m_deviceResources->UpdateColorSpace();
}

void Game::OnWindowSizeChanged(int width, int height)
{
    if (!m_deviceResources->WindowSizeChanged(width, height))
        return;

    CreateWindowSizeDependentResources();

    // TODO: Game window is being resized.
}

// Properties
void Game::GetDefaultSize(int& width, int& height) const noexcept
{
    // TODO: Change to desired default window size (note minimum size is 320x200).
    width = 1280;
    height = 720;
}
#pragma endregion

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void Game::CreateDeviceDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();
    // ④置換
    auto context = m_deviceResources->GetD3DDeviceContext();

    // CommonStates
    m_states = std::make_unique<CommonStates>(device);

    // テクスチャの読み込み
    DX::ThrowIfFailed(
        CreateWICTextureFromFile(
            device,
            L"rocks.jpg",
            nullptr,
            m_texture.ReleaseAndGetAddressOf()
        )
    );

    // BasicEffect作成
    m_effect = std::make_unique<BasicEffect>(device);
    // テクスチャを有効化（使えるようにする）
    m_effect->SetTextureEnabled(true);
    // BasicEffectにテクスチャをセット
    m_effect->SetTexture(m_texture.Get());

    // 各行列を単位行列にすることで、描画時の変換を行わないようにする
    m_effect->SetWorld(XMMatrixIdentity());
    m_effect->SetView(XMMatrixIdentity());
    m_effect->SetProjection(XMMatrixIdentity());

    // 入力レイアウトを作成
    DX::ThrowIfFailed(
        CreateInputLayoutFromEffect<VertexType>(
            device,
            m_effect.get(),
            m_inputLayout.ReleaseAndGetAddressOf()
        )
    );

    // PrimitiveBatch作成
    m_batch = std::make_unique<PrimitiveBatch<VertexType>>(context);
}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateWindowSizeDependentResources()
{
    // TODO: Initialize windows-size dependent objects here.
}

void Game::OnDeviceLost()
{
    // TODO: Add Direct3D resource cleanup here.
}

void Game::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#pragma endregion
