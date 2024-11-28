#include "OCASI/Core/Importer.h"
#include "OCASI/Core/FileUtil.h"
#include "OCASI/Core/StringUtil.h"

#include "iostream"

#include "glaze/glaze.hpp"

int main()
{
    OCASI::Importer::Init();

    OCASI_LOG_ERROR("hi");
    std::filesystem::current_path("C:/Lauri/Dev/C++/Projekte/OCASI/Tester");
//    auto scene = OCASI::Importer::Load3DFile("Resources/Mushroom_smartUVs_01.obj");
//
//    std::cout << std::filesystem::current_path().string() << std::endl;

    OCASI::FileReader reader("Resources/GLTF/Mushroom.gltf");
    glz::json_t json;
    auto e = glz::read_json(json, reader.GetFileString());

    json = json["accessors"];

    OCASI_LOG_INFO("type: {}", json.is_array());


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