#pragma once

#include "OCASI/Core/Base.h"

namespace OCASI {

    struct ImageData
    {
        uint32_t Width, Height;
        uint8_t Channels = 0;
        std::vector<char> Data;
    }

    class Image 
    {
    public:
        Image() = default;
        Image(const Path& path);
        Image(const std::vector<char>& imageData, uint8_t channels);

        // If the image is not a memory image, it's data can be loaded with this function;
        ImageData LoadImageFromDisk();

        bool IsMemoryImage() { return m_MemoryImage; }
        std::vector<char>& GetImageData() { return m_ImageData; }
        uint8_t GetChannels() { return m_Channels; }

        std::filesystem::path& GetImagePath() { return m_ImagePath; }
    private:
        bool m_MemoryImage;
        ImageData m_Data;

        Path m_ImagePath;
    };
}
