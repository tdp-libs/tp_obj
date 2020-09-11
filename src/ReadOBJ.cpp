#include "tp_obj/ReadOBJ.h"

#include "tp_math_utils/Geometry3D.h"

#include "tp_utils/DebugUtils.h"

#include "OBJ_Loader.h"

namespace tp_obj
{

//##################################################################################################
void readOBJFile(const std::string & filePath,
                 std::string& error,
                 int triangleFan,
                 int triangleStrip,
                 int triangles,
                 bool reverse,
                 std::vector<tp_maps::Geometry3D>& outputGeometry)
{
  objl::Loader loader;

  if(!loader.LoadFile(filePath))
  {
    error = "failed to parse: " + filePath;
    return;
  }

  readOBJLoader(loader,
                error,
                triangleFan,
                triangleStrip,
                triangles,
                reverse,
                outputGeometry);
}


//##################################################################################################
void readOBJStream(std::istream& inputStream,
                   const std::function<void(const std::string&, const std::function<void(std::istream&)>&)>& loadMTL,
                   std::string& error,
                   int triangleFan,
                   int triangleStrip,
                   int triangles,
                   bool reverse,
                   std::vector<tp_maps::Geometry3D>& outputGeometry)
{
  objl::Loader loader;

  if(!loader.LoadStreams(inputStream, loadMTL))
  {
    error = "failed to parse";
    return;
  }

  readOBJLoader(loader,
                error,
                triangleFan,
                triangleStrip,
                triangles,
                reverse,
                outputGeometry);
}

//##################################################################################################
void readOBJLoader(const objl::Loader& loader,
                   std::string& error,
                   int triangleFan,
                   int triangleStrip,
                   int triangles,
                   bool reverse,
                   std::vector<tp_maps::Geometry3D>& outputGeometry)
{
  TP_UNUSED(error);

  outputGeometry.reserve(loader.LoadedMeshes.size());
  for(const auto& mesh : loader.LoadedMeshes)
  {
    auto& outMesh = outputGeometry.emplace_back();
    outMesh.geometry.triangleFan   = triangleFan;
    outMesh.geometry.triangleStrip = triangleStrip;
    outMesh.geometry.triangles     = triangles;

    outMesh.geometry.comments.push_back(mesh.MeshName);
    outMesh.geometry.verts.resize(mesh.Vertices.size());
    auto outVert = outMesh.geometry.verts.data();
    for(const auto& v : mesh.Vertices)
    {
      outVert->vert    = {v.Position.X, v.Position.Y, v.Position.Z};
      outVert->normal  = {v.Normal.X, v.Normal.Y, v.Normal.Z};
      outVert->texture = {v.TextureCoordinate.X, reverse?1.0f-v.TextureCoordinate.Y:v.TextureCoordinate.Y};
      outVert->color   = {1.0f, 1.0f, 1.0f, 1.0f};
      outVert++;
    }

    auto& indexes = outMesh.geometry.indexes.emplace_back();
    indexes.type = outMesh.geometry.triangles;
    indexes.indexes.resize(mesh.Indices.size());
    auto outIndex = indexes.indexes.data();
    for (const auto& index : mesh.Indices)
    {
      *outIndex = int(index);
      outIndex++;
    }

    outMesh.material.name = mesh.MeshMaterial.name;
    outMesh.material.ambient   = {mesh.MeshMaterial.Ka.X, mesh.MeshMaterial.Ka.Y, mesh.MeshMaterial.Ka.Z};
    outMesh.material.diffuse   = {mesh.MeshMaterial.Kd.X, mesh.MeshMaterial.Kd.Y, mesh.MeshMaterial.Kd.Z};
    outMesh.material.specular  = {mesh.MeshMaterial.Ks.X, mesh.MeshMaterial.Ks.Y, mesh.MeshMaterial.Ks.Z};
    outMesh.material.shininess = mesh.MeshMaterial.Ns;
    outMesh.material.alpha     = mesh.MeshMaterial.d;

    outMesh.material.ambientTexture = mesh.MeshMaterial.map_Ka;
    outMesh.material.diffuseTexture = mesh.MeshMaterial.map_Kd;
    outMesh.material.specularTexture = mesh.MeshMaterial.map_Ks;
    outMesh.material.alphaTexture = mesh.MeshMaterial.map_d;
    outMesh.material.bumpTexture = mesh.MeshMaterial.map_bump;

    std::cout << "Material: " << mesh.MeshMaterial.name << "\n";
    std::cout << "Ambient Color: " << mesh.MeshMaterial.Ka.X << ", " << mesh.MeshMaterial.Ka.Y << ", " << mesh.MeshMaterial.Ka.Z << "\n";
    std::cout << "Diffuse Color: " << mesh.MeshMaterial.Kd.X << ", " << mesh.MeshMaterial.Kd.Y << ", " << mesh.MeshMaterial.Kd.Z << "\n";
    std::cout << "Specular Color: " << mesh.MeshMaterial.Ks.X << ", " << mesh.MeshMaterial.Ks.Y << ", " << mesh.MeshMaterial.Ks.Z << "\n";
    std::cout << "Specular Exponent: " << mesh.MeshMaterial.Ns << "\n";
    std::cout << "Optical Density: " << mesh.MeshMaterial.Ni << "\n";
    std::cout << "Dissolve: " << mesh.MeshMaterial.d << "\n";
    std::cout << "Illumination: " << mesh.MeshMaterial.illum << "\n";
    std::cout << "Ambient Texture Map: " << mesh.MeshMaterial.map_Ka << "\n";
    std::cout << "Diffuse Texture Map: " << mesh.MeshMaterial.map_Kd << "\n";
    std::cout << "Specular Texture Map: " << mesh.MeshMaterial.map_Ks << "\n";
    std::cout << "Alpha Texture Map: " << mesh.MeshMaterial.map_d << "\n";
    std::cout << "Bump Map: " << mesh.MeshMaterial.map_bump << "\n";
  }
}

//##################################################################################################
std::string getAssociatedFilePath(const std::string& objFilePath,
                                  const std::string& associatedFileName)
{
  std::vector<std::string> temp;
  tpSplit(temp, objFilePath, '/', tp_utils::SplitBehavior::SkipEmptyParts);

  if(!temp.empty())
    temp.pop_back();

  std::string result;

  if(objFilePath[0] == '/')
    result += '/';

  result.reserve(objFilePath.size() + associatedFileName.size());
  for(const auto& part : temp)
    result += part + '/';
  result += associatedFileName;

  return result;
}

}
