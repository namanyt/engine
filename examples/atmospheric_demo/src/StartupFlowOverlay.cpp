#include "StartupFlowOverlay.h"

#include "assets/AssetManager.h"
#include "core/Renderer.h"

#include "OverlayUiLayout.h"

#include <glad/glad.h>

#include <algorithm>
#include <cstring>
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

COLORREF toColorRef(const engine::BootSequenceTextEntry& entry, COLORREF fallback) noexcept
{
    if (!entry.hasColor)
    {
        return fallback;
    }

    const auto toByte = [](float value)
    { return static_cast<int>(std::clamp(value, 0.0f, 1.0f) * 255.0f + 0.5f); };

    return RGB(toByte(entry.color.r), toByte(entry.color.g), toByte(entry.color.b));
}

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

struct PixelSize final
{
    int width = 0;
    int height = 0;
};

std::wstring widen(std::string_view text)
{
    return std::wstring{text.begin(), text.end()};
}

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

std::vector<std::uint8_t> rgbaToPremultipliedBgra(const DecodedImage& image)
{
    std::vector<std::uint8_t> bgraBytes(image.rgbaBytes.size(), 0);
    for (std::size_t index = 0; index + 3 < image.rgbaBytes.size(); index += 4)
    {
        const float alpha = static_cast<float>(image.rgbaBytes[index + 3]) / 255.0f;
        bgraBytes[index + 0] = static_cast<std::uint8_t>(
            std::clamp(static_cast<float>(image.rgbaBytes[index + 2]) * alpha, 0.0f, 255.0f) +
            0.5f);
        bgraBytes[index + 1] = static_cast<std::uint8_t>(
            std::clamp(static_cast<float>(image.rgbaBytes[index + 1]) * alpha, 0.0f, 255.0f) +
            0.5f);
        bgraBytes[index + 2] = static_cast<std::uint8_t>(
            std::clamp(static_cast<float>(image.rgbaBytes[index + 0]) * alpha, 0.0f, 255.0f) +
            0.5f);
        bgraBytes[index + 3] = image.rgbaBytes[index + 3];
    }

    return bgraBytes;
}

RECT coverBounds(const RECT& destinationBounds, int sourceWidth, int sourceHeight)
{
    const int destinationWidth = destinationBounds.right - destinationBounds.left;
    const int destinationHeight = destinationBounds.bottom - destinationBounds.top;
    if (destinationWidth <= 0 || destinationHeight <= 0 || sourceWidth <= 0 || sourceHeight <= 0)
    {
        return destinationBounds;
    }

    const float scale =
        std::max(static_cast<float>(destinationWidth) / static_cast<float>(sourceWidth),
                 static_cast<float>(destinationHeight) / static_cast<float>(sourceHeight));
    const int scaledWidth = static_cast<int>(static_cast<float>(sourceWidth) * scale + 0.5f);
    const int scaledHeight = static_cast<int>(static_cast<float>(sourceHeight) * scale + 0.5f);
    const int left = destinationBounds.left + (destinationWidth - scaledWidth) / 2;
    const int top = destinationBounds.top + (destinationHeight - scaledHeight) / 2;
    return RECT{left, top, left + scaledWidth, top + scaledHeight};
}

