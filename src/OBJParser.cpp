#include "tp_obj/OBJParser.h"

#include "tp_math_utils/materials/OpenGLMaterial.h"
#include "tp_math_utils/materials/LegacyMaterial.h"

#include "tp_utils/FileUtils.h"

namespace tp_obj
{

namespace
{

//##################################################################################################
std::string joinName(const std::vector<std::string>& parts)
{
  std::string name;
  for(size_t i=1; i<parts.size(); i++)
  {
    if(!name.empty())
      name+=' ';
    name += parts.at(i);
  }
  return name;
}

//##################################################################################################
bool readBool(const std::string& s)
{
  auto ss = tpToLower(s);

  if(ss == "0" || ss == "false")
    return false;

  return true;
}

//##################################################################################################
float readFloat(const std::string& s)
{
  float v;
  std::istringstream istr(s);
  istr.imbue(std::locale("C"));
  istr >> v;
  return v;
}

//##################################################################################################
int readInt(const std::string& s)
{
  int n=0;
  tpNumber(s, n);
  return n;
}

}

//##################################################################################################
std::vector<std::vector<std::string>> parseLines(const std::string& filePath, std::string* exporterVersion)
{
  std::vector<std::vector<std::string>> lines;

  auto splitLine = [](std::string line, std::string* exporterVersion)
  {
    std::vector<std::string> parts;
    if(auto i=line.find_first_of('#'); i!=std::string::npos)
    {
      if(nullptr != exporterVersion)
      {
        const std::string exporterVersionPrefix = "# OMI OBJ exporter v";
        if(0 == line.rfind(exporterVersionPrefix, 0))
          *exporterVersion = line.substr(exporterVersionPrefix.size());
      }

      line.erase(i);
    }

    line.erase(0, line.find_first_not_of(" \t\n\r\f\v"));

    tpSplit(parts, line, ' ', TPSplitBehavior::SkipEmptyParts);
    return parts;
  };

  std::vector<std::string> rawLines;
  std::string text = tp_utils::readTextFile(filePath);
  tpRemoveChar(text, '\r');
  tpSplit(rawLines, text, '\n', TPSplitBehavior::SkipEmptyParts);

  lines.reserve(rawLines.size());
  for(const auto& line : rawLines)
    if(std::vector<std::string> parts = splitLine(line, exporterVersion); !parts.empty())
      lines.push_back(parts);

  return lines;
}

//##################################################################################################
bool parseOBJ(const std::string& filePath,
              std::string& error,
              int triangleFan,
              int triangleStrip,
              int triangles,
              bool reverse,
              std::string& exporterVersion,
              std::vector<tp_math_utils::Geometry3D>& outputGeometry)
{
  auto barf = [&](auto msg)
  {
    error += "Parse OBJ error: ";
    error += msg;
    return false;
  };

  std::vector<std::vector<std::string>> lines = parseLines(filePath, &exporterVersion);

  std::vector<glm::vec3> objVV;
  std::vector<glm::vec2> objVT;
  std::vector<glm::vec3> objVN;
  std::vector<tp_math_utils::Material> objMaterials;

  //-- Reserve -----------------------------------------------------------------------------------
  {
    size_t objVVCount=0;
    size_t objVTCount=0;
    size_t objVNCount=0;
    for(const auto& parts : lines)
    {
      std::string c = parts.front();
      if     (c == "v" ) objVVCount++;
      else if(c == "vt") objVTCount++;
      else if(c == "vn") objVNCount++;
    }

    objVV.reserve(objVVCount);
    objVT.reserve(objVTCount);
    objVN.reserve(objVNCount);
  }

  //-- Extract verts, tex coords, and normals ------------------------------------------------------
  try
  {
    for(const auto& parts : lines)
    {
      std::string c = parts.front();
      if(c == "v")
      {
        if(parts.size()<4)
          return barf("v should have 4 parts.");

        glm::vec3& v = objVV.emplace_back();

        v.x = std::stof(parts.at(1));
        v.y = std::stof(parts.at(2));
        v.z = std::stof(parts.at(3));

        if(glm::any(glm::isnan(v)))
          return barf("v NaN.");

        if(glm::any(glm::isinf(v)))
          return barf("v inf.");
      }

      else if(c == "vt")
      {
        if(parts.size()<3)
          return barf("vt should have 3 parts.");

        glm::vec2& v = objVT.emplace_back();

        v.x = std::stof(parts.at(1));
        v.y = std::stof(parts.at(2));

        if(glm::any(glm::isnan(v)))
          return barf("v NaN.");

        if(glm::any(glm::isinf(v)))
          return barf("v inf.");

        if(reverse)
          v.y = 1.0f-v.y;
      }

      else if(c == "vn")
      {
        if(parts.size()<4)
          return barf("vn should have 4 parts.");

        glm::vec3& v = objVN.emplace_back();

        v.x = std::stof(parts.at(1));
        v.y = std::stof(parts.at(2));
        v.z = std::stof(parts.at(3));

        if(glm::any(glm::isnan(v)))
          return barf("v NaN.");

        if(glm::any(glm::isinf(v)))
          return barf("v inf.");
      }

      else if(c == "mtllib")
      {
        parseMTL(tp_utils::pathAppend(tp_utils::directoryName(filePath), joinName(parts)), error, objMaterials);
      }
    }
  }
  catch(const std::invalid_argument& e)
  {
    return barf(e.what());
  }
  catch(const std::out_of_range& e)
  {
    return barf(e.what());
  }

  //-- Extract objects and faces -------------------------------------------------------------------
  {
    std::string materialName;
    std::string objectName;
    std::string groupName;
    bool newObject=true;
    bool newMesh=true;

    std::map<std::tuple<size_t,size_t,size_t>, int> indexes;

    for(const auto& parts : lines)
    {
      std::string c = parts.front();

      if(c == "o")
      {
        objectName=joinName(parts);
        newObject = true;
        newMesh = true;
      }

      else if(c == "usemtl")
      {
        materialName=joinName(parts);
        newObject = true;
        newMesh = true;
      }
      else if(c == "g")
      {
        groupName=joinName(parts);
        newMesh = true;
      }
      else if(c == "s")
      {
        groupName=joinName(parts);
      }

      else if(c == "f")
      {
        if(parts.size()<4)
          continue;

        if(newObject)
        {
          newObject = false;
          indexes.clear();
          auto& o = outputGeometry.emplace_back();
          o.triangleFan   = triangleFan  ;
          o.triangleStrip = triangleStrip;
          o.triangles     = triangles    ;

          if(!objectName.empty())
          {
            o.comments.push_back("MESH_NAME");
            o.comments.push_back(objectName);
          }

          if(!materialName.empty())
          {
            o.comments.push_back("MATERIAL_NAME");
            o.comments.push_back(materialName);
          }

          o.material.name = materialName;

          for(const auto& m : objMaterials)
          {
            if(m.name == o.material.name)
            {
              o.material = m;
              break;
            }
          }
        }

        auto& o = outputGeometry.back();

        if(newMesh)
        {
          newMesh = false;
          auto& f = o.indexes.emplace_back();
          f.type = o.triangles;
        }

        auto& f = o.indexes.back();

        auto parseAddVert = [&](const std::string& part)
        {
          size_t vvi=0;
          size_t vti=0;
          size_t vni=0;

          try
          {
            std::vector<std::string> indexes;
            tpSplit(indexes, part, '/', TPSplitBehavior::KeepEmptyParts);

            if(indexes.size()>=1)
              vvi = size_t(std::stoull(indexes.at(0)))-1;

            if(indexes.size()>=2)
            {
              if(indexes.at(1).empty())
                vti = vvi;
              else
                vti = size_t(std::stoull(indexes.at(1)))-1;
            }
            else
              vti = vvi;

            if(indexes.size()>=3)
            {
              if(indexes.at(2).empty())
                vni = vvi;
              else
                vni = size_t(std::stoull(indexes.at(2)))-1;
            }
            else
              vni = vvi;
          }
          catch (const std::invalid_argument&)
          {
            return -1;
          }
          catch (const std::out_of_range&)
          {
            return -1;
          }

          const auto i = indexes.find({vvi, vti, vni});
          if(i!=indexes.end())
            return i->second;

          tp_math_utils::Vertex3D v;

          if(vvi>=objVV.size())
            return -1;

          v.vert = objVV.at(vvi);

          if(vti<objVT.size())
            v.texture = objVT.at(vti);

          if(vni<objVN.size())
            v.normal = objVN.at(vni);

          int index = int(o.verts.size());
          indexes[{vvi, vti, vni}] = index;
          o.verts.push_back(v);
          return index;
        };

        int a = parseAddVert(parts.at(1));
        int b = parseAddVert(parts.at(2));
        int c = parseAddVert(parts.at(3));

        if(a<0 || b<0 || c<0)
          continue;

        f.indexes.push_back(a);
        f.indexes.push_back(b);
        f.indexes.push_back(c);

        // If its a quad we need to add an extra polygon. It looks like faces can contain an
        // arbitrary number of points but im not sure what the rule is for triangulating them.
        if(parts.size()>4)
        {
          int d = parseAddVert(parts.at(4));
          if(d<0)
            continue;

          f.indexes.push_back(c);
          f.indexes.push_back(d);
          f.indexes.push_back(a);
        }
      }
    }
  }

  return true;
}

//##################################################################################################
bool parseMTL(const std::string& filePath,
              std::string& error,
              std::vector<tp_math_utils::Material>& outputMaterials)
{
  TP_UNUSED(error);

  std::vector<std::vector<std::string>> lines = parseLines(filePath);
  for(const auto& parts : lines)
  {
    std::string c = parts.front();

    if(c == "newmtl")
    {
      auto& m = outputMaterials.emplace_back();
      m.name = joinName(parts);

      if(!m.name.isValid())
        m.name = "none";

      continue;
    }

    if(outputMaterials.empty())
      continue;

    auto& m = outputMaterials.back();

    auto openGLMaterial = m.findOrAddOpenGL();
    auto legacyMaterial = m.findOrAddLegacy();

    auto boolProperty = [&](auto key, bool& value)
    {
      if(c != key)
        return false;

      if(parts.size() == 2)
        value = readBool(parts[1]);

      return true;
    };

    auto floatProperty = [&](auto key, float& value)
    {
      if(c != key)
        return false;

      if(parts.size() == 2)
        value = readFloat(parts[1]);

      return true;
    };

//    auto intProperty = [&](auto key, int& value)
//    {
//      if(c != key)
//        return false;

//      if(parts.size() == 2)
//        value = readInt(parts[1]);

//      return true;
//    };

    auto sssMethodProperty = [&](auto key, tp_math_utils::SSSMethod& value)
    {
      if(c != key)
        return false;

      if(parts.size() == 2)
        value = tp_math_utils::SSSMethod(readInt(parts[1]));

      return true;
    };

    auto ignoreFloatProperty = [&](auto key)
    {
      return (c == key);
    };

    auto vec3Property = [&](auto key, glm::vec3& value)
    {
      if(c != key)
        return false;

      if(parts.size() == 4)
      {
        value.x = readFloat(parts[1]);
        value.y = readFloat(parts[2]);
        value.z = readFloat(parts[3]);
      }

      return true;
    };

    auto ignoreVec3Property = [&](auto key)
    {
      return (c == key);
    };

    auto mapProperty = [&](auto key, tp_utils::StringID& value)
    {
      if(c != key)
        return false;

      value = splitTextureOptions(joinName(parts)).file;
      return true;
    };

    auto ignoreMapProperty = [&](auto key)
    {
      return (c == key);
    };

    if     ( ignoreVec3Property("Ka"                                      )){} // Ambient Color
    else if(       vec3Property("Kd"      , openGLMaterial->albedo        )){} // Diffuse Color
    else if( ignoreVec3Property("Ks"                                      )){} // Specular Color
    else if(ignoreFloatProperty("Ni"                                      )){} // Optical Density
    else if(      floatProperty("d"       , openGLMaterial->alpha         )){} // Dissolve
    else if(        mapProperty("map_Kd"  , openGLMaterial->albedoTexture )){} // Diffuse Texture Map
    else if(  ignoreMapProperty("map_Ks"                                  )){} // Specular Texture Map
    else if(  ignoreMapProperty("map_Ns"                                  )){} // Specular Hightlight Map
    else if(        mapProperty("map_d"   , openGLMaterial->alphaTexture  )){} // Alpha Texture Map
    else if(        mapProperty("map_Bump", openGLMaterial->normalsTexture)){} // Bump Map
    else if(        mapProperty("map_bump", openGLMaterial->normalsTexture)){} // Bump Map
    else if(        mapProperty("bump"    , openGLMaterial->normalsTexture)){} // Bump Map
    else if(        mapProperty("norm"    , openGLMaterial->normalsTexture)){} // Bump Map
    else if(  ignoreMapProperty("map_ao"                                  )){} // Alpha Texture Map

    // Ambient Texture Map
    else if(c == "map_Ka")
    {
      if(!openGLMaterial->albedoTexture.isValid())
        openGLMaterial->albedoTexture = joinName(parts);
    }

    // Specular Exponent
    else if(c == "Ns")
    {
      if(parts.size() != 2)
        continue;

      //m.specular = readFloat(parts[1]);
      //m.roughness = std::sqrt(2.0f/(2.0f+float(m.specular)));
    }

    // Illumination
    else if(c == "illum")
    {
      if(parts.size() != 2)
        continue;

      //illum = std::stoi(parts[1]);
    }

    //-- Extended material properties --------------------------------------------------------------
    else if(    floatProperty("Roughness"                   , openGLMaterial->roughness                   )){}
    else if(    floatProperty("Metalness"                   , openGLMaterial->metalness                   )){}
    else if(    floatProperty("Specular"                    , legacyMaterial->specular                    )){}
    else if(     vec3Property("Emission"                    , legacyMaterial->emission                    )){}
    else if(    floatProperty("EmissionStrength"            , legacyMaterial->emissionScale               )){}
    else if(     vec3Property("Subsurface"                  , legacyMaterial->sss                         )){}
    else if(    floatProperty("SubsurfaceScale"             , legacyMaterial->sssScale                    )){}
    else if(     vec3Property("SubsurfaceRadius"            , legacyMaterial->sssRadius                   )){}
    else if(sssMethodProperty("SubsurfaceMethod"            , legacyMaterial->sssMethod                   )){}
    else if(    floatProperty("NormalStrength"              , legacyMaterial->normalStrength              )){}
    else if(    floatProperty("Transmission"                , openGLMaterial->transmission                )){}
    else if(    floatProperty("TransmissionRoughness"       , openGLMaterial->transmissionRoughness       )){}
    else if(    floatProperty("Sheen"                       , legacyMaterial->sheen                       )){}
    else if(    floatProperty("SheenTint"                   , legacyMaterial->sheenTint                   )){}
    else if(    floatProperty("ClearCoat"                   , legacyMaterial->clearCoat                   )){}
    else if(    floatProperty("ClearCoatRoughness"          , legacyMaterial->clearCoatRoughness          )){}
    else if(    floatProperty("IOR"                         , legacyMaterial->ior                         )){}
    else if(    floatProperty("albedoBrightness"            , openGLMaterial->albedoBrightness            )){}
    else if(    floatProperty("albedoContrast"              , openGLMaterial->albedoContrast              )){}
    else if(    floatProperty("albedoGamma"                 , openGLMaterial->albedoGamma                 )){}
    else if(    floatProperty("albedoHue"                   , openGLMaterial->albedoHue                   )){}
    else if(    floatProperty("albedoSaturation"            , openGLMaterial->albedoSaturation            )){}
    else if(    floatProperty("albedoValue"                 , openGLMaterial->albedoValue                 )){}
    else if(    floatProperty("albedoFactor"                , openGLMaterial->albedoFactor                )){}
    else if(     boolProperty("rayVisibilityCamera"         , legacyMaterial->rayVisibilityCamera         )){}
    else if(     boolProperty("rayVisibilityDiffuse"        , legacyMaterial->rayVisibilityDiffuse        )){}
    else if(     boolProperty("rayVisibilityGlossy"         , legacyMaterial->rayVisibilityGlossy         )){}
    else if(     boolProperty("rayVisibilityTransmission"   , legacyMaterial->rayVisibilityTransmission   )){}
    else if(     boolProperty("rayVisibilityScatter"        , legacyMaterial->rayVisibilityScatter        )){}
    else if(     boolProperty("rayVisibilityShadow"         , legacyMaterial->rayVisibilityShadow         )){}
    else if(     boolProperty("rayVisibilityShadowCatcher"  , openGLMaterial->rayVisibilityShadowCatcher  )){}
    else if(      mapProperty("map_ClearCoat"               , legacyMaterial->clearCoatTexture            )){}
    else if(      mapProperty("map_ClearCoatRoughness"      , legacyMaterial->clearCoatRoughnessTexture   )){}
    else if(      mapProperty("map_Emission"                , legacyMaterial->emissionTexture             )){}
    else if(      mapProperty("map_Metalness"               , openGLMaterial->metalnessTexture            )){}
    else if(      mapProperty("map_Roughness"               , openGLMaterial->roughnessTexture            )){}
    else if(      mapProperty("map_Sheen"                   , legacyMaterial->sheenTexture                )){}
    else if(      mapProperty("map_SheenTint"               , legacyMaterial->sheenTintTexture            )){}
    else if(      mapProperty("map_Specular"                , legacyMaterial->specularTexture             )){}
    else if(      mapProperty("map_Subsurface"              , legacyMaterial->sssTexture                  )){}
    else if(      mapProperty("map_SubsurfaceScale"         , legacyMaterial->sssScaleTexture             )){}
    else if(      mapProperty("map_Transmission"            , openGLMaterial->transmissionTexture         )){}
    else if(      mapProperty("map_TransmissionRoughness"   , openGLMaterial->transmissionRoughnessTexture)){}
  }

  return true;
}

}
