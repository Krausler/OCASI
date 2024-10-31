#include "ObjFileParser.h"

#include "OCASI/Core/StringUtil.h"

namespace OCASI::OBJ {

    ObjFileParser::ObjFileParser(FileReader &reader)
        : m_FileReader(reader), m_OBJModel(std::make_shared<Model>())
    {
    }

    std::shared_ptr<Model> ObjFileParser::ParseOBJFile()
    {
        std::vector<char> line;
        uint32_t vertexCount = 0;
        while (m_FileReader.NextLineC(line))
        {
            m_Begin = line.begin();
            m_End = line.end();

            if (*m_Begin == '\n')
                continue;

            switch (*m_Begin)
            {
                // Vertices
                case 'v':
                {
                    if (!m_CurrentMesh)
                        CreateMesh("Mesh");

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
                    // TODO: Not implemented yet
                    break;
                }

                case 'u':
                {
                    // TODO: Not implemented yet
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

    void ObjFileParser::ParseVertex2D()
    {
        Vertex& v = m_Vertices.emplace_back();
        v.Position = { ParseVec2(), 0 };
        v.VertexIndex = m_Vertices.size();
    }

    void ObjFileParser::ParseVertex3D()
    {
        Vertex& v = m_Vertices.emplace_back();
        v.Position = ParseVec3();
        v.VertexIndex = m_Vertices.size();
    }

    void ObjFileParser::ParseVertexColour()
    {
        m_VertexColours.push_back(ParseVec3());
    }

    void ObjFileParser::ParseTextureCoordinate()
    {
        m_Begin += 2;
        m_TexCoords.push_back(ParseVec2());
    }

    void ObjFileParser::ParseNormal()
    {
        m_Begin += 2;
        m_Normals.push_back(ParseVec3());
    }

    void ObjFileParser::ParseFace()
    {
        bool hasNormals = !m_Normals.empty();
        bool hasTexCoords = !m_TexCoords.empty();

        enum Stage : uint8_t
        {
            Vertex = 1,
            TexCoord = 2,
            Normal = 3
        };
        uint8_t stage = Stage::Vertex;
        uint8_t vertexCountPerFace = 1;
        m_Begin++;
        OCASI::OBJ::Vertex& vertex = m_Vertices.emplace_back();
        while (m_Begin < m_End)
        {
            size_t iteratorStep = 1;
            if(Util::IsLineEnd(*m_Begin))
                break;

            if(*m_Begin == '/')
            {
                stage++;
            }
            else if (Util::IsSpace(*m_Begin))
            {
                stage = Stage::Vertex;
                vertex = m_Vertices.emplace_back();
                vertexCountPerFace++;
            }
            else
            {
                size_t parsedIndex = std::stoi(&(*m_Begin));

                // Increase the iterator step by the count of digits
                while ((parsedIndex /= 10) != 0)
                    iteratorStep++;

                if (stage == Stage::Vertex && !hasTexCoords && hasNormals)
                    stage++;

                switch (stage)
                {
                    case Stage::Vertex:

                        break;
                    case Stage::TexCoord:
                        break;
                    case Stage::Normal:
                        break;
                }
            }

            m_Begin += iteratorStep;
        }

        m_CurrentMesh->FaceType = (OCASI::FaceType) vertexCountPerFace;
    }

    void ObjFileParser::ProcessObject()
    {
        m_Begin++;

        std::string name = Util::GetToNextSpaceOrEndOfLine<FileDataIterator>(m_Begin, m_End);
        Object& o = m_OBJModel->Objects.emplace_back();
        o.Name = name;
        o.ParentMesh = m_OBJModel->Meshes.size();
        CreateMesh(name);
    }

    void ObjFileParser::ProcessGroup()
    {
        m_Begin++;

        if (m_CurrentObject)
        {
            m_CurrentObject->Children.push_back(m_OBJModel->Meshes.size());
        }
        CreateMesh(Util::GetToNextSpaceOrEndOfLine<FileDataIterator>(m_Begin, m_End));
    }

    void ObjFileParser::CreateMesh(const std::string& name)
    {
        m_CurrentMesh = &m_OBJModel->Meshes.emplace_back(name);
        m_Vertices.clear();
        m_VertexColours.clear();
        m_TexCoords.clear();
        m_Normals.clear();
    }

    glm::vec4 ObjFileParser::ParseVec4()
    {
        std::string component1 = Util::GetToNextSpaceOrEndOfLine<FileDataIterator>(m_Begin, m_End);
        m_Begin++;
        std::string component2 = Util::GetToNextSpaceOrEndOfLine<FileDataIterator>(m_Begin, m_End);
        m_Begin++;
        std::string component3 = Util::GetToNextSpaceOrEndOfLine<FileDataIterator>(m_Begin, m_End);
        m_Begin++;
        std::string component4 = Util::GetToNextSpaceOrEndOfLine<FileDataIterator>(m_Begin, m_End);

        return { std::stof(component1), std::stof(component2), std::stof(component3), std::stof(component4) };
    }

    glm::vec3 ObjFileParser::ParseVec3()
    {
        std::string component1 = Util::GetToNextSpaceOrEndOfLine<FileDataIterator>(m_Begin, m_End);
        m_Begin++;
        std::string component2 = Util::GetToNextSpaceOrEndOfLine<FileDataIterator>(m_Begin, m_End);
        m_Begin++;
        std::string component3 = Util::GetToNextSpaceOrEndOfLine<FileDataIterator>(m_Begin, m_End);

        return { std::stof(component1), std::stof(component2), std::stof(component3) };
    }

    glm::vec2 ObjFileParser::ParseVec2()
    {
        std::string component1 = Util::GetToNextSpaceOrEndOfLine<FileDataIterator>(m_Begin, m_End);
        m_Begin++;
        std::string component2 = Util::GetToNextSpaceOrEndOfLine<FileDataIterator>(m_Begin, m_End);

        return { std::stof(component1), std::stof(component2) };
    }
}