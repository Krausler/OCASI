#include "Image.h"

#define STBI_NO_BMP
#define STBI_NO_PSD
#define STBI_NO_TGA
#define STBI_NO_GIF
#define STBI_NO_PIC
#define STBI_NO_PNM
#define STBI_FAILURE_USERMSG
#define STB_IMAGE_IMPLEMENTATION
#include "stbimage/stb_image.h"

namespace OCASI {

    Image::Image(const Path& path, const ImageSettings& settings)
        : m_ImagePath(path), m_Settings(settings)
    {}

    Image::Image(const std::vector<uint8_t>& imageData, uint8_t channels, uint32_t width, uint32_t height, const ImageSettings& settings)
        : m_MemoryImage(true), m_Settings(settings)
    {
        m_ImageData = {};
        m_ImageData.Data = imageData;
        m_ImageData.Channels = channels;
        m_ImageData.Width = width;
        m_ImageData.Height = height;
    }

    Image::Image(const std::vector<uint8_t>& data, bool load, const ImageSettings& settings)
        : m_Settings(settings)
    {
        m_ImageData = {};
        m_ImageData.Data = data;

        if (load)
            ImportImageFromData();
    }

    const ImageData* Image::LoadImageFromDisk()
    {
        if (m_MemoryImage)
        {
            OCASI_LOG_INFO("Tried to load an image from disk that is memory only.");
            return nullptr;
        }

        stbi_set_flip_vertically_on_load(true);

        ImageData& outData = m_ImageData = {};
        int width, height, channels;
        stbi_uc* data = stbi_load(m_ImagePath.string().c_str(), &width, &height, &channels, 4);
        outData.Width = width;
        outData.Height = height;
        outData.Channels = channels;

        if(data)
        {
            uint32_t size = outData.Width * outData.Height * outData.Channels;
            outData.Data.resize(size);

            std::memcpy(outData.Data.data(), data, size);

            stbi_image_free(data);
        }
        else
        {
            OCASI_FAIL(FORMAT("Failed to load image with path {0}: {1}", m_ImagePath.string(), stbi_failure_reason()));
        }

        return &outData;
    }

    const ImageData* Image::ImportImageFromData()
    {
        if (!m_MemoryImage)
        {
            OCASI_LOG_INFO("Tried to load an image from memory that is not a memory only image.");
            return nullptr;
        }

        stbi_set_flip_vertically_on_load(true);

        OCASI_ASSERT(!m_ImageData.Data.empty());
        ImageData& outData = m_ImageData = {};
        stbi_uc* data = stbi_load_from_memory((stbi_uc*)m_ImageData.Data.data(), m_ImageData.Data.size(), (int*) &outData.Width, (int*) &outData.Height, (int*) &outData.Channels, 4);

        if(data)
        {
            uint32_t size = outData.Width * outData.Height * outData.Channels;
            outData.Data.resize(size);

            std::memcpy(outData.Data.data(), data, size);

            stbi_image_free(data);
        }
        else
        {
            OCASI_FAIL(FORMAT("Failed to load image with path {0}: {1}", m_ImagePath.string(), stbi_failure_reason()));
        }

        return nullptr;
    }
}