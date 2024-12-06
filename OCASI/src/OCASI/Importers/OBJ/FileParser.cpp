#include "FileParser.h"

#include "OCASI/Core/StringUtil.h"

namespace OCASI::OBJ {

    FileParser::FileParser(FileReader &reader)
        : m_FileReader(reader), m_OBJModel(std::make_shared<Model>())
    {}

    std::shared_ptr<Model> FileParser::ParseOBJFile()
    {
        std::vector<char> line;
        uint32_t vertexCount = 0;
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
                    if (!m_CurrentMesh)
                        CreateMesh("Model");

                    m_Begin++;
                    switch (*m_Begin)
                    {
                        case ' ':
                        {
                            m_Begin++;
                            uint32_t spaces = Util::GetAmountOfTokens(m_Begin, m_End, ' ');

                            // vertex colours (xyz rgb)
                            if (spaces == 5)
                            {
                                ParseVertex3D();
                                ParseVertexColour();
                                m_CurrentMesh->Dimension = Dimension::_3D;
                            }
                            // Vertex with 3 components (xyz)
                            else if (spaces == 2)
                            {
                                ParseVertex3D();
                                m_CurrentMesh->Dimension = Dimension::_3D;
                            }
                            // Vertex with 2 components (xy)
                            else if (spaces == 1)
                            {
                                ParseVertex2D();
                                m_CurrentMesh->Dimension = Dimension::_2D;
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
                    ParseFace();
                    break;
                }

                case 'm':
                {
                    Util::GetToNextSpaceOrEndOfLine(m_Begin, m_End);
                    m_Begin++;
                    m_OBJModel->MTLFilePath = { m_Begin, m_End };
                    break;
                }

                case 'u':
                {
                    std::string materialName = Util::GetToNextSpaceOrEndOfLine(m_Begin, m_End);

                    if (m_CurrentMesh->MaterialName.empty() && m_CurrentMesh->Faces.empty())
                    {
                        // TODO: set the current objects material
                    }

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
            Normal = 3
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
                    OCASI_FAIL("OBJ file contains face texture coordinate indices without defined texture coordinates.");
                }
                else if(stage == Stage::Normal && !hasNormals)
                {
                    OCASI_FAIL("OBJ file contains face normal indices without defined normals.");
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
                    case Stage::Normal:
                    {
                        face.NormalIndices.push_back(parsedIndex);
                        break;
                    }
                    default:
                        OCASI_FAIL("OBJ: Invalid Stage for face assembly.");
                        break;
                }
            }

            m_Begin += iteratorStep;
        }
        face.Type = (FaceType) vertexCountPerFace;
        m_CurrentMesh->FaceType = m_CurrentMesh->FaceType | ((FaceType) vertexCountPerFace);
    }

    void FileParser::ProcessObject()
    {
        m_Begin++;

        // Checking whether the current mesh attached to the object has already got faces.
        // When it doesn't it does not make sense to create a new object and mesh.
        if (m_CurrentObject)
        {
            Mesh &m = m_OBJModel->Meshes.at(m_CurrentObject->Mesh);
            if (m.Faces.empty())
                return;
        }

        std::string name = Util::GetToNextSpaceOrEndOfLine(m_Begin, m_End);
        Object& o = m_OBJModel->Objects.emplace_back();
        o.Name = name;
        o.Mesh = m_OBJModel->Meshes.size();
        m_CurrentObject = &o;
        CreateMesh(name);
    }

    void FileParser::ProcessGroup()
    {
        m_Begin++;

        if (m_CurrentObject)
        {
            Mesh& m = m_OBJModel->Meshes.at(m_CurrentObject->Mesh);
            if (m.Faces.empty())
                return;
            m_CurrentObject->Children.push_back(m_OBJModel->Meshes.size());
        }
        CreateMesh(Util::GetToNextSpaceOrEndOfLine<FileDataIterator>(m_Begin, m_End));
    }

    void FileParser::CreateMesh(const std::string& name)
    {
        m_CurrentMesh = &m_OBJModel->Meshes.emplace_back(name);
        m_CurrentMesh->Name = name;
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

#if 0
    void ObjFileParser::CreateNewVertex(Vertex &v)
    {
        m_CurrentMesh->Vertices.push_back(m_Vertices.at(v.VertexIndex - m_CurrentVertexIndex));
        if(!m_TexCoords.empty())
            m_CurrentMesh->TexCoords.push_back(m_TexCoords.at(v.TextureCoordinateIndex - m_CurrentTextureCoordinateIndex));
        if(!m_Normals.empty())
            m_CurrentMesh->Normals.push_back(m_Normals.at(v.NormalIndex - m_CurrentNormalIndex));

        size_t index = m_CurrentMesh->Vertices.size() - 1;
        m_CurrentMesh->Indices.push_back(index);
        v.IndicesIndex = index;
        m_VerticesLookupTable[v.VertexIndex].push_back(v);
    }
#endif
}