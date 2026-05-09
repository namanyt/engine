#include "runtime/StartupFlowOverlay.h"

#include "assets/AssetManager.h"
#include "core/Renderer.h"

#include <glad/glad.h>

#include <algorithm>
#include <filesystem>
#include <functional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#if !defined(NOMINMAX)
#define NOMINMAX
#endif
#include <windows.h>
#include <wincodec.h>
#endif

namespace
{
constexpr int kOverlayWidth = 1920;
constexpr int kOverlayHeight = 1080;

unsigned int uploadTexture(const std::vector<std::uint8_t>& rgbaPixels, int width, int height)
{
    unsigned int textureId = 0;
    glGenTextures(1, &textureId);
    if (textureId == 0)
    {
        throw std::runtime_error("Failed to allocate a startup flow overlay texture.");
    }

    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 rgbaPixels.data());
    glBindTexture(GL_TEXTURE_2D, 0);
    return textureId;
}

#if defined(_WIN32)
struct DecodedImage final
{
    int width = 0;
    int height = 0;
    std::vector<std::uint8_t> rgbaBytes;
};

template <typename T> struct ComReleaser final
{
    void operator()(T* value) const noexcept
    {
        if (value != nullptr)
        {
            value->Release();
        }
    }
};

template <typename T> using ComPtr = std::unique_ptr<T, ComReleaser<T>>;

class ScopedCom final
{
  public:
    ScopedCom()
    {
        const HRESULT result = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        if (result == S_OK || result == S_FALSE)
        {
            m_shouldUninitialize = true;
            return;
        }

        if (result != RPC_E_CHANGED_MODE)
        {
            throw std::runtime_error("Failed to initialize COM for startup overlay decoding.");
        }
    }

    ~ScopedCom()
    {
        if (m_shouldUninitialize)
        {
            CoUninitialize();
        }
    }

  private:
    bool m_shouldUninitialize = false;
};

struct FontScope final
{
    explicit FontScope(HFONT font) : handle(font) {}
    ~FontScope()
    {
        if (handle != nullptr)
        {
            DeleteObject(handle);
        }
    }

    FontScope(const FontScope&) = delete;
    FontScope& operator=(const FontScope&) = delete;

    HFONT handle = nullptr;
};

struct PaintTextCommand final
{
    std::wstring text;
    RECT bounds{};
    int pointSize = 18;
    int weight = FW_NORMAL;
    COLORREF color = RGB(255, 255, 255);
    UINT format = DT_CENTER | DT_VCENTER | DT_WORDBREAK;
};

void throwIfFailed(HRESULT result, const std::string& message)
{
    if (FAILED(result))
    {
        std::ostringstream stream;
        stream << message << " (HRESULT=0x" << std::hex << static_cast<unsigned long>(result)
               << ')';
        throw std::runtime_error(stream.str());
    }
}

