#pragma once

#include "OCASI/Core/Base.h"

namespace OCASI {

    struct ImageData
    {
        uint32_t Width, Height;
        uint8_t Channels = 0;
        std::vector<char> Data;
    };

    class Image 
    {
    public:
        Image() = default;
        Image(const Path& path);
        Image(const std::vector<char>& imageData, uint8_t channels, uint32_t width, uint32_t height);

        // If the image is not a memory image, it's data can be loaded with this function;
        ImageData LoadImageFromDisk();

        bool IsMemoryImage() { return m_MemoryImage; }
        const ImageData& GetImageData() { return m_ImageData; }

        std::filesystem::path& GetImagePath() { return m_ImagePath; }
    private:
        bool m_MemoryImage;
        ImageData m_ImageData;

        Path m_ImagePath;
    };
}
