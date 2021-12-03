#include "tp_obj/OBJParser.h"

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
float readFloat(const std::string& s)
{
  float v;
  std::istringstream istr(s);
  istr.imbue(std::locale("C"));
  istr >> v;
  return v;
}

}

//##################################################################################################
std::vector<std::vector<std::string>> parseLines(const std::string& filePath)
{
  std::vector<std::vector<std::string>> lines;

  auto splitLine = [](std::string line)
  {
    std::vector<std::string> parts;
    if(auto i=line.find_first_of('#'); i!=std::string::npos)
      line.erase(i);
    line.erase(0, line.find_first_not_of(" \t\n\r\f\v"));

    tpSplit(parts, line, ' ', tp_utils::SplitBehavior::SkipEmptyParts);
    return parts;
  };

  std::vector<std::string> rawLines;
  std::string text = tp_utils::readTextFile(filePath);
  tpRemoveChar(text, '\r');
  tpSplit(rawLines, text, '\n', tp_utils::SplitBehavior::SkipEmptyParts);

  lines.reserve(rawLines.size());
  for(const auto& line : rawLines)
    if(std::vector<std::string> parts = splitLine(line); !parts.empty())
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
              std::vector<tp_math_utils::Geometry3D>& outputGeometry)
{
  auto barf = [&](auto msg)
  {
    error += "Parse OBJ error: ";
    error += msg;
    return false;
  };

  std::vector<std::vector<std::string>> lines = parseLines(filePath);

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
            o.comments.push_back(objectName);

          if(!materialName.empty())
            o.comments.push_back(materialName);

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
            tpSplit(indexes, part, '/', tp_utils::SplitBehavior::KeepEmptyParts);

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

    auto floatProperty = [&](auto key, float& value)
    {
      if(c != key)
        return false;

      if(parts.size() == 2)
        value = readFloat(parts[1]);

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

      if(parts.size() == 2)
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

    if     ( ignoreVec3Property("Ka"                        )){} // Ambient Color
    else if(       vec3Property("Kd"      , m.albedo        )){} // Diffuse Color
    else if( ignoreVec3Property("Ks"                        )){} // Specular Color
    else if(ignoreFloatProperty("Ni"                        )){} // Optical Density
    else if(      floatProperty("d"       , m.alpha         )){} // Dissolve
    else if(        mapProperty("map_Kd"  , m.albedoTexture )){} // Diffuse Texture Map
    else if(  ignoreMapProperty("map_Ks"                    )){} // Specular Texture Map
    else if(  ignoreMapProperty("map_Ns"                    )){} // Specular Hightlight Map
    else if(        mapProperty("map_d"   , m.alphaTexture  )){} // Alpha Texture Map
    else if(        mapProperty("map_Bump", m.normalsTexture)){} // Bump Map
    else if(        mapProperty("map_bump", m.normalsTexture)){} // Bump Map
    else if(        mapProperty("bump"    , m.normalsTexture)){} // Bump Map
    else if(        mapProperty("norm"    , m.normalsTexture)){} // Bump Map
    else if(  ignoreMapProperty("map_ao"                    )){} // Alpha Texture Map

    // Ambient Texture Map
    else if(c == "map_Ka")
    {
      if(!m.albedoTexture.isValid())
        m.albedoTexture = joinName(parts);
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
    else if(floatProperty("Roughness"                , m.roughness                   )){}
    else if(floatProperty("Metalness"                , m.metalness                   )){}
    else if(floatProperty("Specular"                 , m.specular                    )){}
    else if( vec3Property("Emission"                 , m.emission                    )){}
    else if( vec3Property("Subsurface"               , m.sss                         )){}
    else if(floatProperty("SubsurfaceScale"          , m.sssScale                    )){}
    else if(floatProperty("Transmission"             , m.transmission                )){}
    else if(floatProperty("TransmissionRoughness"    , m.transmissionRoughness       )){}
    else if(floatProperty("Sheen"                    , m.sheen                       )){}
    else if(floatProperty("SheenTint"                , m.sheenTint                   )){}
    else if(floatProperty("ClearCoat"                , m.clearCoat                   )){}
    else if(floatProperty("ClearCoatRoughness"       , m.clearCoatRoughness          )){}
    else if(floatProperty("IOR"                      , m.ior                         )){}
    else if(floatProperty("albedoBrightness"         , m.albedoBrightness            )){}
    else if(floatProperty("albedoContrast"           , m.albedoContrast              )){}
    else if(floatProperty("albedoGamma"              , m.albedoGamma                 )){}
    else if(floatProperty("albedoHue"                , m.albedoHue                   )){}
    else if(floatProperty("albedoSaturation"         , m.albedoSaturation            )){}
    else if(floatProperty("albedoValue"              , m.albedoValue                 )){}
    else if(floatProperty("albedoFactor"             , m.albedoFactor                )){}
    else if(  mapProperty("map_ClearCoat"            , m.clearCoatTexture            )){}
    else if(  mapProperty("map_ClearCoatRoughness"   , m.clearCoatRoughnessTexture   )){}
    else if(  mapProperty("map_Emission"             , m.emissionTexture             )){}
    else if(  mapProperty("map_Metalness"            , m.metalnessTexture            )){}
    else if(  mapProperty("map_Roughness"            , m.roughnessTexture            )){}
    else if(  mapProperty("map_Sheen"                , m.sheenTexture                )){}
    else if(  mapProperty("map_SheenTint"            , m.sheenTintTexture            )){}
    else if(  mapProperty("map_Specular"             , m.specularTexture             )){}
    else if(  mapProperty("map_Subsurface"           , m.sssTexture                  )){}
    else if(  mapProperty("map_SubsurfaceScale"      , m.sssScaleTexture             )){}
    else if(  mapProperty("map_Transmission"         , m.transmissionTexture         )){}
    else if(  mapProperty("map_TransmissionRoughness", m.transmissionRoughnessTexture)){}
  }

  return true;
}

}
