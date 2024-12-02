#include "OCASI/Core/Importer.h"
#include "OCASI/Core/FileUtil.h"
#include "OCASI/Core/StringUtil.h"

#include "iostream"

#include "glaze/glaze.hpp"

int main()
{
    OCASI::Importer::Init();

    OCASI_LOG_ERROR("hi");
    std::filesystem::current_path("C:/Lauri/Dev/C++/Projekte/Octopus/Projekte/OCASI/Tester");
    auto scene = OCASI::Importer::Load3DFile("Resources/GLTF/Mushroom.glb");
    std::cout << std::filesystem::current_path().string() << std::endl;


#if 0
    std::filesystem::current_path("C:/Lauri/Dev/C++/Projekte/OCASI/Tester");
    OCASI::FileReader reader("Resources/TestObject.obj");

    std::vector<char> line;
    std::vector<char>::iterator begin;
    std::vector<char>::iterator end;
    while (reader.NextLineC(line))
    {
        begin = line.begin();
        end = line.end();

        if (begin == end)
        {
            std::cout << "begin == end" << std::endl;
            continue;
        }

        if (*begin == 'v')
        {
            begin++;
            std::cout << OCASI::Util::GetToNextSpaceOrEndOfLine(begin, end) << std::endl;
        }
    }
#endif
}