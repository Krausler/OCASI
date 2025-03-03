#pragma once

#include "OCASI/Core/Base.h"

namespace OCASI {

    //! @brief This struct either stores the output of an image loaded by stb_image or populates the Data field to read from,
    //!        when loading is issued.
    struct ImageData
    {
        //! The width and height of the image.
        uint32_t Width = 0, Height = 0;
        
        //! The amount of channels the image is made of (1 = r, 2 = rg, 3 = rgb, 4 = rgba, ...).
        uint8_t Channels = 0;
        
        //! Either the output image data in bytes or the input data for loading the image.
        std::vector<uint8_t> Data;
    };

    //! @brief Specifies how to react, if a meshes texture coordinate is not in the range 0.0f - 1.0f.
    enum class ClampOption
    {
        //! Repeats the image.
        Repeat = 0,
        
        //! Discards the pixels (the affected pixels will be black in most cases).
        ClampToEdge,
        
        //! Uses whatever pixel was the last to be in the correct texture coordinate range.
        ClampToBorder,
        
        //! Repeats the image but flips it, before applying the image.
        MirroredRepeat
    };

    //! @brief This is only used for weird reflection map textures with the OBJ file format. TextureOrientations are used very rarely.
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

    //! Defines what anisotropy filtering technique is used. For more information take a look at the vkspec documentation:
    //! https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html#textures-texel-filtering
    enum class FilterOption
    {
        Linear = 0,
        Nearest,
        NearestMipMapNearest,
        NearestMipMapLinear,
        LinearMipMapLinear,
        LinearMipMapNearest
    };

    // TODO: Add the ImageType to the ImageSettings
    //! @brief Specifies the type of image compression.
    enum class ImageType
    {
        None = 0,
        PNG,
        JPEG
    };

    //! @brief Settings for the Image class.
    struct ImageSettings
    {
        // Even though the Linear option is more computational complex and can create some overhead,
        // its results are much more visual pleasing
        FilterOption MinFilter = FilterOption::Linear;
        FilterOption MagFilter = FilterOption::Linear;

        ClampOption Clamp = ClampOption::Repeat; // SetValue to repeat by default as it's the most common option
        TextureOrientation Orientation = TextureOrientation::None; // Will be TextureOrientation::None when it is not relevant
    };

    //! @brief This class manages images, specified by the different importers. Images may be a path to an image or the compressed
    //!        image data in bytes. Images data can be loaded or handled by the user internally.
    class Image 
    {
    public:
        Image() = default;
        
        //! @brief Constructs a new image from the image file path and the settings.
        Image(const Path& path, const ImageSettings& settings = {});
        //! @brieg Constructs an image from the already decoded image data, channels, width and height and the settings.
        Image(std::vector<uint8_t>&& imageData, uint8_t channels, uint32_t width, uint32_t height, const ImageSettings& settings = {});
        //! @brieg Constructs an image from the none decoded data and the settings.
        Image(std::vector<uint8_t>&& data, const ImageSettings& settings = {});

        // If the image is not a memory image, it's data can be loaded with this function;
        /*! @brief Loads an image from disk and decodes the data, if the image is not a memory image.
         *
         *  @return Whether image loading was successful.
         */
        bool LoadImageFromDisk();
        /*! @brief Loads an image from disk and decodes the data, if the image is a memory image.
         *
         *  @return Whether image loading was successful.
         */
        bool LoadImageFromMemory();
        /*! @brief Loads an image from disk or memory.
         *
         *  @return The loaded image data.
         */
        const ImageData* Load();

        bool IsMemoryImage() const { return m_MemoryImage; }
        bool IsLoaded() const { return m_ImageData.Width != 0 && m_ImageData.Height != 0 && m_ImageData.Channels != 0; }
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