DecodedImage decodeWithWic(const std::filesystem::path& path)
{
    ScopedCom com;

    IWICImagingFactory* factoryRaw = nullptr;
    throwIfFailed(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                                   IID_IWICImagingFactory, reinterpret_cast<void**>(&factoryRaw)),
                  "Failed to create WIC imaging factory.");
    ComPtr<IWICImagingFactory> factory(factoryRaw);

    IWICBitmapDecoder* decoderRaw = nullptr;
    throwIfFailed(factory->CreateDecoderFromFilename(path.c_str(), nullptr, GENERIC_READ,
                                                     WICDecodeMetadataCacheOnLoad, &decoderRaw),
                  "Failed to open startup overlay backdrop: " + path.string());
    ComPtr<IWICBitmapDecoder> decoder(decoderRaw);

    IWICBitmapFrameDecode* frameRaw = nullptr;
    throwIfFailed(decoder->GetFrame(0, &frameRaw),
                  "Failed to read backdrop frame: " + path.string());
    ComPtr<IWICBitmapFrameDecode> frame(frameRaw);

    UINT width = 0;
    UINT height = 0;
    throwIfFailed(frame->GetSize(&width, &height),
                  "Failed to read backdrop dimensions: " + path.string());

    IWICFormatConverter* converterRaw = nullptr;
    throwIfFailed(factory->CreateFormatConverter(&converterRaw),
                  "Failed to create WIC format converter.");
    ComPtr<IWICFormatConverter> converter(converterRaw);

    throwIfFailed(converter->Initialize(frame.get(), GUID_WICPixelFormat32bppRGBA,
                                        WICBitmapDitherTypeNone, nullptr, 0.0,
                                        WICBitmapPaletteTypeCustom),
                  "Failed to convert backdrop to RGBA8: " + path.string());

    DecodedImage image{};
    image.width = static_cast<int>(width);
    image.height = static_cast<int>(height);
    image.rgbaBytes.resize(static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4u);
    throwIfFailed(converter->CopyPixels(nullptr, width * 4u,
                                        static_cast<UINT>(image.rgbaBytes.size()),
                                        image.rgbaBytes.data()),
                  "Failed to copy backdrop pixels: " + path.string());
    return image;
}

std::filesystem::path resolveMenuBackdropPath(const engine::AssetManager& assetManager)
{
    return assetManager.resolveAssetPath(std::filesystem::path("textures") / "background.png");
}

void copyScaledBackdropToBgra(const DecodedImage& image, std::uint8_t* destination)
{
    if (image.width <= 0 || image.height <= 0 || destination == nullptr)
    {
        return;
    }

    const float sourceWidth = static_cast<float>(image.width);
    const float sourceHeight = static_cast<float>(image.height);
    const float scale = std::max(static_cast<float>(kOverlayWidth) / sourceWidth,
                                 static_cast<float>(kOverlayHeight) / sourceHeight);
    const float sampleWidth = static_cast<float>(kOverlayWidth) / scale;
    const float sampleHeight = static_cast<float>(kOverlayHeight) / scale;
    const float xOffset = (sourceWidth - sampleWidth) * 0.5f;
    const float yOffset = (sourceHeight - sampleHeight) * 0.5f;

    for (int y = 0; y < kOverlayHeight; ++y)
    {
        const float sourceY =
            yOffset +
            ((static_cast<float>(y) + 0.5f) / static_cast<float>(kOverlayHeight)) * sampleHeight;
        const int sampleY = std::clamp(static_cast<int>(sourceY), 0, image.height - 1);
        for (int x = 0; x < kOverlayWidth; ++x)
        {
            const float sourceX =
                xOffset +
                ((static_cast<float>(x) + 0.5f) / static_cast<float>(kOverlayWidth)) * sampleWidth;
            const int sampleX = std::clamp(static_cast<int>(sourceX), 0, image.width - 1);

            const std::size_t sourceIndex =
                (static_cast<std::size_t>(sampleY) * static_cast<std::size_t>(image.width) +
                 static_cast<std::size_t>(sampleX)) *
                4u;
            const float red = static_cast<float>(image.rgbaBytes[sourceIndex + 0]) / 255.0f;
            const float green = static_cast<float>(image.rgbaBytes[sourceIndex + 1]) / 255.0f;
            const float blue = static_cast<float>(image.rgbaBytes[sourceIndex + 2]) / 255.0f;
            const float luminance = red * 0.299f + green * 0.587f + blue * 0.114f;

            const float gradedRed = ((red * 0.86f + luminance * 0.14f) * 0.74f) + 0.03f;
            const float gradedGreen = ((green * 0.86f + luminance * 0.14f) * 0.74f) + 0.04f;
            const float gradedBlue = ((blue * 0.86f + luminance * 0.14f) * 0.74f) + 0.05f;

            const std::size_t destinationIndex =
                (static_cast<std::size_t>(y) * static_cast<std::size_t>(kOverlayWidth) +
                 static_cast<std::size_t>(x)) *
                4u;
            destination[destinationIndex + 0] =
                static_cast<std::uint8_t>(std::clamp(gradedBlue, 0.0f, 1.0f) * 255.0f + 0.5f);
            destination[destinationIndex + 1] =
                static_cast<std::uint8_t>(std::clamp(gradedGreen, 0.0f, 1.0f) * 255.0f + 0.5f);
            destination[destinationIndex + 2] =
                static_cast<std::uint8_t>(std::clamp(gradedRed, 0.0f, 1.0f) * 255.0f + 0.5f);
            destination[destinationIndex + 3] = 255;
        }
    }
}

