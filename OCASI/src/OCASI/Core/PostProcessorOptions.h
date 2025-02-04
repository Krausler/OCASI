#pragma once

namespace OCASI {
    
    enum class PostProcessorOptions
    {
        None = 0,
        Triangulate,
        GenerateNormals,
        GenerateTextureCoordinates,
        CollapseChildNodes,
        ConvertToRHC
    };
    
    inline PostProcessorOptions operator|(PostProcessorOptions first, PostProcessorOptions second)
    {
        return (PostProcessorOptions)((int)first | (int) second);
    }
    
    inline bool operator&(PostProcessorOptions first, PostProcessorOptions second)
    {
        return ((int)first & (int) second) == (int)second;
    }
}