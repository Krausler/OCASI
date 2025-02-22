#include "FileParser.h"

#include "OCASI/Core/StringUtil.h"

namespace OCASI::OBJ {

    FileParser::FileParser(FileReader &reader)
        : m_FileReader(reader), m_OBJModel(MakeShared<Model>())
    {}

    std::shared_ptr<Model> FileParser::ParseOBJFile()
    {
        std::vector<char> line;
        size_t vertexCount = 0;
        while (m_FileReader.NextLineC(line))
        {
            m_Begin = line.begin();
            m_End = line.end();

            if (m_Begin == m_End)
                continue;

            switch (*m_Begin)
            {
                // Vertices
                case 'v':
                {
                    m_Begin++;
                    switch (*m_Begin)
                    {
                        case ' ':
                        {
                            m_Begin++;
                            size_t spaces = Util::GetAmountOfTokens(m_Begin, m_End, ' ');

                            // vertex colours (xyz rgb)
                            if (spaces == 5)
                            {
                                ParseVertex3D();
                                ParseVertexColour();
                            }
                            // Vertex with 3 components (xyz)
                            else if (spaces == 2)
                            {
                                ParseVertex3D();
                            }
                            // Vertex with 2 components (xy)
                            else if (spaces == 1)
                            {
                                ParseVertex2D();
                            }
                            vertexCount++;
                            break;
                        }

                        case 't':
                        {
                            ParseTextureCoordinate();
                            break;
                        }

                        case 'n':
                        {
                            ParseNormal();
                            break;
                        }

                        default:
                            OCASI_ASSERT_MSG(false, "Invalid line character");
                    }
                    break;
                }

                case 'g':
                {
                    ProcessGroup();
                    break;
                }

                case 'o':
                {
                    ProcessObject();
                    break;
                }

                case 'p':
                case 'l':
                case 'f':
                {
                    // If at face assembly stage, there is no object present, create a group and an
                    // associated mesh.
                    if (!m_CurrentObject)
                    {
                        CreateObject("Object");
                        m_CurrentObject->Meshes.push_back(CreateMesh("Mesh"));
                    }
                    else if (m_CurrentObject->Meshes.empty() && !m_GroupActive)
                    {
                        m_CurrentObject->Meshes.push_back(CreateMesh("Mesh"));
                    }

                    ParseFace();
                    break;
                }

                case 'm':
                {
                    Util::GetToNextSpaceOrEndOfLine(m_Begin, m_End);
                    m_Begin++;
                    m_OBJModel->MTLFilePath = std::string(m_Begin, m_End);
                    break;
                }

                case 'u':
                {
                    ProcessMaterialAssignment();
                    break;
                }

                case '#':
                {
                    // TODO: Not implemented
                    break;
                }

                default:
                    break;
            }
        }

        return m_OBJModel;
    }

    void FileParser::ParseVertex2D()
    {
        m_OBJModel->Vertices.emplace_back(ParseVec2(), 0);
    }

    void FileParser::ParseVertex3D()
    {
        m_OBJModel->Vertices.push_back(ParseVec3());
    }

    void FileParser::ParseVertexColour()
    {
        m_OBJModel->VertexColours.push_back(ParseVec3());
    }

    void FileParser::ParseTextureCoordinate()
    {
        m_Begin += 2;
        m_OBJModel->TexCoords.push_back(ParseVec2());
    }

    void FileParser::ParseNormal()
    {
        m_Begin += 2;
        m_OBJModel->Normals.push_back(ParseVec3());
    }

