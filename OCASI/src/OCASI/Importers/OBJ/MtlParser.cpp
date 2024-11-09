#include "MtlParser.h"

#include "OCASI/Core/StringUtil.h"

namespace OCASI::OBJ {

    MtlParser::MtlParser(const std::shared_ptr<Model> &model, const Path& relativePath)
        : m_Reader(relativePath), m_Model(model)
    {
    }

    bool MtlParser::ParseMTLFile()
    {
        if (!m_Reader.IsOpen())
        {
            OCASI_FAIL(FORMAT("Failed to open material file with path: {}", m_Reader.GetPath().string()))
            return false;
        }

        std::vector<char> line;
        while (m_Reader.NextLineC(line))
        {
            m_Begin = line.begin();
            m_End = line.end();

            if (m_Begin == m_End)
                continue;

            ParseParameter(*m_Begin == 'm');
        }

        return true;
    }

    bool MtlParser::ParseParameter(bool isMap)
    {
        if (isMap)
        {
            m_Begin += 4;
        }

        switch (*m_Begin)
        {
            // New material
            case 'n':
            {
                std::string statement = Util::GetToNextToken(m_Begin, m_End, ' ');
                if (statement == "newmtl")
                    CreateNewMaterial(Util::GetToNextSpaceOrEndOfLine(m_Begin, m_End));
                else if (statement == "norm")
                    ParseTexture(TextureType::Normal);
                break;
            }
            case 'K':
            {
                if (!CheckMaterial())
                    return false;

                m_Begin += 3;
                switch (*(m_Begin - 2))
                {
                    // Diffuse colour
                    case 'd':
                    {
                        // Skipping the parameter identifier and the space separating the value
                        if(isMap)
                            ParseTexture(TextureType::Diffuse);
                        else
                            m_CurrentMaterial->DiffuseColour = ParseVec3();

                        break;
                    }
                    // Specular colour
                    case 's':
                    {
                        if(isMap)
                            ParseTexture(TextureType::Specular);
                        else
                            m_CurrentMaterial->SpecularColour =  ParseVec3();

                        break;
                    }
                    // Emissive colour
                    case 'e':
                    {
                        if(isMap)
                            ParseTexture(TextureType::Emissive);
                        else
                            m_CurrentMaterial->EmissiveColour = ParseVec3();

                        break;
                    }
                    // Ambient colour
                    case 'a':
                    {
                        if(isMap)
                            ParseTexture(TextureType::Ambient);
                        else
                            m_CurrentMaterial->AmbientColour = ParseVec3();

                        break;
                    }
                    // Normal map
                    case 'n':
                    {
                        if (isMap)
                            ParseTexture(TextureType::Normal);
                    }
                    default:
                        OCASI_LOG_WARN("Unknown parameter: K{}", *(m_Begin - 2));
                        break;
                }
                break;
            }
            case 'N':
            {
                if (!CheckMaterial())
                    return false;

                m_Begin += 2;
                if (*(m_Begin - 1) == 'i')
                {
                    m_CurrentMaterial->IOR = ParseFloat();
                }
                else if(*(m_Begin - 1) == 's')
                {
                    if(isMap)
                        ParseTexture(TextureType::Shininess);
                    else
                        m_CurrentMaterial->Shininess = ParseFloat();
                }
                else
                {
                    OCASI_LOG_WARN("Unknown parameter: N{}", *(m_Begin - 2));
                }

                break;
            }
            case 'T':
            case 'd':
            {
                if (!CheckMaterial())
                    return false;

                if  (isMap)
                    m_CurrentMaterial->Opacity = ParseFloat();
                else
                    ParseTexture(TextureType::Transparency);
                break;
            }
            case 'i':
            {
                if (!CheckMaterial())
                    return false;

                m_Begin += 5;
                m_CurrentMaterial->Opacity = ParseFloat();
                break;
            }
            // PBR optional extension
            case 'P':
            {
                if (!CheckMaterial())
                    return false;

                m_Begin += 3;
                switch (*(m_Begin - 2))
                {
                    // Roughness
                    case 'r':
                    {
                        if (isMap)
                            ParseTexture(TextureType::Roughness);
                        else
                            m_CurrentMaterial->Roughness = ParseFloat();
                        break;
                    }
                    // Metallic
                    case 'm':
                    {
                        if (isMap)
                            ParseTexture(TextureType::Metallic);
                        else
                            m_CurrentMaterial->Metallic = ParseFloat();
                        break;
                    }
                    // Sheen
                    case 's':
                    {
                        if (isMap)
                            ParseTexture(TextureType::Sheen);
                        else
                            m_CurrentMaterial->Sheen = ParseFloat();
                        break;
                    }
                    // Clearcoat and CleacoatRougness
                    case 'c':
                    {
                        if (*(m_Begin - 1) == 'r')
                        {
                            if (isMap)
                                ParseTexture(TextureType::ClearcoatRoughness);
                            else
                                m_CurrentMaterial->ClearcoatRoughness = ParseFloat();
                        }
                        else if(*(m_Begin - 1) == ' ')
                        {
                            if (isMap)
                                ParseTexture(TextureType::Clearcoat);
                            else
                                m_CurrentMaterial->Clearcoat = ParseFloat();
                        }
                        else
                        {
                            OCASI_LOG_WARN("Unknown parameter: Pc{}", *(m_Begin - 1));
                        }
                        break;
                    }
                    default:
                        OCASI_LOG_WARN("Unknown parameter: P{}", *(m_Begin - 2));
                        break;
                }
                break;
            }
            // Anisotropy
            case 'a':
            {
                if (!CheckMaterial())
                    return false;

                size_t charactersToNextSpace = Util::GetToNextToken(m_Begin, m_End, ' ').size();

                // Either aniso or an
                if (charactersToNextSpace == 5 || charactersToNextSpace == 2)
                {
                    m_Begin += charactersToNextSpace;
                    m_CurrentMaterial->Anisotropy = ParseFloat();
                }
                // Either anisor or anR
                else if (charactersToNextSpace == 6 || charactersToNextSpace == 3)
                {
                    m_Begin += charactersToNextSpace;
                    m_CurrentMaterial->AnisotropyRotation = ParseFloat();
                }
                else
                {
                    OCASI_LOG_WARN("Unknown parameter: Parameter starts with 'a' but does not fulfill the length requirements. Length is {}.", charactersToNextSpace);
                }
                break;
            }
            // Occlusion map
            case 'o':
            {
                if (!CheckMaterial())
                    return false;

                if (isMap)
                    ParseTexture(TextureType::Occlusion);
                else
                {
                    m_Begin++;
                    OCASI_LOG_WARN("Unkown parameter: o{} is not a map", Util::GetToNextSpaceOrEndOfLine(m_Begin, m_End));
                }
                break;
            }
            // Bump map
            case 'b':
            {
                if (!CheckMaterial())
                    return false;

                ParseTexture(TextureType::Bump);
                break;
            }

            default:
                break;
        }

        return true;
    }

