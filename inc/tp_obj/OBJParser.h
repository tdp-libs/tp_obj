#pragma once

#include "tp_obj/Globals.h"

#include "tp_math_utils/Geometry3D.h"

namespace tp_utils
{
class Progress;
}

namespace tp_obj
{

//##################################################################################################
//! Read the file, split lines, read exporter version number, remove comments
std::vector<std::vector<std::string>> parseLines(const std::string& filePath,
                                                 std::string* exporterVersion=nullptr);

//##################################################################################################
bool TP_OBJ_EXPORT parseOBJ(const std::string& filePath,
                            int triangleFan,
                            int triangleStrip,
                            int triangles,
                            bool reverse,
                            std::string& exporterVersion,
                            std::vector<tp_math_utils::Geometry3D>& outputGeometry,
                            tp_utils::Progress* progress);

//##################################################################################################
bool TP_OBJ_EXPORT parseMTL(const std::string& filePath,
                            std::vector<tp_math_utils::Material>& outputMaterials,
                            tp_utils::Progress* progress);


}
