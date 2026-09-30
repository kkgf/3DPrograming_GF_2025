//
// Game.cpp
//

#include "pch.h"
#include "Game.h"

extern void ExitGame() noexcept;

using namespace DirectX;
using namespace DirectX::SimpleMath;// ③ 追加

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

    // ⑥ 追加
    // キーボードの現在の状態を取得する
    auto keyboard = Keyboard::Get().GetState();

    // 移動速度：単位/秒
    float speed = 3.0f * elapsedTime;

    // 左右移動
    if (keyboard.A)
    {
        m_cameraPosition.x -= speed;
    }

    if (keyboard.D)
    {
        m_cameraPosition.x += speed;
    }

    // 前後移動
    if (keyboard.W)
    {
        m_cameraPosition.z -= speed;
    }

    if (keyboard.S)
    {
        m_cameraPosition.z += speed;
    }

    // 上下移動
    if (keyboard.E)
    {
        m_cameraPosition.y += speed;
    }

    if (keyboard.Q)
    {
        m_cameraPosition.y -= speed;
    }

    // カメラが被写体に近づきすぎないようにする
    if (m_cameraPosition.z < 1.0f)
    {
        m_cameraPosition.z = 1.0f;
    }

    // ビュー行列を作成
    m_view =
        Matrix::CreateLookAt(
            m_cameraPosition,
            m_cameraTarget,
            Vector3::Up
        );
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

    // ⑦ 追加
    // 左に配置する立方体
    Matrix worldLeft =
        Matrix::CreateTranslation(
            -2.0f,
            0.0f,
            0.0f
        );

    m_cube->Draw(
        worldLeft,
        m_view,
        m_projection,
        Colors::Red
    );


    // 真ん中に配置する立方体
    Matrix worldCenter = Matrix::Identity;

    m_cube->Draw(
        worldCenter,
        m_view,
        m_projection,
        Colors::Green
    );


    // 遠くに配置する立方体
    Matrix worldBack =
        Matrix::CreateTranslation(
            2.0f,
            0.0f,
            -3.0f
        );

    m_cube->Draw(
        worldBack,
        m_view,
        m_projection,
        Colors::Blue
    );

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

    // ④ 追加
    auto context = m_deviceResources->GetD3DDeviceContext();

    m_cube =
        GeometricPrimitive::CreateCube(
            context,
            1.0f
        );
}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateWindowSizeDependentResources()
{
    // ⑤ 置換
    auto size = m_deviceResources->GetOutputSize();

    float width = static_cast<float>(size.right - size.left);

    float height = static_cast<float>(size.bottom - size.top);

    float aspectRatio = width / height;

    // 投影は画面のアスペクト比に依存するので、ウィンドウサイズが変更されたときに更新する必要がある。
    m_projection =
        Matrix::CreatePerspectiveFieldOfView(
            XMConvertToRadians(45.0f),
            aspectRatio,
            0.1f,
            100.0f
        );
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
