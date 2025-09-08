#pragma once

#include "tp_obj/Globals.h"

#include "tp_math_utils/Geometry3D.h"

namespace objl
{
class Loader;
}

namespace tp_utils
{
class Progress;
}

namespace tp_obj
{

//##################################################################################################
bool TP_OBJ_EXPORT readOBJFile(const std::string& filePath,
                               int triangleFan,
                               int triangleStrip,
                               int triangles,
                               bool reverse,
                               std::string& exporterVersion,
                               std::vector<tp_math_utils::Geometry3D>& outputGeometry,
                               tp_utils::Progress* progress);

//##################################################################################################
std::string TP_OBJ_EXPORT getAssociatedFilePath(const std::string& objFilePath,
                                                const std::string& associatedFileName);

}
