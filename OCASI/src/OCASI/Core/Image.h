#pragma once

#include "OCASI/Core/Base.h"

namespace OCASI {

    struct ImageData
    {
        uint32_t Width = 0, Height = 0;
        uint8_t Channels = 0;
        std::vector<uint8_t> Data;
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

    enum class FilterOption
    {
        Linear = 0,
        Nearest,
        NearestMipMapNearest,
        NearestMipMapLinear,
        LinearMipMapLinear,
        LinearMipMapNearest
    };

    enum class ImageType
    {
        None = 0,
        PNG,
        JPEG
    };

    struct ImageSettings
    {
        // Even though the Linear option is more computational complex and can create some overhead,
        // its results are much more visual pleasing
        FilterOption MinFilter = FilterOption::Linear;
        FilterOption MagFilter = FilterOption::Linear;

        ClampOption Clamp = ClampOption::ClampRepeat; // Set to repeat by default as it's the most common option
        TextureOrientation Orientation = TextureOrientation::None; // Will be TextureOrientation::None when it is not relevant
    };

    class Image 
    {
    public:
        Image() = default;
        Image(const Path& path, const ImageSettings& settings = {});
        Image(const std::vector<uint8_t>& imageData, uint8_t channels, uint32_t width, uint32_t height, const ImageSettings& settings = {});
        Image(const std::vector<uint8_t>& data, bool load = false, const ImageSettings& settings = {});

        // If the image is not a memory image, it's data can be loaded with this function;
        const ImageData* LoadImageFromDisk();
        const ImageData* ImportImageFromData();

        bool IsMemoryImage() const { return m_MemoryImage; }
        bool IsLoaded() const { return m_MemoryImage && m_ImageData.Width != 0 && m_ImageData.Height != 0 && m_ImageData.Channels != 0; }
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
