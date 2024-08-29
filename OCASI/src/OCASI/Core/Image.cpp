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

    Image::Image(const Path& path)
        : m_ImagePath(path), m_MemoryImage(false) 
    {}

    Image::Image(const std::vector<char>& imageData, uint8_t channels, uint32_t width, uint32_t height)
        : m_MemoryImage(true)
    {
        m_ImageData = {};
        m_ImageData.Data = imageData;
        m_ImageData.Channels = channels;
        m_ImageData.Width = width;
        m_ImageData.Height = height;
    }

    ImageData Image::LoadImageFromDisk()
    {
        if (m_MemoryImage)
        {
            OCASI_LOG_INFO("Tried to load an image from disk that is memory only.");
            return {};
        }

        stbi_set_flip_vertically_on_load(true);

        ImageData outData = {};
        int width, height, channels;
        stbi_uc* data = stbi_load(m_ImagePath.string().c_str(), &width, &height, &channels, 4);
        outData.Width = width;
        outData.Height = height;
        outData.Channels = channels;

        if(data)
        {
            uint32_t size = outData.Width * outData.Height * outData.Channels;
            outData.Data.resize(size);   
            for(uint32_t i = 0; i < outData.Width * outData.Height * outData.Channels; i++)
            {
                outData.Data[i] = data[i];
            }
            stbi_image_free(data);
        }
        else
        {
            OCASI_LOG_ERROR("Failed to load image with path {0}: {1}", m_ImagePath.string(), stbi_failure_reason());
        }

        return outData;
    }
}