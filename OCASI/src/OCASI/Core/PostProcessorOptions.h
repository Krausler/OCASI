#pragma once

namespace OCASI {
    
    //! @brief A bit flag enum for post processing steps, performed after 3D file importing.
    enum class PostProcessorOptions
    {
        None = 0,
        //! Triangulates meshes with FaceType Quad.
        Triangulate,
        
        //! Generates normals if they do not exist inside the mesh.
        GenerateNormals,
        
        //! Converts the mesh vertex data from a left handed coordinate system, with the z axis pointing
        //! into the screen to a right handed coordinated system, with the z axis pointing out of the screen.
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