#include "tp_obj/WriteOBJ.h"

#include "tp_math_utils/materials/OpenGLMaterial.h"

#include "tp_utils/FileUtils.h"

#include <sstream>

namespace tp_obj
{

//##################################################################################################
std::string serializeMTL(const std::vector<tp_math_utils::Geometry3D>& geometry)
{
  std::stringstream result;

  for(const auto& mesh : geometry)
  {
    result << "newmtl " << mesh.material.name.toString() << "\n";

    auto addVec3 = [&](auto name, auto c)
    {
      result << name << " " << c.x << " " << c.y << " " << c.z << "\n";
    };

    auto addFloat = [&](auto name, auto c)
    {
      result << name << " " << c << "\n";
    };

    tp_math_utils::OpenGLMaterial::view(mesh.material, [&](const tp_math_utils::OpenGLMaterial& material)
    {
      addVec3("Kd", material.albedo);
      addFloat("d", material.alpha);
    });
  }

  return result.str();
}

//##################################################################################################
std::string serializeOBJ(const std::vector<tp_math_utils::Geometry3D>& geometry,
                         const std::string& mtlName)
{
  std::stringstream result;

  result << "mtllib " << mtlName << "\n";

  for(const auto& mesh : geometry)
  {
    for(const auto& vert : mesh.verts)
    {
      const auto v = vert.vert;
      result << std::fixed << "v " << v.x << ' ' << v.y << ' ' << v.z << '\n';
    }
  }

  for(const auto& mesh : geometry)
  {
    for(const auto& vert : mesh.verts)
    {
      const auto vt = vert.texture;
      result << std::fixed << "vt " << vt.x << ' ' << vt.y << '\n';
    }
  }

  for(const auto& mesh : geometry)
  {
    for(const auto& vert : mesh.verts)
    {
      const auto vn = vert.normal;
      result << std::fixed << "vn " << vn.x << ' ' << vn.y << ' ' << vn.z << '\n';
    }
  }


  int offset=0;
  for(const auto& mesh : geometry)
  {
    result << "usemtl " << mesh.material.name.toString() << "\n";
    for(const auto& indexes : mesh.indexes)
    {
      size_t i = 0;
      size_t iMax = indexes.indexes.size();
      for(; (i+2)<iMax; i+=3)
      {
        int i0 = indexes.indexes.at(i+0) + offset+1;
        int i1 = indexes.indexes.at(i+1) + offset+1;
        int i2 = indexes.indexes.at(i+2) + offset+1;

        result << "f " << i0 << '/' << i0 <<'/' << i0 <<
                  ' '  << i1 << '/' << i1 <<'/' << i1 <<
                  ' '  << i2 << '/' << i2 <<'/' << i2 << '\n';
      }
    }

    offset += int(mesh.verts.size());
  }

  return result.str();
}

//##################################################################################################
void writeOBJ(const std::string& filename,
              const std::vector<tp_math_utils::Geometry3D>& geometry,
              const std::string& mtlName)
{
  tp_utils::writeTextFile(filename, serializeOBJ(geometry, mtlName));
}

//##################################################################################################
void writeOBJ(const std::string& path,
              const std::string& name,
              const std::vector<tp_math_utils::Geometry3D>& geometry)
{
  std::string objName = name + ".obj";
  std::string mtlName = name + ".mtl";
  writeOBJ(tp_utils::pathAppend(path, objName), geometry, mtlName);
  writeMTL(tp_utils::pathAppend(path, mtlName), geometry);
}

//##################################################################################################
void writeMTL(const std::string& filename, const std::vector<tp_math_utils::Geometry3D>& geometry)
{
  tp_utils::writeTextFile(filename, serializeMTL(geometry));
}

}
