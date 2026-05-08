#include "assets/TextureAsset.h"

#include "core/RenderDebug.h"

#include <glad/glad.h>

#include <array>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wincodec.h>
#endif

namespace engine
{
namespace
{
struct DecodedTexture final
{
    int width = 0;
    int height = 0;
    int channelCount = 0;
    bool hdr = false;
    std::vector<std::uint8_t> bytes;
    std::vector<float> floats;
};

#if defined(_WIN32)
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
            throw std::runtime_error("Failed to initialize COM for texture decoding.");
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

void throwIfFailed(HRESULT result, const std::string& message)
{
    if (FAILED(result))
    {
        std::ostringstream stream;
        stream << message << " (HRESULT=0x" << std::hex << static_cast<unsigned long>(result)
               << ")";
        throw std::runtime_error(stream.str());
    }
}

DecodedTexture decodeWithWic(const std::filesystem::path& path)
{
    ScopedCom com;

    IWICImagingFactory* factoryRaw = nullptr;
    throwIfFailed(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                                   IID_IWICImagingFactory, reinterpret_cast<void**>(&factoryRaw)),
                  "Failed to create the WIC imaging factory.");
    ComPtr<IWICImagingFactory> factory(factoryRaw);

    IWICBitmapDecoder* decoderRaw = nullptr;
    throwIfFailed(factory->CreateDecoderFromFilename(path.c_str(), nullptr, GENERIC_READ,
                                                     WICDecodeMetadataCacheOnLoad, &decoderRaw),
                  "Failed to open texture file with WIC: " + path.string());
    ComPtr<IWICBitmapDecoder> decoder(decoderRaw);

    IWICBitmapFrameDecode* frameRaw = nullptr;
    throwIfFailed(decoder->GetFrame(0, &frameRaw),
                  "Failed to read the first texture frame: " + path.string());
    ComPtr<IWICBitmapFrameDecode> frame(frameRaw);

    UINT width = 0;
    UINT height = 0;
    throwIfFailed(frame->GetSize(&width, &height),
                  "Failed to query texture dimensions: " + path.string());

    IWICFormatConverter* converterRaw = nullptr;
    throwIfFailed(factory->CreateFormatConverter(&converterRaw),
                  "Failed to create a WIC format converter.");
    ComPtr<IWICFormatConverter> converter(converterRaw);

    throwIfFailed(converter->Initialize(frame.get(), GUID_WICPixelFormat32bppRGBA,
                                        WICBitmapDitherTypeNone, nullptr, 0.0,
                                        WICBitmapPaletteTypeCustom),
                  "Failed to convert texture pixels to RGBA8: " + path.string());

    DecodedTexture texture{};
    texture.width = static_cast<int>(width);
    texture.height = static_cast<int>(height);
    texture.channelCount = 4;
    texture.bytes.resize(static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4U);

    throwIfFailed(converter->CopyPixels(nullptr, width * 4U,
                                        static_cast<UINT>(texture.bytes.size()),
                                        texture.bytes.data()),
                  "Failed to copy texture pixels: " + path.string());

    return texture;
}
#endif

void parseHdrResolution(const std::string& line, int& width, int& height)
{
    std::stringstream stream(line);
    std::string yAxis;
    std::string xAxis;
    stream >> yAxis >> height >> xAxis >> width;

    if (width <= 0 || height <= 0)
    {
        throw std::runtime_error("Invalid HDR resolution line: " + line);
    }
}

void rgbeToFloat(std::uint8_t red, std::uint8_t green, std::uint8_t blue, std::uint8_t exponent,
                 float& outRed, float& outGreen, float& outBlue)
{
    if (exponent == 0)
    {
        outRed = 0.0f;
        outGreen = 0.0f;
        outBlue = 0.0f;
        return;
    }

    const float scale = std::ldexp(1.0f, static_cast<int>(exponent) - (128 + 8));
    outRed = static_cast<float>(red) * scale;
    outGreen = static_cast<float>(green) * scale;
    outBlue = static_cast<float>(blue) * scale;
}

DecodedTexture decodeHdr(const std::filesystem::path& path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open HDR texture file: " + path.string());
    }

    std::string line;
    bool hasFormat = false;

    while (std::getline(file, line))
    {
        if (line.empty())
        {
            break;
        }

        if (line.find("FORMAT=32-bit_rle_rgbe") != std::string::npos)
        {
            hasFormat = true;
        }
    }

    if (!hasFormat)
    {
        throw std::runtime_error("Unsupported HDR texture format: " + path.string());
    }

    if (!std::getline(file, line))
    {
        throw std::runtime_error("Missing HDR resolution line: " + path.string());
    }

    int width = 0;
    int height = 0;
    parseHdrResolution(line, width, height);

    DecodedTexture texture{};
    texture.width = width;
    texture.height = height;
    texture.channelCount = 3;
    texture.hdr = true;
    texture.floats.resize(static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 3U);

    std::vector<std::uint8_t> scanline(static_cast<std::size_t>(width) * 4U);
    std::array<std::uint8_t, 4> header{};

