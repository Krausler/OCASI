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
    {
    }

    Image::Image(std::vector<uint8_t>&& imageData, uint8_t channels, uint32_t width, uint32_t height, const ImageSettings& settings)
        : m_MemoryImage(true), m_Settings(settings)
    {
        m_ImageData = {};
        m_ImageData.Data = std::move(imageData);
        m_ImageData.Channels = channels;
        m_ImageData.Width = width;
        m_ImageData.Height = height;
    }

    Image::Image(std::vector<uint8_t>&& data, const ImageSettings& settings)
        : m_Settings(settings), m_MemoryImage(true)
    {
        m_ImageData = {};
        m_ImageData.Data = std::move(data);
    }
    
    bool Image::LoadImageFromDisk()
    {
        if (m_MemoryImage)
        {
            OCASI_LOG_INFO("Tried to load an image from disk that is memory only.");
            return false;
        }

        stbi_set_flip_vertically_on_load(true);

        stbi_uc* data = stbi_load(m_ImagePath.string().c_str(), (int*) &m_ImageData.Width, (int*) &m_ImageData.Height, (int*) &m_ImageData.Channels, 0);

        if(data)
        {
            uint32_t size = m_ImageData.Width * m_ImageData.Height * m_ImageData.Channels;
            m_ImageData.Data.resize(size);

            std::memcpy(m_ImageData.Data.data(), data, size);

            stbi_image_free(data);
            return true;
        }
        return false;
    }
    
    bool Image::LoadImageFromMemory()
    {
        if (!m_MemoryImage)
        {
            OCASI_LOG_INFO("Tried to load an image from memory that is not a memory only image.");
            return false;
        }

        stbi_set_flip_vertically_on_load(true);
        
        
        OCASI_ASSERT(!m_ImageData.Data.empty());
        stbi_uc* data = stbi_load_from_memory((stbi_uc*)m_ImageData.Data.data(), (int) m_ImageData.Data.size(), (int*) &m_ImageData.Width, (int*) &m_ImageData.Height, (int*) &m_ImageData.Channels, 0);

        if(data)
        {
            uint32_t size = m_ImageData.Width * m_ImageData.Height * m_ImageData.Channels;
            m_ImageData.Data.clear();
            m_ImageData.Data.resize(size);

            std::memcpy(m_ImageData.Data.data(), data, size);

            stbi_image_free(data);
            return true;
        }
        return false;
    }
    
    const ImageData* Image::Load()
    {
        if (m_MemoryImage)
        {
            if(!LoadImageFromMemory())
            {
                return nullptr;
                
            }
            
        }
        else if(!LoadImageFromDisk())
            return nullptr;

        return &m_ImageData;
    }
}