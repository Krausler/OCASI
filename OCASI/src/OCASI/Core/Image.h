#pragma once

#include "OCASI/Core/Base.h"

namespace OCASI {

    struct ImageData
    {
        uint32_t Width, Height;
        uint8_t Channels = 0;
        std::vector<char> Data;
    };

    enum class ClampOption
    {
        ClampRepeat = 0,
        ClampToEdge,
        ClampToBorder,
        ClampMirroredRepeat
    };

    enum class TextureOrientation
    {
        None = 0,
        Top = 1,
        Bottom = 2,
        Front = 3,
        Back = 4,
        Left = 5,
        Right = 6,
        Sphere = 7
    };

    struct ImageSettings
    {
        ClampOption Clamp = ClampOption::ClampRepeat; // Set to repeat by default as it's the most common option
        TextureOrientation Orientation = TextureOrientation::None; // Will be TextureOrientation::None when it is not relevant
    };

    class Image 
    {
    public:
        Image() = default;
        Image(const Path& path, const ImageSettings& settings = {});
        Image(const std::vector<char>& imageData, uint8_t channels, uint32_t width, uint32_t height, const ImageSettings& settings = {});
        Image(const ImageData& data, const ImageSettings& settings = {});

        // If the image is not a memory image, it's data can be loaded with this function;
        ImageData LoadImageFromDisk();

        bool IsMemoryImage() const { return m_MemoryImage; }
        const ImageData& GetImageData() const { return m_ImageData; }
        const ImageSettings& GetImageSettings() const { return m_Settings; }
        const Path& GetImagePath() const { return m_ImagePath; }
    private:
        bool m_MemoryImage = false;
        ImageData m_ImageData;
        ImageSettings m_Settings;

        Path m_ImagePath;
    };
}
