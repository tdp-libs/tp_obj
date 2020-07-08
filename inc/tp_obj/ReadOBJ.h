#ifndef tp_obj_ReadOBJ_h
#define tp_obj_ReadOBJ_h

#include "tp_obj/Globals.h"
#include "tp_maps/layers/Geometry3DLayer.h"

#include <iosfwd>

namespace objl
{
class Loader;
}

namespace tp_obj
{

//##################################################################################################
void readOBJFile(const std::string & filePath,
                 std::string& error,
                 int triangleFan,
                 int triangleStrip,
                 int triangles,
                 bool reverse,
                 std::vector<tp_maps::Geometry3D>& outputGeometry);

//##################################################################################################
void readOBJStream(std::istream& inputStream,
                   const std::function<void(const std::string&, const std::function<void(std::istream&)>&)>& loadMTL,
                   std::string& error,
                   int triangleFan,
                   int triangleStrip,
                   int triangles,
                   bool reverse,
                   std::vector<tp_maps::Geometry3D>& outputGeometry);

//##################################################################################################
void readOBJLoader(const objl::Loader& loader,
                   std::string& error,
                   int triangleFan,
                   int triangleStrip,
                   int triangles,
                   bool reverse,
                   std::vector<tp_maps::Geometry3D>& outputGeometry);

}

#endif
