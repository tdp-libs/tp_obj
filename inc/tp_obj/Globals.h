#pragma once

#include "tp_utils/Globals.h"

#if defined(TP_OBJ_LIBRARY)
#  define TP_OBJ_EXPORT TP_EXPORT
#else
#  define TP_OBJ_EXPORT TP_IMPORT
#endif

//##################################################################################################
//! Load 3D models from .obj files.
namespace tp_obj
{

//##################################################################################################
struct TextureOptions
{
  std::string file;
  std::vector<std::pair<std::string, std::string>> options;
};

//##################################################################################################
TextureOptions splitTextureOptions(const std::string& in);

}
