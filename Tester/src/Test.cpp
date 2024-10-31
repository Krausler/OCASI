
#include "OCASI/Core/Importer.h"
#include "iostream"

int main()
{
    OCASI::Importer::Init();
    std::filesystem::current_path("C:/Lauri/Dev/C++/Projekte/OCASI/Tester");
    OCASI::Importer::Load3DFile("Resources/TestObject.obj");
    
    std::cout << std::filesystem::current_path().string() << std::endl;
}