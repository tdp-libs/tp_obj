#ifndef tp_obj_WriteOBJ_h
#define tp_obj_WriteOBJ_h

#include "tp_obj/Globals.h" // IWYU pragma: keep

#include "tp_math_utils/Geometry3D.h"

namespace tp_obj
{

//##################################################################################################
std::string serializeMTL(const std::vector<tp_math_utils::Geometry3D>& geometry);

//##################################################################################################
std::string serializeOBJ(const std::vector<tp_math_utils::Geometry3D>& geometry,
                         const std::string& mtlName);

//##################################################################################################
void writeOBJ(const std::string& filename,
              const std::vector<tp_math_utils::Geometry3D>& geometry,
              const std::string& mtlName);

//##################################################################################################
void writeOBJ(const std::string& path,
              const std::string& name,
              const std::vector<tp_math_utils::Geometry3D>& geometry);

//##################################################################################################
void writeMTL(const std::string& filename,
              const std::vector<tp_math_utils::Geometry3D>& geometry);

}

#endif
