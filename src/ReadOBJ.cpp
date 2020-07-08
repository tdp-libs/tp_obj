#include "tp_obj/ReadOBJ.h"

#include "tp_math_utils/Geometry3D.h"

#include "tp_utils/DebugUtils.h"

#include "OBJ_Loader.h"

//#include<memory>
//#include <fstream>
//#include <type_traits>
//#include <cstring>

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
  std::ifstream ss(filePath, std::ios::binary);
  if(ss.fail())
  {
    error = "failed to open: " + filePath;
    return;
  }

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
  TP_UNUSED(reverse);


//  {
//    size_t vertCount=0;
//    for(const auto& mesh : loader.LoadedMeshes)
//      vertCount+= mesh.Vertices.size();
//    outputGeometry.verts.resize(vertCount);
//  }

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
      outVert->texture = {v.TextureCoordinate.X, v.TextureCoordinate.Y};
      outVert->color   = {1.0f, 1.0f, 1.0f, 1.0f};
      outVert++;
    }

    // Go through every 3rd index and print the
    //	triangle that these indices represent
    auto& indexes = outMesh.geometry.indexes.emplace_back();
    indexes.type = outMesh.geometry.triangles;
    indexes.indexes.resize(mesh.Indices.size());
    auto outIndex = indexes.indexes.data();
    for (const auto& index : mesh.Indices)
    {
      *outIndex = int(index);
      outIndex++;
    }

    outMesh.material.ambient   = {mesh.MeshMaterial.Ka.X, mesh.MeshMaterial.Ka.Y, mesh.MeshMaterial.Ka.Z};
    outMesh.material.diffuse   = {mesh.MeshMaterial.Kd.X, mesh.MeshMaterial.Kd.Y, mesh.MeshMaterial.Kd.Z};
    outMesh.material.specular  = {mesh.MeshMaterial.Ks.X, mesh.MeshMaterial.Ks.Y, mesh.MeshMaterial.Ks.Z};
    outMesh.material.shininess = mesh.MeshMaterial.Ns;

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








  //  try
  //  {
  //    tinyobj::PlyFile file;
  //    file.parse_header(inputStream);

  //    outputGeometry.comments = file.get_comments();


  //    ElementDetails_lt vertexDetails;
  //    ElementDetails_lt faceDetails;
  //    ElementDetails_lt tristripsDetails;
  //    ElementDetails_lt trifanDetails;

  //    faceDetails     .geometryType = triangles;
  //    tristripsDetails.geometryType = triangleStrip;
  //    trifanDetails   .geometryType = triangleFan;

  //    //-- Parse the header --------------------------------------------------------------------------
  //    bool printProperties = false;
  //    for (const auto& e : file.get_elements())
  //    {
  //      if(printProperties)
  //      {
  //        tpWarning() << "element - " << e.name << " (" << e.size << ")" << std::endl;
  //        for(const auto& p : e.properties)
  //          tpWarning() << "\tproperty - " << p.name << " (" << tinyobj::PropertyTable[p.propertyType].str << ")" << std::endl;
  //      }

  //      if(e.name == "vertex")
  //        parseElement(file, error, e, vertexDetails);

  //      else if(e.name == "face")
  //        parseElement(file, error, e, faceDetails);

  //      else if(e.name == "tristrips")
  //        parseElement(file, error, e, tristripsDetails);
  //    }

  //    //-- Read the requested properties -------------------------------------------------------------
  //    file.read(inputStream);

  //    //-- Read in the verts -------------------------------------------------------------------------
  //    if(!readVertices(vertexDetails.vertices, outputGeometry))
  //    {
  //      error = "Error reading vertices.";
  //      return;
  //    }

  //    //-- Read in the other vertex properties -------------------------------------------------------
  //    readNormals (vertexDetails.normals  , outputGeometry);
  //    readTextures(vertexDetails.texcoords, outputGeometry);

  //    //-- Read in the faces -------------------------------------------------------------------------
  //    readFaces(error,      faceDetails, reverse, outputGeometry);
  //    readFaces(error, tristripsDetails, reverse, outputGeometry);
  //  }
  //  catch (const std::exception & e)
  //  {
  //    error = std::string("Caught tinyobj exception: ") + e.what();
  //  }
}

}
