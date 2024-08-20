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

    Image::Image(const std::vector<char>& imageData, uint8_t channels)
        : m_Data({imageData, channels}), m_MemoryImage(true)
    {}

    ImageData Image::LoadImageFromDisk()
    {
        if (m_MemoryImage)
        {
            OCASI_LOG_INFO("Tried to load an image from disk that is memory only.");
            return {};
        }

        stbi_set_flip_vertically_on_load(true);

        ImageData outData = {};
        stbi_uc* data = stbi_load(m_ImagePath.string().c_str(), &outData.Width, &outData.Height, &outData.Channels, 4);

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