    void MtlParser::CreateNewMaterial(const std::string &name)
    {
        m_Model->Materials[name] = {};
        m_CurrentMaterial = &m_Model->Materials.at(name);
        m_CurrentMaterial->Name = name;
    }

    float MtlParser::ParseFloat()
    {
        std::string s = Util::GetToNextSpaceOrEndOfLine(m_Begin, m_End);
        return std::stof(s);
    }

    glm::vec3 MtlParser::ParseVec3()
    {
        std::string s1 = Util::GetToNextSpaceOrEndOfLine(m_Begin, m_End);
        float f1 = std::stof(s1);

        std::string s2 = Util::GetToNextSpaceOrEndOfLine(m_Begin, m_End);
        float f2 = std::stof(s2);

        std::string s3 = Util::GetToNextSpaceOrEndOfLine(m_Begin, m_End);
        float f3 = std::stof(s3);

        return { f1, f2, f3 };
    }

    glm::vec4 MtlParser::ParseVec4()
    {
        std::string s1 = Util::GetToNextSpaceOrEndOfLine(m_Begin, m_End);
        float f1 = std::stof(s1);

        std::string s2 = Util::GetToNextSpaceOrEndOfLine(m_Begin, m_End);
        float f2 = std::stof(s2);

        std::string s3 = Util::GetToNextSpaceOrEndOfLine(m_Begin, m_End);
        float f3 = std::stof(s3);

        std::string s4 = Util::GetToNextSpaceOrEndOfLine(m_Begin, m_End);
        float f4 = std::stof(s4);

        return { f1, f2, f3, f4 };
    }

