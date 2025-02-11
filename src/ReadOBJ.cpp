#include "tp_obj/ReadOBJ.h"
#include "tp_obj/OBJParser.h"

#include "tp_math_utils/Geometry3D.h"

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
  parseOBJ(filePath, error, triangleFan, triangleStrip, triangles, reverse, exporterVersion, outputGeometry);
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
