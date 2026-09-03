//
// Game.cpp
//

#include "pch.h"
#include "Game.h"

extern void ExitGame() noexcept;

using namespace DirectX;

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
    //float elapsedTime = static_cast<float>(timer.GetElapsedSeconds());

    // TODO: Add your game logic here.
    //elapsedTime;
}
#pragma endregion

#pragma region Frame Render
// Draws the scene.
void Game::Render()
{
    // Don't try to render anything before the first Update.
    if (m_timer.GetFrameCount() == 0)   return;

    Clear();

    m_deviceResources->PIXBeginEvent(L"Render");
    auto context = m_deviceResources->GetD3DDeviceContext();

    //// Call the rendering function here
    //DrawGrid();
    //RenderCenterPixel(Colors::Red);
    //DrawRectangle(100, 100, 200, 150, Colors::CornflowerBlue);

    // Render the loaded mesh
    if (m_model)
    {
        m_model->Draw(context, *m_states, m_world, m_view, m_proj);
    }

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

    context->ClearRenderTargetView(renderTarget, Colors::Black);
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
    auto context = m_deviceResources->GetD3DDeviceContext();

    // Common rendering states
    m_states = std::make_unique<CommonStates>(device);

    // Sprite rendering
    m_spriteBatch = std::make_unique<SpriteBatch>(context);
    CreatePixelTexture();

    // Model rendering
    m_fxFactory = std::make_unique<EffectFactory>(device);
    m_fxFactory->SetDirectory(L"Resources/Models");

    m_model = Model::CreateFromSDKMESH(
        device,
        L"Resources/Models/Duck1.sdkmesh",
        *m_fxFactory
    );

    m_world = DirectX::SimpleMath::Matrix::Identity;
}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateWindowSizeDependentResources()
{
    auto size = m_deviceResources->GetOutputSize();
    float aspectRatio = float(size.right) / float(size.bottom);

    // Set camera position (adjust position coordinates based on model scale)
    m_view = DirectX::SimpleMath::Matrix::CreateLookAt(
        DirectX::SimpleMath::Vector3(0.0f, 2.0f, -5.0f), // Camera position
        DirectX::SimpleMath::Vector3(0.0f, 0.0f, 0.0f),  // Target position
        DirectX::SimpleMath::Vector3(0.0f, 1.0f, 0.0f)   // Up vector
    );

    m_proj = DirectX::SimpleMath::Matrix::CreatePerspectiveFieldOfView(
        DirectX::XMConvertToRadians(45.0f),
        aspectRatio,
        0.1f,
        1000.0f
    );
}

/**
 * Creates a 1x1 pixel texture that can be used for rendering a single pixel.
 * The texture is created with a white color (0xFFFFFFFF) and is immutable.
 */
void Game::CreatePixelTexture()
{
    // this line gets the D3D device from the device resources, which is needed to create textures and other GPU resources
    auto device = m_deviceResources->GetD3DDevice();

    // Define a 1x1 raw RGBA pixel (white: 0xFFFFFFFF)
    const uint32_t pixelColor = 0xFFFFFFFF;

    // Set up the texture description and initial data for a 1x1 texture
    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = &pixelColor;
    initData.SysMemPitch = sizeof(uint32_t);

    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = 1;
    desc.Height = 1;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_IMMUTABLE; // CPU won't modify this after creation
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    // allocates gpu memory for the texture and creates a shader resource view for it, which can be used in shaders
    ComPtr<ID3D11Texture2D> tex;
    DX::ThrowIfFailed(device->CreateTexture2D(&desc, &initData, tex.GetAddressOf()));
    DX::ThrowIfFailed(device->CreateShaderResourceView(tex.Get(), nullptr, m_pixelTexture.ReleaseAndGetAddressOf()));
}

/**
 * Renders a single pixel at the center of the screen.
 */
void Game::RenderCenterPixel(DirectX::FXMVECTOR color)
{
    const auto viewport = m_deviceResources->GetScreenViewport();

    // Calculate center screen coordinates (offset by 0.5 for pixel alignment)
    const float centerX = (viewport.Width * 0.5f) - 0.5f;
    const float centerY = (viewport.Height * 0.5f) - 0.5f;

    m_spriteBatch->Begin();

    // Draw the 1x1 texture at exact center. Color tinting can be changed via DirectX::Colors
    m_spriteBatch->Draw(
        m_pixelTexture.Get(),
        XMFLOAT2(centerX, centerY),
        nullptr,
        color
    );

    m_spriteBatch->End();
}

/**
 * Draws a grid of pixels across the entire screen with specified spacing and color.
 * @param spacing The distance in pixels between each grid point.
 * @param color The color of the grid points, specified as a DirectX::FXMVECTOR.
 */
void Game::DrawGrid(int spacing, DirectX::FXMVECTOR color)
{
    const auto viewport = m_deviceResources->GetScreenViewport();

    m_spriteBatch->Begin();

    // Loop through the screen height and width in steps of 'spacing'
    for (int y = 0; y < viewport.Height; y += spacing)
    {
        for (int x = 0; x < viewport.Width; x += spacing)
        {
            m_spriteBatch->Draw(
                m_pixelTexture.Get(),
                DirectX::XMFLOAT2(static_cast<float>(x), static_cast<float>(y)),
                nullptr,
                color
            );
        }
    }

    m_spriteBatch->End();
}

/**
 * Draws a rectangle on the screen using the 1x1 pixel texture.
 * @param x The x-coordinate of the top-left corner of the rectangle.
 * @param y The y-coordinate of the top-left corner of the rectangle.
 * @param width The width of the rectangle in pixels.
 * @param height The height of the rectangle in pixels.
 * @param color The color of the rectangle, specified as a DirectX::FXMVECTOR.
 */
void Game::DrawRectangle(int x, int y, int width, int height, DirectX::FXMVECTOR color)
{
    RECT destination = { x, y, x + width, y + height };

    m_spriteBatch->Begin();
    m_spriteBatch->Draw(
        m_pixelTexture.Get(),
        destination,
        nullptr,
        color
    );
    m_spriteBatch->End();
}

void Game::OnDeviceLost()
{
    // Clean up GPU-dependent resources when Direct3D device is reset/lost
    m_spriteBatch.reset();
    m_pixelTexture.Reset();
}

void Game::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#pragma endregion