    bool MtlParser::CheckMaterial()
    {
        if (!m_CurrentMaterial)
        {
            OCASI_FAIL("Trying to write to a material, when there was none initialized.");
            return false;
        }
        return true;
    }

    void MtlParser::ParseTexture(TextureType type)
    {
        if (m_Begin == m_End)
        {
            OCASI_FAIL("Tried to parse texture, while end of line was already reached.");
            return;
        }

        while (true)
        {
            if (m_Begin == m_End || Util::IsLineEnd(*m_Begin))
                break;

            auto begin = m_Begin;
            // Acquire the next argument or texture name
            std::string tokenSequence = Util::GetToNextSpaceOrEndOfLine(m_Begin, m_End);

            if (tokenSequence.at(0) == '-')
            {
                tokenSequence.erase(0);

                if (tokenSequence == "bump")
                {
                    float bumpValue = ParseFloat();
                    if(bumpValue != 0)
                        m_CurrentMaterial->BumpMapMultiplier = bumpValue;
                }
                else if (tokenSequence == "type" && type == Reflection)
                {
                    std::string orientation = Util::GetToNextSpaceOrEndOfLine(m_Begin, m_End);

                    if (orientation == "cube_top")
                    {
                        type = TextureType::ReflectionTop;
                    }
                    else if (orientation == "cube_bottom")
                    {
                        type = TextureType::ReflectionBottom;
                    }
                    else if (orientation == "cube_front")
                    {
                        type = TextureType::ReflectionFront;
                    }
                    else if (orientation == "cube_back")
                    {
                        type = TextureType::ReflectionBack;
                    }
                    else if (orientation == "cube_left")
                    {
                        type = TextureType::ReflectionLeft;
                    }
                    else if (orientation == "cube_right")
                    {
                        type = TextureType::ReflectionRight;
                    }
                    else if (orientation == "sphere")
                    {
                        type = TextureType::ReflectionSphere;
                    }
                }
                else if (tokenSequence == "clamp")
                {
                    std::string option = Util::GetToNextSpaceOrEndOfLine(m_Begin, m_End);
                    if (option == "on")
                        m_CurrentMaterial->TextureClamps.at(type) = true;
                }
                // Only have one parameter. This is skipped
                else if (tokenSequence == "blendu" || tokenSequence == "blendv" || tokenSequence == "blend" || tokenSequence == "boost" || tokenSequence == "imfchan")
                {
                    Util::GetToNextSpaceOrEndOfLine(m_Begin, m_End);
                }
                // Have two parameters. This is skipped
                else if (tokenSequence == "mm")
                {
                    Util::GetToNextSpaceOrEndOfLine(m_Begin, m_End);
                    Util::GetToNextSpaceOrEndOfLine(m_Begin, m_End);
                }
                else if (tokenSequence == "o" || tokenSequence == "s" || tokenSequence == "t")
                {
                    Util::GetToNextSpaceOrEndOfLine(m_Begin, m_End);
                    Util::GetToNextSpaceOrEndOfLine(m_Begin, m_End);
                    Util::GetToNextSpaceOrEndOfLine(m_Begin, m_End);
                }
            }
            else
            {
                m_CurrentMaterial->Textures[type] = Util::GetToNextToken(begin, m_End, '\n');
            }
        }
    }
}