    for (int y = 0; y < height; ++y)
    {
        file.read(reinterpret_cast<char*>(header.data()), 4);
        if (!file)
        {
            throw std::runtime_error("Unexpected end of HDR texture data: " + path.string());
        }

        if (header[0] != 2 || header[1] != 2 || (header[2] & 0x80U) != 0 ||
            ((static_cast<int>(header[2]) << 8) | static_cast<int>(header[3])) != width)
        {
            throw std::runtime_error(
                "Unsupported non-RLE HDR texture encoding. Use standard Radiance RLE files.");
        }

        for (int channel = 0; channel < 4; ++channel)
        {
            int x = 0;
            while (x < width)
            {
                std::uint8_t count = 0;
                file.read(reinterpret_cast<char*>(&count), 1);
                if (!file)
                {
                    throw std::runtime_error("Unexpected end of HDR scanline data: " +
                                             path.string());
                }

                if (count > 128)
                {
                    const int runLength = static_cast<int>(count) - 128;
                    std::uint8_t value = 0;
                    file.read(reinterpret_cast<char*>(&value), 1);
                    if (!file)
                    {
                        throw std::runtime_error("Unexpected end of HDR RLE run: " + path.string());
                    }

                    for (int repeat = 0; repeat < runLength; ++repeat)
                    {
                        scanline[static_cast<std::size_t>(x++) * 4U +
                                 static_cast<std::size_t>(channel)] = value;
                    }
                }
                else
                {
                    const int runLength = static_cast<int>(count);
                    for (int repeat = 0; repeat < runLength; ++repeat)
                    {
                        std::uint8_t value = 0;
                        file.read(reinterpret_cast<char*>(&value), 1);
                        if (!file)
                        {
                            throw std::runtime_error("Unexpected end of HDR literal run: " +
                                                     path.string());
                        }

                        scanline[static_cast<std::size_t>(x++) * 4U +
                                 static_cast<std::size_t>(channel)] = value;
                    }
                }
            }
        }

        for (int x = 0; x < width; ++x)
        {
            float red = 0.0f;
            float green = 0.0f;
            float blue = 0.0f;
            const std::size_t pixelIndex = static_cast<std::size_t>(x) * 4U;
            rgbeToFloat(scanline[pixelIndex + 0], scanline[pixelIndex + 1],
                        scanline[pixelIndex + 2], scanline[pixelIndex + 3], red, green, blue);

            const std::size_t outputIndex =
                (static_cast<std::size_t>(y) * static_cast<std::size_t>(width) +
                 static_cast<std::size_t>(x)) *
                3U;
            texture.floats[outputIndex + 0] = red;
            texture.floats[outputIndex + 1] = green;
            texture.floats[outputIndex + 2] = blue;
        }
    }

    return texture;
}

DecodedTexture decodeTextureFile(const std::filesystem::path& path)
{
    if (path.extension() == ".hdr")
    {
        return decodeHdr(path);
    }

#if defined(_WIN32)
    return decodeWithWic(path);
#else
    (void)path;
    throw std::runtime_error("Texture decoding is only implemented on Windows in this phase.");
#endif
}

unsigned int uploadTexture(const DecodedTexture& texture, const std::filesystem::path& path)
{
    unsigned int textureId = 0;
    glGenTextures(1, &textureId);
    if (textureId == 0)
    {
        throw std::runtime_error("Failed to allocate an OpenGL texture for: " + path.string());
    }

    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    if (texture.hdr)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, texture.width, texture.height, 0, GL_RGB,
                     GL_FLOAT, texture.floats.data());
    }
    else
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, texture.width, texture.height, 0, GL_RGBA,
                     GL_UNSIGNED_BYTE, texture.bytes.data());
    }

    glGenerateMipmap(GL_TEXTURE_2D);
    labelGlObject(GL_TEXTURE, textureId, path.filename().string());
    glBindTexture(GL_TEXTURE_2D, 0);
    return textureId;
}
} // namespace

std::shared_ptr<TextureAsset> TextureAsset::loadFromFile(const AssetMeta& meta)
{
    const DecodedTexture decodedTexture = decodeTextureFile(meta.sourcePath);
    const unsigned int textureId = uploadTexture(decodedTexture, meta.sourcePath);
    return std::make_shared<TextureAsset>(meta, textureId, decodedTexture.width,
                                          decodedTexture.height, decodedTexture.channelCount,
                                          decodedTexture.hdr);
}

TextureAsset::TextureAsset(AssetMeta meta, unsigned int textureId, int width, int height,
                           int channelCount, bool hdr)
    : Asset(std::move(meta)), m_textureId(textureId), m_width(width), m_height(height),
      m_channelCount(channelCount), m_isHdr(hdr)
{
}

TextureAsset::~TextureAsset()
{
    if (m_textureId != 0)
    {
        glDeleteTextures(1, &m_textureId);
    }
}

unsigned int TextureAsset::textureId() const noexcept
{
    return m_textureId;
}

int TextureAsset::width() const noexcept
{
    return m_width;
}

int TextureAsset::height() const noexcept
{
    return m_height;
}

int TextureAsset::channelCount() const noexcept
{
    return m_channelCount;
}

bool TextureAsset::isHdr() const noexcept
{
    return m_isHdr;
}
} // namespace engine
