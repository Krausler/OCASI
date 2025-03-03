#pragma once

#include "OCASI/Core/Scene.h"
#include "OCASI/Core/PostProcessorOptions.h"

namespace OCASI {
    class BaseImporter;
    
    /*! @brief The importer class for loading assets.
     *
     *  To load an image call Importer::Load3DFile(const Path& path, PostProcessorOptions options)
     */
    class Importer 
    {
    public:
        // TODO: Implement a function taking the file data as a string
        
        /*! @brief Loads a 3D model. Supported file formats are GLTF and OBJ.
         *
         * @param path Specifies the path to the 3D model file to import.
         * @param options A bit enum flag for specifying after importation, scene and mesh transformations and
         *                generations. In short post processing operations..
         * @return The imported scene. Nullptr if scene creation failed.
         */
        static std::shared_ptr<Scene> Load3DFile(const Path& path, PostProcessorOptions options);
        
        /*! @brief Sets a global constant to be applied to all meshes for importing with a specific set
         *         of post processing operations.
         *
         * @param options The post processing operations to be set, in form of a enum bit flag.
         */
        static void SetGlobalPostProcessorOptions(PostProcessorOptions options);
    private:
        static void SetImporters();
    private:
        static std::vector<SharedPtr<BaseImporter>> s_Importers;
        static PostProcessorOptions s_GlobalPostProcessingOptions;
    };
}