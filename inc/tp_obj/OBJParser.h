#ifndef tp_obj_ParseOBJ_h
#define tp_obj_ParseOBJ_h

#include "tp_obj/Globals.h"

#include "tp_math_utils/Geometry3D.h"

namespace tp_obj
{

//##################################################################################################
//! Read the file, split lines, remove comments
std::vector<std::vector<std::string>> parseLines(const std::string& filePath);

//##################################################################################################
bool parseOBJ(const std::string& filePath,
              std::string& error,
              int triangleFan,
              int triangleStrip,
              int triangles,
              bool reverse,
              std::vector<tp_math_utils::Geometry3D>& outputGeometry);

//##################################################################################################
bool parseMTL(const std::string& filePath,
              std::string& error,
              std::vector<tp_math_utils::Material>& outputMaterials);


}

#endif