int pointSizeToPixels(int pointSize)
{
    HDC screenDc = GetDC(nullptr);
    const int dpi = screenDc != nullptr ? GetDeviceCaps(screenDc, LOGPIXELSY) : 96;
    if (screenDc != nullptr)
    {
        ReleaseDC(nullptr, screenDc);
    }

    return -MulDiv(pointSize, dpi, 72);
}

void drawTextCommand(HDC deviceContext, const PaintTextCommand& command, const wchar_t* faceName)
{
    FontScope font(CreateFontW(pointSizeToPixels(command.pointSize), 0, 0, 0, command.weight, FALSE,
                               FALSE, FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS,
                               CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
                               VARIABLE_PITCH | FF_DONTCARE, faceName));
    if (font.handle == nullptr)
    {
        throw std::runtime_error("Failed to create a startup flow overlay font.");
    }

    HGDIOBJ previousFont = SelectObject(deviceContext, font.handle);
    SetBkMode(deviceContext, TRANSPARENT);
    SetTextColor(deviceContext, command.color);

    RECT bounds = command.bounds;
    DrawTextW(deviceContext, command.text.c_str(), -1, &bounds, command.format);

    SelectObject(deviceContext, previousFont);
}

void fillSolidRect(HDC deviceContext, const RECT& bounds, COLORREF color)
{
    HBRUSH brush = CreateSolidBrush(color);
    FillRect(deviceContext, &bounds, brush);
    DeleteObject(brush);
}

engine::StartupFlowOverlay
paintWindowsOverlay(const std::function<void(HDC, const RECT&)>& paintCallback,
                    bool opaqueBackground, const std::filesystem::path& backdropPath = {})
{
    BITMAPINFO bitmapInfo{};
    bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bitmapInfo.bmiHeader.biWidth = kOverlayWidth;
    bitmapInfo.bmiHeader.biHeight = -kOverlayHeight;
    bitmapInfo.bmiHeader.biPlanes = 1;
    bitmapInfo.bmiHeader.biBitCount = 32;
    bitmapInfo.bmiHeader.biCompression = BI_RGB;

    void* pixelMemory = nullptr;
    HDC deviceContext = CreateCompatibleDC(nullptr);
    if (deviceContext == nullptr)
    {
        throw std::runtime_error("Failed to create a startup flow overlay device context.");
    }

    HBITMAP bitmap =
        CreateDIBSection(deviceContext, &bitmapInfo, DIB_RGB_COLORS, &pixelMemory, nullptr, 0);
    if (bitmap == nullptr || pixelMemory == nullptr)
    {
        DeleteDC(deviceContext);
        throw std::runtime_error("Failed to allocate a startup flow overlay bitmap.");
    }

    HGDIOBJ previousBitmap = SelectObject(deviceContext, bitmap);
    RECT fullRect{0, 0, kOverlayWidth, kOverlayHeight};
    if (!backdropPath.empty() && std::filesystem::exists(backdropPath))
    {
        copyScaledBackdropToBgra(decodeWithWic(backdropPath),
                                 static_cast<std::uint8_t*>(pixelMemory));
    }
    else
    {
        HBRUSH clearBrush = CreateSolidBrush(RGB(0, 0, 0));
        FillRect(deviceContext, &fullRect, clearBrush);
        DeleteObject(clearBrush);
    }

    paintCallback(deviceContext, fullRect);

    const auto* bgraBytes = static_cast<const std::uint8_t*>(pixelMemory);
    std::vector<std::uint8_t> rgbaPixels(static_cast<std::size_t>(kOverlayWidth) *
                                         static_cast<std::size_t>(kOverlayHeight) * 4u);
    for (int index = 0; index < kOverlayWidth * kOverlayHeight; ++index)
    {
        const std::uint8_t blue = bgraBytes[index * 4 + 0];
        const std::uint8_t green = bgraBytes[index * 4 + 1];
        const std::uint8_t red = bgraBytes[index * 4 + 2];
        const std::uint8_t alpha = opaqueBackground ? 255 : std::max({red, green, blue});
        rgbaPixels[index * 4 + 0] = red;
        rgbaPixels[index * 4 + 1] = green;
        rgbaPixels[index * 4 + 2] = blue;
        rgbaPixels[index * 4 + 3] = alpha;
    }

    SelectObject(deviceContext, previousBitmap);
    DeleteObject(bitmap);
    DeleteDC(deviceContext);
    return engine::StartupFlowOverlay(uploadTexture(rgbaPixels, kOverlayWidth, kOverlayHeight),
                                      kOverlayWidth, kOverlayHeight);
}
#endif
} // namespace