void drawDecodedImage(HDC deviceContext, const DecodedImage& image, const RECT& destinationBounds)
{
    if (deviceContext == nullptr || image.width <= 0 || image.height <= 0)
    {
        return;
    }

    BITMAPINFO bitmapInfo{};
    bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bitmapInfo.bmiHeader.biWidth = image.width;
    bitmapInfo.bmiHeader.biHeight = -image.height;
    bitmapInfo.bmiHeader.biPlanes = 1;
    bitmapInfo.bmiHeader.biBitCount = 32;
    bitmapInfo.bmiHeader.biCompression = BI_RGB;

    void* pixelMemory = nullptr;
    HDC sourceContext = CreateCompatibleDC(deviceContext);
    if (sourceContext == nullptr)
    {
        throw std::runtime_error(
            "Failed to create a source device context for overlay image composition.");
    }

    HBITMAP bitmap =
        CreateDIBSection(sourceContext, &bitmapInfo, DIB_RGB_COLORS, &pixelMemory, nullptr, 0);
    if (bitmap == nullptr || pixelMemory == nullptr)
    {
        DeleteDC(sourceContext);
        throw std::runtime_error(
            "Failed to allocate a source bitmap for overlay image composition.");
    }

    const std::vector<std::uint8_t> bgraBytes = rgbaToPremultipliedBgra(image);
    std::memcpy(pixelMemory, bgraBytes.data(), bgraBytes.size());

    HGDIOBJ previousBitmap = SelectObject(sourceContext, bitmap);
    const BLENDFUNCTION blendFunction{AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
    const int width = destinationBounds.right - destinationBounds.left;
    const int height = destinationBounds.bottom - destinationBounds.top;
    if (!AlphaBlend(deviceContext, destinationBounds.left, destinationBounds.top, width, height,
                    sourceContext, 0, 0, image.width, image.height, blendFunction))
    {
        SelectObject(sourceContext, previousBitmap);
        DeleteObject(bitmap);
        DeleteDC(sourceContext);
        throw std::runtime_error("Failed to alpha blend an overlay image.");
    }

    SelectObject(sourceContext, previousBitmap);
    DeleteObject(bitmap);
    DeleteDC(sourceContext);
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

RECT toWinRect(const engine::overlayui::Rect& rect)
{
    return RECT{static_cast<LONG>(rect.left + 0.5f), static_cast<LONG>(rect.top + 0.5f),
                static_cast<LONG>(rect.right + 0.5f), static_cast<LONG>(rect.bottom + 0.5f)};
}

engine::overlayui::Rect offsetRect(const engine::overlayui::Rect& rect, float offsetX,
                                   float offsetY)
{
    return engine::overlayui::Rect{rect.left + offsetX, rect.top + offsetY, rect.right + offsetX,
                                   rect.bottom + offsetY};
}

engine::SettingsPanelItemViewModel
localizeSettingsItem(const engine::SettingsPanelItemViewModel& item, float offsetX, float offsetY)
{
    engine::SettingsPanelItemViewModel localized = item;
    localized.bounds = offsetRect(item.bounds, offsetX, offsetY);
    localized.interactiveBounds = offsetRect(item.interactiveBounds, offsetX, offsetY);
    for (engine::SettingsSegmentOptionViewModel& option : localized.segmentOptions)
    {
        option.bounds = offsetRect(option.bounds, offsetX, offsetY);
    }
    return localized;
}

void fillRoundedRect(HDC deviceContext, const RECT& bounds, COLORREF color, int radius)
{
    HBRUSH brush = CreateSolidBrush(color);
    HPEN pen = CreatePen(PS_NULL, 0, color);
    HGDIOBJ previousBrush = SelectObject(deviceContext, brush);
    HGDIOBJ previousPen = SelectObject(deviceContext, pen);
    RoundRect(deviceContext, bounds.left, bounds.top, bounds.right, bounds.bottom, radius, radius);
    SelectObject(deviceContext, previousPen);
    SelectObject(deviceContext, previousBrush);
    DeleteObject(pen);
    DeleteObject(brush);
}

void frameRoundedRect(HDC deviceContext, const RECT& bounds, COLORREF color, int radius)
{
    HBRUSH brush = static_cast<HBRUSH>(GetStockObject(HOLLOW_BRUSH));
    HPEN pen = CreatePen(PS_SOLID, 1, color);
    HGDIOBJ previousBrush = SelectObject(deviceContext, brush);
    HGDIOBJ previousPen = SelectObject(deviceContext, pen);
    RoundRect(deviceContext, bounds.left, bounds.top, bounds.right, bounds.bottom, radius, radius);
    SelectObject(deviceContext, previousPen);
    SelectObject(deviceContext, previousBrush);
    DeleteObject(pen);
}

bool intersects(const engine::overlayui::Rect& left, const engine::overlayui::Rect& right)
{
    return !(left.right < right.left || left.left > right.right || left.bottom < right.top ||
             left.top > right.bottom);
}

void paintSettingsSlider(HDC deviceContext, const engine::SettingsPanelItemViewModel& item,
                         const engine::SettingsHoverTarget& hoverTarget, COLORREF cardColor)
{
    fillRoundedRect(deviceContext, toWinRect(item.bounds), cardColor, 22);
    drawTextCommand(
        deviceContext,
        PaintTextCommand{
            item.label,
            toWinRect(engine::overlayui::Rect{item.bounds.left + 24.0f, item.bounds.top + 14.0f,
                                              item.bounds.left + 360.0f, item.bounds.top + 48.0f}),
            22, FW_NORMAL, RGB(224, 229, 234), DT_LEFT | DT_VCENTER | DT_SINGLELINE},
        L"Segoe UI");
    drawTextCommand(
        deviceContext,
        PaintTextCommand{
            item.valueLabel,
            toWinRect(engine::overlayui::Rect{item.bounds.right - 176.0f, item.bounds.top + 16.0f,
                                              item.bounds.right - 24.0f, item.bounds.top + 48.0f}),
            18, FW_SEMIBOLD, RGB(244, 214, 128), DT_RIGHT | DT_VCENTER | DT_SINGLELINE},
        L"Segoe UI");

    const RECT trackRect = toWinRect(item.interactiveBounds);
    fillSolidRect(deviceContext,
                  RECT{trackRect.left, trackRect.top + 10, trackRect.right, trackRect.top + 16},
                  item.active || hoverTarget == item.hoverTarget ? RGB(188, 160, 92)
                                                                 : RGB(84, 92, 102));
    const int knobX =
        trackRect.left +
        static_cast<int>((item.interactiveBounds.right - item.interactiveBounds.left) *
                             item.normalizedValue +
                         0.5f);
    fillRoundedRect(deviceContext, RECT{knobX - 14, trackRect.top, knobX + 14, trackRect.bottom},
                    RGB(244, 214, 128), 20);
}

void paintSettingsToggle(HDC deviceContext, const engine::SettingsPanelItemViewModel& item,
                         const engine::SettingsHoverTarget& hoverTarget, COLORREF cardColor)
{
    fillRoundedRect(deviceContext, toWinRect(item.bounds), cardColor, 22);
    drawTextCommand(
        deviceContext,
        PaintTextCommand{
            item.label,
            toWinRect(engine::overlayui::Rect{item.bounds.left + 24.0f, item.bounds.top + 14.0f,
                                              item.bounds.left + 360.0f, item.bounds.top + 48.0f}),
            22, FW_NORMAL, RGB(224, 229, 234), DT_LEFT | DT_VCENTER | DT_SINGLELINE},
        L"Segoe UI");

    const bool hovered = hoverTarget == item.hoverTarget;
    const RECT toggleRect = toWinRect(item.interactiveBounds);
    fillRoundedRect(deviceContext, toggleRect,
                    item.active ? RGB(110, 96, 64) : (hovered ? RGB(72, 80, 88) : RGB(54, 60, 68)),
                    20);
    drawTextCommand(deviceContext,
                    PaintTextCommand{item.valueLabel, toggleRect, 18,
                                     item.active ? FW_SEMIBOLD : FW_NORMAL,
                                     item.active ? RGB(244, 214, 128) : RGB(222, 228, 233),
                                     DT_CENTER | DT_VCENTER | DT_SINGLELINE},
                    L"Segoe UI");
}

void paintSettingsSegmented(HDC deviceContext, const engine::SettingsPanelItemViewModel& item,
                            COLORREF cardColor)
{
    fillRoundedRect(deviceContext, toWinRect(item.bounds), cardColor, 22);
    drawTextCommand(
        deviceContext,
        PaintTextCommand{
            item.label,
            toWinRect(engine::overlayui::Rect{item.bounds.left + 24.0f, item.bounds.top + 14.0f,
                                              item.bounds.left + 360.0f, item.bounds.top + 48.0f}),
            22, FW_NORMAL, RGB(224, 229, 234), DT_LEFT | DT_VCENTER | DT_SINGLELINE},
        L"Segoe UI");

    for (const engine::SettingsSegmentOptionViewModel& option : item.segmentOptions)
    {
        const RECT optionRect = toWinRect(option.bounds);
        fillRoundedRect(deviceContext, optionRect,
                        option.selected ? RGB(110, 96, 64) : RGB(54, 60, 68), 18);
        drawTextCommand(deviceContext,
                        PaintTextCommand{option.label, optionRect, 15,
                                         option.selected ? FW_SEMIBOLD : FW_NORMAL,
                                         option.selected ? RGB(244, 214, 128) : RGB(222, 228, 233),
                                         DT_CENTER | DT_VCENTER | DT_WORDBREAK},
                        L"Segoe UI");
    }
}

void paintSettingsSection(HDC deviceContext, const engine::SettingsPanelItemViewModel& item)
{
    drawTextCommand(
        deviceContext,
        PaintTextCommand{
            item.label,
            toWinRect(engine::overlayui::Rect{item.bounds.left, item.bounds.top, item.bounds.right,
                                              item.bounds.top + 30.0f}),
            24, FW_SEMIBOLD, RGB(242, 240, 232), DT_LEFT | DT_VCENTER | DT_SINGLELINE},
        L"Segoe UI");
}

void paintSettingsPlaceholder(HDC deviceContext, const engine::SettingsPanelItemViewModel& item,
                              COLORREF cardColor)
{
    fillRoundedRect(deviceContext, toWinRect(item.bounds), cardColor, 22);
    drawTextCommand(
        deviceContext,
        PaintTextCommand{
            item.label,
            toWinRect(engine::overlayui::Rect{item.bounds.left + 24.0f, item.bounds.top + 18.0f,
                                              item.bounds.left + 300.0f, item.bounds.top + 50.0f}),
            22, FW_NORMAL, RGB(224, 229, 234), DT_LEFT | DT_VCENTER | DT_SINGLELINE},
        L"Segoe UI");
    drawTextCommand(
        deviceContext,
        PaintTextCommand{
            item.valueLabel,
            toWinRect(engine::overlayui::Rect{item.bounds.right - 184.0f, item.bounds.top + 18.0f,
                                              item.bounds.right - 24.0f, item.bounds.top + 50.0f}),
            15, FW_SEMIBOLD, RGB(196, 172, 122), DT_RIGHT | DT_VCENTER | DT_SINGLELINE},
        L"Segoe UI");
}

bool shouldPaintVisualNovelDialogueChrome(const engine::VisualNovelOverlayModel& viewModel)
{
    return viewModel.showDialogueChrome || !viewModel.speakerName.empty() ||
           !viewModel.dialogueText.empty() || !viewModel.advancePrompt.empty();
}

void paintVisualNovelDialogueChrome(HDC deviceContext)
{
    const RECT dialoguePanel{120, 730, 1800, 1000};
    const RECT namePlate{160, 676, 600, 740};
    const RECT accentLine{160, 968, 1760, 974};
    fillSolidRect(deviceContext, dialoguePanel, RGB(20, 20, 24));
    fillSolidRect(deviceContext, namePlate, RGB(70, 52, 34));
    fillSolidRect(deviceContext, accentLine, RGB(178, 144, 94));
}

void paintVisualNovelDialogueText(HDC deviceContext,
                                  const engine::VisualNovelOverlayModel& viewModel)
{
    if (!viewModel.speakerName.empty())
    {
        drawTextCommand(deviceContext,
                        PaintTextCommand{widen(viewModel.speakerName), RECT{186, 688, 570, 730}, 20,
                                         FW_SEMIBOLD, RGB(246, 238, 228),
                                         DT_LEFT | DT_VCENTER | DT_SINGLELINE},
                        L"Segoe UI");
    }

    if (!viewModel.dialogueText.empty())
    {
        drawTextCommand(deviceContext,
                        PaintTextCommand{widen(viewModel.dialogueText), RECT{170, 784, 1740, 942},
                                         24, FW_NORMAL, RGB(244, 244, 242),
                                         DT_LEFT | DT_TOP | DT_WORDBREAK},
                        L"Segoe UI");
    }

    if (!viewModel.advancePrompt.empty())
    {
        drawTextCommand(deviceContext,
                        PaintTextCommand{widen(viewModel.advancePrompt), RECT{1320, 942, 1740, 980},
                                         16, FW_NORMAL, RGB(186, 186, 190),
                                         DT_RIGHT | DT_VCENTER | DT_SINGLELINE},
                        L"Segoe UI");
    }
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

PixelSize measureTextCommand(const PaintTextCommand& command, const wchar_t* faceName)
{
    HDC deviceContext = CreateCompatibleDC(nullptr);
    if (deviceContext == nullptr)
    {
        throw std::runtime_error("Failed to create a device context for text measurement.");
    }

    FontScope font(CreateFontW(pointSizeToPixels(command.pointSize), 0, 0, 0, command.weight, FALSE,
                               FALSE, FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS,
                               CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
                               VARIABLE_PITCH | FF_DONTCARE, faceName));
    if (font.handle == nullptr)
    {
        DeleteDC(deviceContext);
        throw std::runtime_error("Failed to create a startup flow overlay font.");
    }

    HGDIOBJ previousFont = SelectObject(deviceContext, font.handle);
    RECT measuredBounds{};
    DrawTextW(deviceContext, command.text.c_str(), -1, &measuredBounds,
              command.format | DT_CALCRECT);
    SelectObject(deviceContext, previousFont);
    DeleteDC(deviceContext);

    const int measuredWidth = static_cast<int>(measuredBounds.right - measuredBounds.left);
    const int measuredHeight = static_cast<int>(measuredBounds.bottom - measuredBounds.top);
    return PixelSize{std::max(measuredWidth, 1), std::max(measuredHeight, 1)};
}

engine::StartupFlowOverlay
paintWindowsTexture(int width, int height,
                    const std::function<void(HDC, const RECT&)>& paintCallback,
                    bool opaqueBackground)
{
    BITMAPINFO bitmapInfo{};
    bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bitmapInfo.bmiHeader.biWidth = width;
    bitmapInfo.bmiHeader.biHeight = -height;
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
    RECT fullRect{0, 0, width, height};
    HBRUSH clearBrush = CreateSolidBrush(RGB(0, 0, 0));
    FillRect(deviceContext, &fullRect, clearBrush);
    DeleteObject(clearBrush);

    paintCallback(deviceContext, fullRect);

    const auto* bgraBytes = static_cast<const std::uint8_t*>(pixelMemory);
    std::vector<std::uint8_t> rgbaPixels(static_cast<std::size_t>(width) *
                                         static_cast<std::size_t>(height) * 4u);
    for (int index = 0; index < width * height; ++index)
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
    return engine::StartupFlowOverlay(uploadTexture(rgbaPixels, width, height), width, height);
}

engine::StartupFlowOverlay
paintWindowsTextureBinaryAlpha(int width, int height,
                               const std::function<void(HDC, const RECT&)>& paintCallback)
{
    BITMAPINFO bitmapInfo{};
    bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bitmapInfo.bmiHeader.biWidth = width;
    bitmapInfo.bmiHeader.biHeight = -height;
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
    RECT fullRect{0, 0, width, height};
    HBRUSH clearBrush = CreateSolidBrush(RGB(0, 0, 0));
    FillRect(deviceContext, &fullRect, clearBrush);
    DeleteObject(clearBrush);

    paintCallback(deviceContext, fullRect);

    const auto* bgraBytes = static_cast<const std::uint8_t*>(pixelMemory);
    std::vector<std::uint8_t> rgbaPixels(static_cast<std::size_t>(width) *
                                         static_cast<std::size_t>(height) * 4u);
    for (int index = 0; index < width * height; ++index)
    {
        const std::uint8_t blue = bgraBytes[index * 4 + 0];
        const std::uint8_t green = bgraBytes[index * 4 + 1];
        const std::uint8_t red = bgraBytes[index * 4 + 2];
        const std::uint8_t alpha = (red != 0 || green != 0 || blue != 0) ? 255 : 0;
        rgbaPixels[index * 4 + 0] = red;
        rgbaPixels[index * 4 + 1] = green;
        rgbaPixels[index * 4 + 2] = blue;
        rgbaPixels[index * 4 + 3] = alpha;
    }

    SelectObject(deviceContext, previousBitmap);
    DeleteObject(bitmap);
    DeleteDC(deviceContext);
    return engine::StartupFlowOverlay(uploadTexture(rgbaPixels, width, height), width, height);
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

StartupFlowOverlay StartupFlowOverlay::createLoadingProgress(const std::string& loadingLabel,
                                                             int percent, const std::string& phase)
{
#if defined(_WIN32)
    const std::wstring title = std::wstring{loadingLabel.begin(), loadingLabel.end()};
    const std::wstring phaseText = std::wstring{phase.begin(), phase.end()};
    const std::wstring percentText = std::to_wstring(std::clamp(percent, 0, 100)) + L"%";

    return paintWindowsOverlay(
        [title, phaseText, percentText](HDC deviceContext, const RECT& fullRect)
        {
            const RECT loadingBounds{fullRect.left + 360, fullRect.top + 300, fullRect.right - 360,
                                     fullRect.top + 360};
            drawTextCommand(deviceContext,
                            PaintTextCommand{L"Loading " + title, loadingBounds, 22, FW_NORMAL,
                                             RGB(208, 214, 220),
                                             DT_CENTER | DT_VCENTER | DT_SINGLELINE},
                            L"Segoe UI");

            const RECT percentBounds{fullRect.left + 360, fullRect.top + 430, fullRect.right - 360,
                                     fullRect.top + 620};
            drawTextCommand(deviceContext,
                            PaintTextCommand{percentText, percentBounds, 56, FW_SEMIBOLD,
                                             RGB(244, 244, 244),
                                             DT_CENTER | DT_VCENTER | DT_SINGLELINE},
                            L"Segoe UI");

            if (!phaseText.empty())
            {
                const RECT phaseBounds{fullRect.left + 360, fullRect.top + 660,
                                       fullRect.right - 360, fullRect.top + 720};
                drawTextCommand(deviceContext,
                                PaintTextCommand{phaseText, phaseBounds, 18, FW_NORMAL,
                                                 RGB(154, 164, 174),
                                                 DT_CENTER | DT_VCENTER | DT_SINGLELINE},
                                L"Segoe UI");
            }
        },
        false);
#else
    (void)loadingLabel;
    (void)percent;
    (void)phase;
    throw std::runtime_error("Startup loading overlay generation is only available on Windows.");
#endif
}

StartupFlowOverlay
StartupFlowOverlay::createBootSequence(const std::vector<BootSequenceTextEntry>& lines,
                                       bool showCursor, const BootSequenceTextEntry& statusText)
{
#if defined(_WIN32)
    constexpr int kTerminalPaddingLeft = 88;
    constexpr int kTerminalPaddingRight = 88;
    constexpr int kTerminalPaddingTop = 104;
    constexpr int kTerminalPaddingBottom = 104;
    constexpr int kTerminalLineHeight = 28;
    constexpr int kTerminalLineStep = 34;
    constexpr int kTerminalFooterGap = 20;

    std::vector<std::pair<std::wstring, COLORREF>> lineTexts;
    lineTexts.reserve(lines.size());
    for (std::size_t index = 0; index < lines.size(); ++index)
    {
        const BootSequenceTextEntry& line = lines[index];
        const COLORREF fallback = index == 0 ? RGB(214, 214, 214) : RGB(176, 176, 176);
        lineTexts.emplace_back(std::wstring{line.text.begin(), line.text.end()},
                               toColorRef(line, fallback));
    }

    const std::wstring statusLine = std::wstring{statusText.text.begin(), statusText.text.end()};
    const COLORREF statusColor = toColorRef(statusText, RGB(214, 214, 214));

    return paintWindowsOverlay(
        [lineTexts = std::move(lineTexts), showCursor, statusLine,
         statusColor](HDC deviceContext, const RECT& fullRect)
        {
            const int left = fullRect.left + kTerminalPaddingLeft;
            const int right = fullRect.right - kTerminalPaddingRight;
            const int top = fullRect.top + kTerminalPaddingTop;
            int footerTop = fullRect.bottom - kTerminalPaddingBottom;

            if (showCursor)
            {
                footerTop -= kTerminalLineHeight;
            }

            if (!statusLine.empty())
            {
                footerTop -= kTerminalFooterGap;
                footerTop -= kTerminalLineHeight;
            }

            const int availableLogHeight = std::max(0, footerTop - top);
            const int maxVisibleLines = std::max(1, availableLogHeight / kTerminalLineStep);
            const std::size_t startIndex =
                lineTexts.size() > static_cast<std::size_t>(maxVisibleLines)
                    ? lineTexts.size() - static_cast<std::size_t>(maxVisibleLines)
                    : 0u;

            int lineTop = top;
            for (std::size_t index = startIndex; index < lineTexts.size(); ++index)
            {
                const RECT lineBounds{left, lineTop, right, lineTop + kTerminalLineHeight};
                drawTextCommand(deviceContext,
                                PaintTextCommand{lineTexts[index].first, lineBounds, 16, FW_NORMAL,
                                                 lineTexts[index].second,
                                                 DT_LEFT | DT_VCENTER | DT_SINGLELINE},
                                L"Consolas");
                lineTop += kTerminalLineStep;
            }

            if (!statusLine.empty())
            {
                const int statusTop =
                    fullRect.bottom - kTerminalPaddingBottom -
                    (showCursor ? (kTerminalLineHeight + kTerminalFooterGap + kTerminalLineHeight)
                                : kTerminalLineHeight);
                const RECT statusBounds{left, statusTop, right, statusTop + kTerminalLineHeight};
                drawTextCommand(deviceContext,
                                PaintTextCommand{statusLine, statusBounds, 16, FW_NORMAL,
                                                 statusColor, DT_LEFT | DT_VCENTER | DT_SINGLELINE},
                                L"Consolas");
            }

            if (showCursor)
            {
                const int cursorTop =
                    fullRect.bottom - kTerminalPaddingBottom - kTerminalLineHeight;
                const RECT cursorBounds{left, cursorTop, right, cursorTop + kTerminalLineHeight};
                drawTextCommand(deviceContext,
                                PaintTextCommand{L"_", cursorBounds, 16, FW_NORMAL,
                                                 RGB(224, 224, 224),
                                                 DT_LEFT | DT_VCENTER | DT_SINGLELINE},
                                L"Consolas");
            }
        },
        false);
#else
    (void)lines;
    (void)showCursor;
    (void)statusText;
    throw std::runtime_error("Startup boot overlay generation is only available on Windows.");
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

StartupFlowOverlay StartupFlowOverlay::createInteractionPromptTexture(const std::string& promptText)
{
#if defined(_WIN32)
    const std::wstring prompt = std::wstring{promptText.begin(), promptText.end()};
    const PaintTextCommand textCommand{prompt,
                                       RECT{},
                                       18,
                                       FW_SEMIBOLD,
                                       RGB(244, 240, 232),
                                       DT_CENTER | DT_VCENTER | DT_SINGLELINE};
    const PixelSize textSize = measureTextCommand(textCommand, L"Segoe UI");
    const int textureWidth = std::max(284, textSize.width + 36);
    const int textureHeight = std::max(54, textSize.height + 19);

    return paintWindowsTexture(
        textureWidth, textureHeight,
        [prompt, textureWidth, textureHeight](HDC deviceContext, const RECT&)
        {
            const RECT backPlate{0, 0, textureWidth, textureHeight};
            const RECT accentLine{22, textureHeight - 3, textureWidth - 22, textureHeight};
            const RECT textBounds{18, 8, textureWidth - 18, textureHeight - 11};

            fillSolidRect(deviceContext, backPlate, RGB(86, 78, 68));
            fillSolidRect(deviceContext, accentLine, RGB(176, 154, 114));
            drawTextCommand(deviceContext,
                            PaintTextCommand{prompt, textBounds, 18, FW_SEMIBOLD,
                                             RGB(244, 240, 232),
                                             DT_CENTER | DT_VCENTER | DT_SINGLELINE},
                            L"Segoe UI");
        },
        false);
#else
    (void)promptText;
    throw std::runtime_error("Interaction prompt overlay generation is only available on Windows.");
#endif
}

StartupFlowOverlay StartupFlowOverlay::createSettingsMenu(const AssetManager& assetManager,
                                                          const SettingsOverlayViewModel& viewModel)
{
#if defined(_WIN32)
    return paintWindowsOverlay(
        [viewModel](HDC deviceContext, const RECT& fullRect)
        {
            const SettingsPageModel page = buildSettingsPageModel(viewModel);
            if (viewModel.pauseContext)
            {
                fillSolidRect(deviceContext, fullRect, RGB(28, 32, 36));
            }
            else
            {
                fillSolidRect(deviceContext, fullRect, RGB(18, 22, 28));
            }

            fillRoundedRect(deviceContext, toWinRect(page.chrome.shellBounds),
                            viewModel.pauseContext ? RGB(40, 46, 52) : RGB(28, 34, 42), 42);
            frameRoundedRect(deviceContext, toWinRect(page.chrome.shellBounds), RGB(84, 92, 100),
                             42);

            fillRoundedRect(deviceContext, toWinRect(page.chrome.sidebarBounds),
                            viewModel.pauseContext ? RGB(52, 58, 64) : RGB(34, 40, 48), 30);
            fillRoundedRect(deviceContext, toWinRect(page.chrome.contentBounds),
                            viewModel.pauseContext ? RGB(48, 54, 60) : RGB(30, 36, 44), 30);

            drawTextCommand(deviceContext,
                            PaintTextCommand{L"Settings", RECT{146, 118, 420, 168}, 30, FW_SEMIBOLD,
                                             RGB(242, 240, 232),
                                             DT_LEFT | DT_VCENTER | DT_SINGLELINE},
                            L"Segoe UI");

            for (const SettingsCategoryEntryViewModel& category : page.categories)
            {
                const RECT categoryRect = toWinRect(category.bounds);
                const COLORREF categoryColor =
                    category.active ? RGB(96, 86, 62)
                                    : (category.hovered ? RGB(60, 68, 76) : RGB(42, 48, 56));
                fillRoundedRect(deviceContext, categoryRect, categoryColor, 24);
                drawTextCommand(
                    deviceContext,
                    PaintTextCommand{category.title,
                                     RECT{categoryRect.left + 24, categoryRect.top + 16,
                                          categoryRect.right - 24, categoryRect.top + 44},
                                     20, category.active ? FW_SEMIBOLD : FW_NORMAL,
                                     category.active ? RGB(244, 214, 128) : RGB(224, 229, 234),
                                     DT_LEFT | DT_VCENTER | DT_SINGLELINE},
                    L"Segoe UI");
            }

            drawTextCommand(deviceContext,
                            PaintTextCommand{page.activeCategoryTitle, RECT{590, 120, 1300, 172},
                                             32, FW_SEMIBOLD, RGB(242, 240, 232),
                                             DT_LEFT | DT_VCENTER | DT_SINGLELINE},
                            L"Segoe UI");

            const COLORREF cardColor = viewModel.pauseContext ? RGB(58, 64, 70) : RGB(42, 48, 56);
            const int savedViewportState = SaveDC(deviceContext);
            const RECT viewportRect = toWinRect(page.chrome.contentViewportBounds);
            IntersectClipRect(deviceContext, viewportRect.left, viewportRect.top,
                              viewportRect.right, viewportRect.bottom);
            for (const SettingsPanelItemViewModel& item : page.items)
            {
                if (!intersects(item.bounds, page.chrome.contentViewportBounds))
                {
                    continue;
                }

                switch (item.kind)
                {
                case SettingsPanelItemKind::Section:
                    paintSettingsSection(deviceContext, item);
                    break;
                case SettingsPanelItemKind::Slider:
                    paintSettingsSlider(deviceContext, item, viewModel.hoverTarget, cardColor);
                    break;
                case SettingsPanelItemKind::Toggle:
                    paintSettingsToggle(deviceContext, item, viewModel.hoverTarget, cardColor);
                    break;
                case SettingsPanelItemKind::Segmented:
                    paintSettingsSegmented(deviceContext, item, cardColor);
                    break;
                case SettingsPanelItemKind::Placeholder:
                    paintSettingsPlaceholder(deviceContext, item, cardColor);
                    break;
                }
            }
            RestoreDC(deviceContext, savedViewportState);

            fillRoundedRect(deviceContext, toWinRect(page.chrome.scrollTrackBounds),
                            RGB(52, 58, 64), 12);
            fillRoundedRect(deviceContext, toWinRect(page.chrome.scrollThumbBounds),
                            RGB(108, 116, 124), 12);

            fillRoundedRect(deviceContext, toWinRect(page.chrome.backButtonBounds),
                            viewModel.hoverTarget.type == SettingsHoverTargetType::BackButton
                                ? RGB(70, 78, 86)
                                : RGB(54, 60, 68),
                            22);
            fillRoundedRect(deviceContext, toWinRect(page.chrome.applyButtonBounds),
                            !viewModel.applyEnabled ? RGB(52, 54, 58)
                                                    : (viewModel.hoverTarget.type ==
                                                               SettingsHoverTargetType::ApplyButton
                                                           ? RGB(126, 108, 70)
                                                           : RGB(110, 96, 64)),
                            22);
            drawTextCommand(deviceContext,
                            PaintTextCommand{L"Back", toWinRect(page.chrome.backButtonBounds), 22,
                                             FW_NORMAL, RGB(222, 228, 233),
                                             DT_CENTER | DT_VCENTER | DT_SINGLELINE},
                            L"Segoe UI");
            drawTextCommand(
                deviceContext,
                PaintTextCommand{L"Apply", toWinRect(page.chrome.applyButtonBounds), 22,
                                 viewModel.applyEnabled ? FW_SEMIBOLD : FW_NORMAL,
                                 viewModel.applyEnabled ? RGB(244, 214, 128) : RGB(140, 146, 152),
                                 DT_CENTER | DT_VCENTER | DT_SINGLELINE},
                L"Segoe UI");
        },
        !viewModel.pauseContext,
        viewModel.pauseContext ? std::filesystem::path{} : resolveMenuBackdropPath(assetManager));
#else
    throw std::runtime_error("Settings overlay generation is only available on Windows.");
#endif
}

StartupFlowOverlay StartupFlowOverlay::createSettingsBase(const AssetManager& assetManager,
                                                          const SettingsOverlayViewModel& viewModel)
{
#if defined(_WIN32)
    return paintWindowsOverlay(
        [viewModel](HDC deviceContext, const RECT& fullRect)
        {
            const SettingsPageModel page = buildSettingsPageModel(viewModel);
            if (viewModel.pauseContext)
            {
                fillSolidRect(deviceContext, fullRect, RGB(28, 32, 36));
            }
            else
            {
                fillSolidRect(deviceContext, fullRect, RGB(18, 22, 28));
            }

            fillRoundedRect(deviceContext, toWinRect(page.chrome.shellBounds),
                            viewModel.pauseContext ? RGB(40, 46, 52) : RGB(28, 34, 42), 42);
            frameRoundedRect(deviceContext, toWinRect(page.chrome.shellBounds), RGB(84, 92, 100),
                             42);

            fillRoundedRect(deviceContext, toWinRect(page.chrome.sidebarBounds),
                            viewModel.pauseContext ? RGB(52, 58, 64) : RGB(34, 40, 48), 30);
            fillRoundedRect(deviceContext, toWinRect(page.chrome.contentBounds),
                            viewModel.pauseContext ? RGB(48, 54, 60) : RGB(30, 36, 44), 30);

            drawTextCommand(deviceContext,
                            PaintTextCommand{L"Settings", RECT{146, 118, 420, 168}, 30, FW_SEMIBOLD,
                                             RGB(242, 240, 232),
                                             DT_LEFT | DT_VCENTER | DT_SINGLELINE},
                            L"Segoe UI");

            for (const SettingsCategoryEntryViewModel& category : page.categories)
            {
                const RECT categoryRect = toWinRect(category.bounds);
                const COLORREF categoryColor =
                    category.active ? RGB(96, 86, 62)
                                    : (category.hovered ? RGB(60, 68, 76) : RGB(42, 48, 56));
                fillRoundedRect(deviceContext, categoryRect, categoryColor, 24);
                drawTextCommand(
                    deviceContext,
                    PaintTextCommand{category.title,
                                     RECT{categoryRect.left + 24, categoryRect.top + 16,
                                          categoryRect.right - 24, categoryRect.top + 44},
                                     20, category.active ? FW_SEMIBOLD : FW_NORMAL,
                                     category.active ? RGB(244, 214, 128) : RGB(224, 229, 234),
                                     DT_LEFT | DT_VCENTER | DT_SINGLELINE},
                    L"Segoe UI");
            }
        },
        !viewModel.pauseContext,
        viewModel.pauseContext ? std::filesystem::path{} : resolveMenuBackdropPath(assetManager));
#else
    (void)assetManager;
    (void)viewModel;
    throw std::runtime_error("Settings overlay generation is only available on Windows.");
#endif
}

StartupFlowOverlay
StartupFlowOverlay::createSettingsContent(const SettingsOverlayViewModel& viewModel)
{
#if defined(_WIN32)
    const SettingsPageModel page = buildSettingsPageModel(viewModel);
    const int textureWidth =
        static_cast<int>(page.chrome.contentBounds.right - page.chrome.contentBounds.left + 0.5f);
    const int textureHeight =
        static_cast<int>(page.chrome.contentBounds.bottom - page.chrome.contentBounds.top + 0.5f);
    const float offsetX = -page.chrome.contentBounds.left;
    const float offsetY = -page.chrome.contentBounds.top;

    return paintWindowsTextureBinaryAlpha(
        textureWidth, textureHeight,
        [viewModel, page, offsetX, offsetY](HDC deviceContext, const RECT& fullRect)
        {
            (void)fullRect;
            const COLORREF cardColor = viewModel.pauseContext ? RGB(58, 64, 70) : RGB(42, 48, 56);

            drawTextCommand(
                deviceContext,
                PaintTextCommand{
                    page.activeCategoryTitle,
                    toWinRect(offsetRect(engine::overlayui::Rect{590.0f, 120.0f, 1300.0f, 172.0f},
                                         offsetX, offsetY)),
                    32, FW_SEMIBOLD, RGB(242, 240, 232), DT_LEFT | DT_VCENTER | DT_SINGLELINE},
                L"Segoe UI");

            const int savedViewportState = SaveDC(deviceContext);
            const engine::overlayui::Rect localizedViewportBounds =
                offsetRect(page.chrome.contentViewportBounds, offsetX, offsetY);
            const RECT viewportRect = toWinRect(localizedViewportBounds);
            IntersectClipRect(deviceContext, viewportRect.left, viewportRect.top,
                              viewportRect.right, viewportRect.bottom);
            for (const SettingsPanelItemViewModel& item : page.items)
            {
                if (!intersects(item.bounds, page.chrome.contentViewportBounds))
                {
                    continue;
                }

                const SettingsPanelItemViewModel localItem =
                    localizeSettingsItem(item, offsetX, offsetY);
                switch (localItem.kind)
                {
                case SettingsPanelItemKind::Section:
                    paintSettingsSection(deviceContext, localItem);
                    break;
                case SettingsPanelItemKind::Slider:
                    paintSettingsSlider(deviceContext, localItem, viewModel.hoverTarget, cardColor);
                    break;
                case SettingsPanelItemKind::Toggle:
                    paintSettingsToggle(deviceContext, localItem, viewModel.hoverTarget, cardColor);
                    break;
                case SettingsPanelItemKind::Segmented:
                    paintSettingsSegmented(deviceContext, localItem, cardColor);
                    break;
                case SettingsPanelItemKind::Placeholder:
                    paintSettingsPlaceholder(deviceContext, localItem, cardColor);
                    break;
                }
            }
            RestoreDC(deviceContext, savedViewportState);

            fillRoundedRect(deviceContext,
                            toWinRect(offsetRect(page.chrome.scrollTrackBounds, offsetX, offsetY)),
                            RGB(52, 58, 64), 12);
            fillRoundedRect(deviceContext,
                            toWinRect(offsetRect(page.chrome.scrollThumbBounds, offsetX, offsetY)),
                            RGB(108, 116, 124), 12);

            const engine::overlayui::Rect backButtonBounds =
                offsetRect(page.chrome.backButtonBounds, offsetX, offsetY);
            const engine::overlayui::Rect applyButtonBounds =
                offsetRect(page.chrome.applyButtonBounds, offsetX, offsetY);
            fillRoundedRect(deviceContext, toWinRect(backButtonBounds),
                            viewModel.hoverTarget.type == SettingsHoverTargetType::BackButton
                                ? RGB(70, 78, 86)
                                : RGB(54, 60, 68),
                            22);
            fillRoundedRect(deviceContext, toWinRect(applyButtonBounds),
                            !viewModel.applyEnabled ? RGB(52, 54, 58)
                                                    : (viewModel.hoverTarget.type ==
                                                               SettingsHoverTargetType::ApplyButton
                                                           ? RGB(126, 108, 70)
                                                           : RGB(110, 96, 64)),
                            22);
            drawTextCommand(deviceContext,
                            PaintTextCommand{L"Back", toWinRect(backButtonBounds), 22, FW_NORMAL,
                                             RGB(222, 228, 233),
                                             DT_CENTER | DT_VCENTER | DT_SINGLELINE},
                            L"Segoe UI");
            drawTextCommand(
                deviceContext,
                PaintTextCommand{L"Apply", toWinRect(applyButtonBounds), 22,
                                 viewModel.applyEnabled ? FW_SEMIBOLD : FW_NORMAL,
                                 viewModel.applyEnabled ? RGB(244, 214, 128) : RGB(140, 146, 152),
                                 DT_CENTER | DT_VCENTER | DT_SINGLELINE},
                L"Segoe UI");
        });
#else
    (void)viewModel;
    throw std::runtime_error("Settings overlay generation is only available on Windows.");
#endif
}

StartupFlowOverlay
StartupFlowOverlay::createVisualNovelScene(const AssetManager& assetManager,
                                           const VisualNovelOverlayModel& viewModel)
{
#if defined(_WIN32)
    return paintWindowsTexture(
        kOverlayWidth, kOverlayHeight,
        [&assetManager, &viewModel](HDC deviceContext, const RECT& fullRect)
        {
            fillSolidRect(deviceContext, fullRect, RGB(8, 8, 10));

            if (!viewModel.backgroundAssetPath.empty())
            {
                const std::filesystem::path backgroundPath =
                    assetManager.resolveAssetPath(viewModel.backgroundAssetPath);
                const DecodedImage backgroundImage = decodeWithWic(backgroundPath);
                drawDecodedImage(
                    deviceContext, backgroundImage,
                    coverBounds(fullRect, backgroundImage.width, backgroundImage.height));
            }

            for (const VisualNovelOverlayPortrait& portrait : viewModel.portraits)
            {
                if (portrait.assetPath.empty())
                {
                    continue;
                }

                const std::filesystem::path portraitPath =
                    assetManager.resolveAssetPath(portrait.assetPath);
                const DecodedImage portraitImage = decodeWithWic(portraitPath);
                const float portraitScale = std::max(portrait.nativeScale, 0.0f);
                const float portraitWidth = static_cast<float>(portraitImage.width) * portraitScale;
                const float portraitHeight =
                    static_cast<float>(portraitImage.height) * portraitScale;
                const float centerX = std::clamp(portrait.centerXNormalized, -1.0f, 2.0f) *
                                      static_cast<float>(kOverlayWidth);
                const float baselineY = std::clamp(portrait.baselineYNormalized, -1.0f, 2.0f) *
                                        static_cast<float>(kOverlayHeight);
                const RECT portraitBounds{static_cast<int>(centerX - portraitWidth * 0.5f + 0.5f),
                                          static_cast<int>(baselineY - portraitHeight + 0.5f),
                                          static_cast<int>(centerX + portraitWidth * 0.5f + 0.5f),
                                          static_cast<int>(baselineY + 0.5f)};
                drawDecodedImage(deviceContext, portraitImage, portraitBounds);
            }

            if (shouldPaintVisualNovelDialogueChrome(viewModel))
            {
                paintVisualNovelDialogueChrome(deviceContext);
                paintVisualNovelDialogueText(deviceContext, viewModel);
            }
        },
        true);
#else
    (void)assetManager;
    (void)viewModel;
    throw std::runtime_error("Visual novel overlay generation is only available on Windows.");
#endif
}

StartupFlowOverlay
StartupFlowOverlay::createVisualNovelDialogueLayer(const VisualNovelOverlayModel& viewModel)
{
#if defined(_WIN32)
    return paintWindowsOverlay(
        [&viewModel](HDC deviceContext, const RECT& fullRect)
        {
            (void)fullRect;
            paintVisualNovelDialogueText(deviceContext, viewModel);
        },
        false);
#else
    (void)viewModel;
    throw std::runtime_error("Visual novel overlay generation is only available on Windows.");
#endif
}
} // namespace engine
