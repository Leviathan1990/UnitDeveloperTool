# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Release")
  file(REMOVE_RECURSE
  "CMakeFiles/UnitDeveloperTool_autogen.dir/AutogenUsed.txt"
  "CMakeFiles/UnitDeveloperTool_autogen.dir/ParseCache.txt"
  "UnitDeveloperTool_autogen"
  )
endif()