    void FileParser::ParseFace()
    {
        bool hasNormals = !m_OBJModel->Normals.empty();
        bool hasTexCoords = !m_OBJModel->TexCoords.empty();

        enum Stage : uint8_t
        {
            Vertex = 1,
            TexCoord = 2,
            NormalVec = 3
        };
        uint8_t stage = Stage::Vertex;
        uint8_t vertexCountPerFace = 0;
        m_Begin++;
        Face& face = m_CurrentMesh->Faces.emplace_back();
        while (true)
        {
            size_t iteratorStep = 1;

            if (m_Begin == m_End || Util::IsSpace(*m_Begin))
            {
                if (m_Begin >= m_End)
                    break;

                stage = Stage::Vertex;
                vertexCountPerFace++;
            }
            else if(*m_Begin == '/')
            {
                stage++;
                if(stage == Stage::TexCoord && !hasTexCoords)
                {
                    throw FailedImportError("Cannot request face indices for texture coordinates, when there are no texture coordinates defined.");
                }
                else if(stage == Stage::NormalVec && !hasNormals)
                {
                    throw FailedImportError("Cannot request face indices for normals, when there are no normals defined.");
                }
            }
            else
            {
                std::string toBeParsedString(m_Begin, m_End);
                size_t parsedIndex = std::atof(toBeParsedString.c_str());

                OCASI_ASSERT(parsedIndex != 0 || parsedIndex <= m_OBJModel->Vertices.size());
                // Increase the iterator step by the count of digits
                size_t temp = parsedIndex;
                while ((temp /= 10) != 0)
                    iteratorStep++;

                parsedIndex--;

                switch (stage)
                {
                    case Stage::Vertex:
                    {
                        face.VertexIndices.push_back(parsedIndex);
                        break;
                    }
                    case Stage::TexCoord:
                    {
                        face.TextureCoordinateIndices.push_back(parsedIndex);
                        break;
                    }
                    case Stage::NormalVec:
                    {
                        face.NormalIndices.push_back(parsedIndex);
                        break;
                    }
                    default:
                        throw FailedImportError(FORMAT("Unsupported face assembly stage {}", stage));
                }
            }

            m_Begin += iteratorStep;
        }
        face.Type = (FaceType) vertexCountPerFace;
        m_CurrentMesh->FaceType = (FaceType) vertexCountPerFace;
    }

    void FileParser::ProcessObject()
    {
        m_Begin++;

        // In the case that an object is already present but the active mesh does not have
        // any faces or there are no groups, there is no need to create a new object.
        if (m_CurrentObject && m_CurrentObject->Meshes.empty())
            return;

        std::string name = Util::GetToNextSpaceOrEndOfLine(m_Begin, m_End);
        CreateObject(name);
        m_GroupActive = false;
    }

    void FileParser::ProcessGroup()
    {
        OCASI_ASSERT(m_CurrentObject);
        m_Begin++;

        size_t groupIndex = CreateMesh(Util::GetToNextSpaceOrEndOfLine(m_Begin, m_End));
        m_CurrentObject->Groups.push_back(groupIndex);

        m_GroupActive = true;
    }

    void FileParser::ProcessMaterialAssignment()
    {
        Util::GetToNextSpaceOrEndOfLine(m_Begin, m_End);
        m_Begin++;
        std::string name = std::string(m_Begin, m_End);

        if (m_GroupActive && m_CurrentMesh->Faces.empty())
        {
            m_CurrentMesh->MaterialName = name;
        }
        else
        {
            if (!m_CurrentObject)
                CreateObject(FORMAT("Model_{}",  name));

            m_CurrentObject->Meshes.push_back(CreateMesh(FORMAT("Mesh_{}",  name)));
            m_GroupActive = false;
        }
    }

    size_t FileParser::CreateObject(const std::string& name)
    {
        size_t index = m_OBJModel->RootObjects.size();

        Object& obj = m_OBJModel->RootObjects.emplace_back();
        obj.Name = name;

        m_CurrentObject = &m_OBJModel->RootObjects.at(index);
        return index;
    }

    size_t FileParser::CreateMesh(const std::string& name)
    {
        m_CurrentMesh = &m_OBJModel->Meshes.emplace_back();
        m_CurrentMesh->Name = name;

        return m_OBJModel->Meshes.size() - 1;
    }

    glm::vec3 FileParser::ParseVec3() {
        std::string s1 = Util::GetToNextSpaceOrEndOfLine(m_Begin, m_End);
        float f1 = std::stof(s1);

        std::string s2 = Util::GetToNextSpaceOrEndOfLine(m_Begin, m_End);
        float f2 = std::stof(s2);

        std::string s3 = Util::GetToNextSpaceOrEndOfLine(m_Begin, m_End);
        float f3 = std::stof(s3);

        return { f1, f2, f3 };
    }

    glm::vec2 FileParser::ParseVec2()
    {
        std::string s1 = Util::GetToNextSpaceOrEndOfLine(m_Begin, m_End);
        float f1 = std::stof(s1);

        std::string s2 = Util::GetToNextSpaceOrEndOfLine(m_Begin, m_End);
        float f2 = std::stof(s2);

        return { f1, f2 };
    }
}