namespace engine
{
StartupFlowOverlay::StartupFlowOverlay(unsigned int textureId, int width, int height) noexcept
    : m_textureId(textureId), m_width(width), m_height(height)
{
}

StartupFlowOverlay::~StartupFlowOverlay()
{
    reset();
}

StartupFlowOverlay::StartupFlowOverlay(StartupFlowOverlay&& other) noexcept
    : m_textureId(std::exchange(other.m_textureId, 0)), m_width(std::exchange(other.m_width, 0)),
      m_height(std::exchange(other.m_height, 0))
{
}

StartupFlowOverlay& StartupFlowOverlay::operator=(StartupFlowOverlay&& other) noexcept
{
    if (this == &other)
    {
        return *this;
    }

    reset();
    m_textureId = std::exchange(other.m_textureId, 0);
    m_width = std::exchange(other.m_width, 0);
    m_height = std::exchange(other.m_height, 0);
    return *this;
}

bool StartupFlowOverlay::valid() const noexcept
{
    return m_textureId != 0 && m_width > 0 && m_height > 0;
}

unsigned int StartupFlowOverlay::textureId() const noexcept
{
    return m_textureId;
}

int StartupFlowOverlay::width() const noexcept
{
    return m_width;
}

int StartupFlowOverlay::height() const noexcept
{
    return m_height;
}

void StartupFlowOverlay::apply(Renderer& renderer, const RuntimeOverlayOptions& options) const
{
    if (!valid())
    {
        renderer.clearRuntimeOverlayTexture();
        return;
    }

    renderer.setRuntimeOverlayTexture(m_textureId, m_width, m_height, options);
}

void StartupFlowOverlay::reset() noexcept
{
    if (m_textureId != 0)
    {
        glDeleteTextures(1, &m_textureId);
    }

    m_textureId = 0;
    m_width = 0;
    m_height = 0;
}

StartupFlowOverlay StartupFlowOverlay::createSolid(std::uint8_t red, std::uint8_t green,
                                                   std::uint8_t blue, std::uint8_t alpha)
{
    const std::vector<std::uint8_t> rgbaPixels{red, green, blue, alpha};
    return StartupFlowOverlay(uploadTexture(rgbaPixels, 1, 1), 1, 1);
}

StartupFlowOverlay StartupFlowOverlay::createDisclaimer()
{
#if defined(_WIN32)
    return paintWindowsOverlay(
        [](HDC deviceContext, const RECT& fullRect)
        {
            const RECT textBounds{fullRect.left + 320, fullRect.top + 390, fullRect.right - 320,
                                  fullRect.bottom - 390};
            drawTextCommand(
                deviceContext,
                PaintTextCommand{
                    L"This game is not suitable for children\nor those who are easily disturbed.",
                    textBounds, 30, FW_NORMAL, RGB(244, 244, 244),
                    DT_CENTER | DT_VCENTER | DT_WORDBREAK},
                L"Segoe UI");
        },
        false);
#else
    throw std::runtime_error("Startup disclaimer overlay generation is only available on Windows.");
#endif
}

StartupFlowOverlay StartupFlowOverlay::createMenu(const AssetManager& assetManager,
                                                  MainMenuSelection selection)
{
#if defined(_WIN32)
    return paintWindowsOverlay(
        [selection](HDC deviceContext, const RECT& fullRect)
        {
            fillSolidRect(deviceContext, RECT{0, 0, 760, fullRect.bottom}, RGB(30, 36, 43));

            const RECT titleBounds{fullRect.left + 180, fullRect.top + 124, fullRect.left + 760,
                                   fullRect.top + 184};
            drawTextCommand(deviceContext,
                            PaintTextCommand{L"ENGINE", titleBounds, 26, FW_SEMIBOLD,
                                             RGB(242, 240, 232),
                                             DT_LEFT | DT_VCENTER | DT_SINGLELINE},
                            L"Segoe UI");

            const RECT buildBounds{fullRect.left + 28, fullRect.top + 24, fullRect.left + 320,
                                   fullRect.top + 54};
            drawTextCommand(deviceContext,
                            PaintTextCommand{L"prototype build", buildBounds, 14, FW_NORMAL,
                                             RGB(174, 185, 191),
                                             DT_LEFT | DT_VCENTER | DT_SINGLELINE},
                            L"Segoe UI");

            const COLORREF startColor =
                selection == MainMenuSelection::NewGame ? RGB(244, 214, 128) : RGB(230, 233, 236);
            const COLORREF settingsColor =
                selection == MainMenuSelection::Settings ? RGB(244, 214, 128) : RGB(230, 233, 236);
            const COLORREF quitColor =
                selection == MainMenuSelection::Quit ? RGB(244, 214, 128) : RGB(196, 203, 209);
            const int startWeight =
                selection == MainMenuSelection::NewGame ? FW_SEMIBOLD : FW_NORMAL;
            const int settingsWeight =
                selection == MainMenuSelection::Settings ? FW_SEMIBOLD : FW_NORMAL;
            const int quitWeight = selection == MainMenuSelection::Quit ? FW_SEMIBOLD : FW_NORMAL;

            const RECT startBounds{fullRect.left + 180, fullRect.top + 468, fullRect.left + 620,
                                   fullRect.top + 536};
            const RECT continueBounds{fullRect.left + 180, fullRect.top + 556, fullRect.left + 620,
                                      fullRect.top + 620};
            const RECT settingsBounds{fullRect.left + 180, fullRect.top + 644, fullRect.left + 620,
                                      fullRect.top + 708};
            const RECT quitBounds{fullRect.left + 180, fullRect.top + 732, fullRect.left + 620,
                                  fullRect.top + 796};
            drawTextCommand(deviceContext,
                            PaintTextCommand{selection == MainMenuSelection::NewGame ? L"> New Game"
                                                                                     : L"New Game",
                                             startBounds, 26, startWeight, startColor,
                                             DT_LEFT | DT_VCENTER | DT_SINGLELINE},
                            L"Segoe UI");
            drawTextCommand(deviceContext,
                            PaintTextCommand{L"Continue", continueBounds, 24, FW_NORMAL,
                                             RGB(112, 120, 128),
                                             DT_LEFT | DT_VCENTER | DT_SINGLELINE},
                            L"Segoe UI");
            drawTextCommand(deviceContext,
                            PaintTextCommand{selection == MainMenuSelection::Settings
                                                 ? L"> Settings"
                                                 : L"Settings",
                                             settingsBounds, 24, settingsWeight, settingsColor,
                                             DT_LEFT | DT_VCENTER | DT_SINGLELINE},
                            L"Segoe UI");
            drawTextCommand(
                deviceContext,
                PaintTextCommand{selection == MainMenuSelection::Quit ? L"> Quit" : L"Quit",
                                 quitBounds, 24, quitWeight, quitColor,
                                 DT_LEFT | DT_VCENTER | DT_SINGLELINE},
                L"Segoe UI");
        },
        true, resolveMenuBackdropPath(assetManager));
#else
    throw std::runtime_error("Startup menu overlay generation is only available on Windows.");
#endif
}

StartupFlowOverlay StartupFlowOverlay::createPauseMenu(PauseMenuSelection selection)
{
#if defined(_WIN32)
    return paintWindowsOverlay(
        [selection](HDC deviceContext, const RECT& fullRect)
        {
            fillSolidRect(deviceContext, fullRect, RGB(52, 56, 62));
            const RECT panelBounds{520, 170, 1400, 900};
            fillSolidRect(deviceContext, panelBounds, RGB(84, 89, 96));

            drawTextCommand(deviceContext,
                            PaintTextCommand{L"Paused", RECT{620, 240, 1240, 300}, 28, FW_SEMIBOLD,
                                             RGB(242, 240, 232),
                                             DT_LEFT | DT_VCENTER | DT_SINGLELINE},
                            L"Segoe UI");

            drawTextCommand(
                deviceContext,
                PaintTextCommand{selection == PauseMenuSelection::Resume ? L"> Resume" : L"Resume",
                                 RECT{620, 396, 1200, 450}, 25,
                                 selection == PauseMenuSelection::Resume ? FW_SEMIBOLD : FW_NORMAL,
                                 selection == PauseMenuSelection::Resume ? RGB(244, 214, 128)
                                                                         : RGB(232, 235, 238),
                                 DT_LEFT | DT_VCENTER | DT_SINGLELINE},
                L"Segoe UI");
            drawTextCommand(
                deviceContext,
                PaintTextCommand{
                    selection == PauseMenuSelection::Settings ? L"> Settings" : L"Settings",
                    RECT{620, 486, 1200, 540}, 25,
                    selection == PauseMenuSelection::Settings ? FW_SEMIBOLD : FW_NORMAL,
                    selection == PauseMenuSelection::Settings ? RGB(244, 214, 128)
                                                              : RGB(232, 235, 238),
                    DT_LEFT | DT_VCENTER | DT_SINGLELINE},
                L"Segoe UI");
            drawTextCommand(
                deviceContext,
                PaintTextCommand{
                    selection == PauseMenuSelection::ReturnToMainMenu ? L"> Return To Main Menu"
                                                                      : L"Return To Main Menu",
                    RECT{620, 576, 1240, 640}, 25,
                    selection == PauseMenuSelection::ReturnToMainMenu ? FW_SEMIBOLD : FW_NORMAL,
                    selection == PauseMenuSelection::ReturnToMainMenu ? RGB(244, 214, 128)
                                                                      : RGB(212, 216, 220),
                    DT_LEFT | DT_VCENTER | DT_SINGLELINE},
                L"Segoe UI");
            drawTextCommand(deviceContext,
                            PaintTextCommand{L"Save Game", RECT{620, 666, 1200, 720}, 23, FW_NORMAL,
                                             RGB(138, 144, 150),
                                             DT_LEFT | DT_VCENTER | DT_SINGLELINE},
                            L"Segoe UI");
        },
        false);
#else
    throw std::runtime_error("Pause overlay generation is only available on Windows.");
#endif
}

StartupFlowOverlay StartupFlowOverlay::createSettingsMenu(const AssetManager& assetManager,
                                                          const SettingsOverlayViewModel& viewModel)
{
#if defined(_WIN32)
    return paintWindowsOverlay(
        [viewModel](HDC deviceContext, const RECT& fullRect)
        {
            if (viewModel.pauseContext)
            {
                fillSolidRect(deviceContext, fullRect, RGB(52, 56, 62));
            }

            const RECT panelBounds{460, 170, 1460, 910};
            fillSolidRect(deviceContext, panelBounds,
                          viewModel.pauseContext ? RGB(88, 92, 98) : RGB(34, 40, 48));

            drawTextCommand(deviceContext,
                            PaintTextCommand{L"Settings", RECT{560, 232, 1300, 292}, 28,
                                             FW_SEMIBOLD, RGB(242, 240, 232),
                                             DT_LEFT | DT_VCENTER | DT_SINGLELINE},
                            L"Segoe UI");

            drawTextCommand(deviceContext,
                            PaintTextCommand{L"Changes stay pending until you click Apply.",
                                             RECT{560, 286, 1320, 320}, 15, FW_NORMAL,
                                             RGB(164, 176, 184),
                                             DT_LEFT | DT_VCENTER | DT_SINGLELINE},
                            L"Segoe UI");

            const std::wstring resolutionLabel = std::to_wstring(viewModel.resolutionWidth) +
                                                 L" x " +
                                                 std::to_wstring(viewModel.resolutionHeight);
            const std::wstring vSyncLabel = viewModel.vSyncEnabled ? L"On" : L"Off";

            drawTextCommand(deviceContext,
                            PaintTextCommand{L"Resolution", RECT{580, 360, 900, 398}, 23, FW_NORMAL,
                                             RGB(208, 214, 220),
                                             DT_LEFT | DT_VCENTER | DT_SINGLELINE},
                            L"Segoe UI");
            fillSolidRect(deviceContext, RECT{760, 404, 1260, 412},
                          (viewModel.hoverTarget == SettingsHoverTarget::ResolutionSlider ||
                           viewModel.resolutionDragging)
                              ? RGB(188, 160, 92)
                              : RGB(96, 104, 112));
            for (int stopIndex = 0; stopIndex < viewModel.resolutionCount; ++stopIndex)
            {
                const float t = viewModel.resolutionCount > 1
                                    ? static_cast<float>(stopIndex) /
                                          static_cast<float>(viewModel.resolutionCount - 1)
                                    : 0.0f;
                const int x = 760 + static_cast<int>(500.0f * t + 0.5f);
                fillSolidRect(deviceContext, RECT{x - 4, 398, x + 4, 418}, RGB(222, 228, 233));
            }
            const float knobT = viewModel.resolutionCount > 1
                                    ? static_cast<float>(viewModel.resolutionIndex) /
                                          static_cast<float>(viewModel.resolutionCount - 1)
                                    : 0.0f;
            const int knobX = 760 + static_cast<int>(500.0f * knobT + 0.5f);
            fillSolidRect(deviceContext, RECT{knobX - 14, 392, knobX + 14, 424},
                          RGB(244, 214, 128));
            drawTextCommand(deviceContext,
                            PaintTextCommand{resolutionLabel, RECT{1288, 384, 1400, 430}, 21,
                                             FW_SEMIBOLD, RGB(232, 235, 238),
                                             DT_RIGHT | DT_VCENTER | DT_SINGLELINE},
                            L"Segoe UI");

            drawTextCommand(deviceContext,
                            PaintTextCommand{L"Display Mode", RECT{580, 450, 920, 486}, 23,
                                             FW_NORMAL, RGB(208, 214, 220),
                                             DT_LEFT | DT_VCENTER | DT_SINGLELINE},
                            L"Segoe UI");
            const auto drawModeButton =
                [&](const RECT& bounds, const wchar_t* label, const bool active, const bool hovered)
            {
                fillSolidRect(deviceContext, bounds,
                              active ? RGB(110, 96, 64)
                                     : (hovered ? RGB(70, 78, 86) : RGB(54, 60, 68)));
                drawTextCommand(deviceContext,
                                PaintTextCommand{label, bounds, 16,
                                                 active ? FW_SEMIBOLD : FW_NORMAL,
                                                 active ? RGB(244, 214, 128) : RGB(222, 228, 233),
                                                 DT_CENTER | DT_VCENTER | DT_SINGLELINE},
                                L"Segoe UI");
            };
            drawModeButton(RECT{580, 490, 780, 548}, L"Windowed",
                           viewModel.windowMode == Application::WindowMode::Windowed,
                           viewModel.hoverTarget == SettingsHoverTarget::WindowedMode);
            drawModeButton(RECT{800, 490, 1080, 548}, L"Windowed Fullscreen",
                           viewModel.windowMode == Application::WindowMode::BorderlessFullscreen,
                           viewModel.hoverTarget == SettingsHoverTarget::BorderlessMode);
            drawModeButton(RECT{1100, 490, 1380, 548}, L"Exclusive Fullscreen",
                           viewModel.windowMode == Application::WindowMode::ExclusiveFullscreen,
                           viewModel.hoverTarget == SettingsHoverTarget::ExclusiveMode);

            drawTextCommand(deviceContext,
                            PaintTextCommand{L"VSync", RECT{580, 578, 900, 616}, 23, FW_NORMAL,
                                             RGB(208, 214, 220),
                                             DT_LEFT | DT_VCENTER | DT_SINGLELINE},
                            L"Segoe UI");
            fillSolidRect(deviceContext, RECT{580, 620, 760, 676},
                          viewModel.hoverTarget == SettingsHoverTarget::VSyncToggle
                              ? RGB(70, 78, 86)
                              : RGB(54, 60, 68));
            drawTextCommand(
                deviceContext,
                PaintTextCommand{vSyncLabel, RECT{580, 620, 760, 676}, 21,
                                 viewModel.vSyncEnabled ? FW_SEMIBOLD : FW_NORMAL,
                                 viewModel.vSyncEnabled ? RGB(244, 214, 128) : RGB(222, 228, 233),
                                 DT_CENTER | DT_VCENTER | DT_SINGLELINE},
                L"Segoe UI");

            fillSolidRect(deviceContext, RECT{950, 734, 1160, 794},
                          viewModel.hoverTarget == SettingsHoverTarget::BackButton
                              ? RGB(70, 78, 86)
                              : RGB(54, 60, 68));
            fillSolidRect(deviceContext, RECT{1188, 734, 1400, 794},
                          !viewModel.applyEnabled
                              ? RGB(52, 54, 58)
                              : (viewModel.hoverTarget == SettingsHoverTarget::ApplyButton
                                     ? RGB(126, 108, 70)
                                     : RGB(110, 96, 64)));
            drawTextCommand(deviceContext,
                            PaintTextCommand{L"Back", RECT{950, 734, 1160, 794}, 22, FW_NORMAL,
                                             RGB(222, 228, 233),
                                             DT_CENTER | DT_VCENTER | DT_SINGLELINE},
                            L"Segoe UI");
            drawTextCommand(
                deviceContext,
                PaintTextCommand{L"Apply", RECT{1188, 734, 1400, 794}, 22,
                                 viewModel.applyEnabled ? FW_SEMIBOLD : FW_NORMAL,
                                 viewModel.applyEnabled ? RGB(244, 214, 128) : RGB(140, 146, 152),
                                 DT_CENTER | DT_VCENTER | DT_SINGLELINE},
                L"Segoe UI");
            drawTextCommand(deviceContext,
                            PaintTextCommand{L"Use the mouse to adjust settings and click Apply.",
                                             RECT{560, 792, 1320, 836}, 16, FW_NORMAL,
                                             RGB(164, 176, 184),
                                             DT_LEFT | DT_VCENTER | DT_SINGLELINE},
                            L"Segoe UI");
        },
        !viewModel.pauseContext,
        viewModel.pauseContext ? std::filesystem::path{} : resolveMenuBackdropPath(assetManager));
#else
    throw std::runtime_error("Settings overlay generation is only available on Windows.");
#endif
}
} // namespace engine
