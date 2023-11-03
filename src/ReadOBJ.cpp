#include "tp_obj/ReadOBJ.h"
#include "tp_obj/OBJParser.h"

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
                 std::string& exporterVersion,
                 std::vector<tp_math_utils::Geometry3D>& outputGeometry)
{
#if 1
  parseOBJ(filePath, error, triangleFan, triangleStrip, triangles, reverse, exporterVersion, outputGeometry);
#else
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
#endif
}


//##################################################################################################
void readOBJStream(std::istream& inputStream,
                   const std::function<void(const std::string&, const std::function<void(std::istream&)>&)>& loadMTL,
                   std::string& error,
                   int triangleFan,
                   int triangleStrip,
                   int triangles,
                   bool reverse,
                   std::vector<tp_math_utils::Geometry3D>& outputGeometry)
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
                   std::vector<tp_math_utils::Geometry3D>& outputGeometry)
{
  error += loader.errors;

  outputGeometry.reserve(loader.LoadedMeshes.size());
  for(const auto& mesh : loader.LoadedMeshes)
  {
    auto& outMesh = outputGeometry.emplace_back();
    outMesh.triangleFan   = triangleFan;
    outMesh.triangleStrip = triangleStrip;
    outMesh.triangles     = triangles;

    outMesh.comments.push_back("MESH_NAME");
    outMesh.comments.push_back(mesh.MeshName);
    outMesh.verts.resize(mesh.Vertices.size());
    auto outVert = outMesh.verts.data();
    for(const auto& v : mesh.Vertices)
    {
      outVert->vert    = {v.Position.X, v.Position.Y, v.Position.Z};
      outVert->normal  = {v.Normal.X, v.Normal.Y, v.Normal.Z};
      outVert->texture = {v.TextureCoordinate.X, reverse?1.0f-v.TextureCoordinate.Y:v.TextureCoordinate.Y};
      outVert++;
    }

    auto& indexes = outMesh.indexes.emplace_back();
    indexes.type = outMesh.triangles;
    indexes.indexes.resize(mesh.Indices.size());
    auto outIndex = indexes.indexes.data();
    for (const auto& index : mesh.Indices)
    {
      *outIndex = int(index);
      outIndex++;
    }

    outMesh.material.name = mesh.MeshMaterial.name;
    outMesh.material.albedo    = {mesh.MeshMaterial.Kd.X, mesh.MeshMaterial.Kd.Y, mesh.MeshMaterial.Kd.Z};
    outMesh.material.alpha     = mesh.MeshMaterial.d;

    outMesh.material.roughness = std::sqrt(2.0f/(2.0f+float(mesh.MeshMaterial.Ns)));

    if(!mesh.MeshMaterial.map_Kd.empty())
      outMesh.material.albedoTexture = mesh.MeshMaterial.map_Kd;
    else
      outMesh.material.albedoTexture = mesh.MeshMaterial.map_Ka;

    outMesh.material.alphaTexture    = splitTextureOptions(mesh.MeshMaterial.map_d   ).file;
    outMesh.material.normalsTexture  = splitTextureOptions(mesh.MeshMaterial.map_bump).file;

    tpWarning() << "Material: " << mesh.MeshMaterial.name << " Mesh name: " << mesh.MeshName;
    tpWarning() << "Ambient Color: " << mesh.MeshMaterial.Ka.X << ", " << mesh.MeshMaterial.Ka.Y << ", " << mesh.MeshMaterial.Ka.Z;
    tpWarning() << "Diffuse Color: " << mesh.MeshMaterial.Kd.X << ", " << mesh.MeshMaterial.Kd.Y << ", " << mesh.MeshMaterial.Kd.Z;
    tpWarning() << "Specular Color: " << mesh.MeshMaterial.Ks.X << ", " << mesh.MeshMaterial.Ks.Y << ", " << mesh.MeshMaterial.Ks.Z;
    tpWarning() << "Specular Exponent: " << mesh.MeshMaterial.Ns;
    tpWarning() << "Optical Density: " << mesh.MeshMaterial.Ni;
    tpWarning() << "Dissolve: " << mesh.MeshMaterial.d;
    tpWarning() << "Illumination: " << mesh.MeshMaterial.illum;
    tpWarning() << "Ambient Texture Map: " << mesh.MeshMaterial.map_Ka;
    tpWarning() << "Diffuse Texture Map: " << mesh.MeshMaterial.map_Kd;
    tpWarning() << "Specular Texture Map: " << mesh.MeshMaterial.map_Ks;
    tpWarning() << "Alpha Texture Map: " << mesh.MeshMaterial.map_d;
    tpWarning() << "Normals Map: " << mesh.MeshMaterial.map_bump;
  }
}

//##################################################################################################
std::string getAssociatedFilePath(const std::string& objFilePath,
                                  const std::string& associatedFileName)
{
  std::string cleanedPath = objFilePath;

  tp_utils::replace(cleanedPath, "\\\\", "/");
  tp_utils::replace(cleanedPath, "\\", "/");

  std::vector<std::string> temp;
  tpSplit(temp, cleanedPath, '/', TPSplitBehavior::SkipEmptyParts);

  if(!temp.empty())
    temp.pop_back();

  std::string result;

  if(cleanedPath[0] == '/')
    result += '/';

  result.reserve(cleanedPath.size() + associatedFileName.size());
  for(const auto& part : temp)
    result += part + '/';
  result += associatedFileName;

  return result;
}

}
