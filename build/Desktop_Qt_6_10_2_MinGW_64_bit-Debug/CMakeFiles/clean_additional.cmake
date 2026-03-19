# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles\\LearningQT_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\LearningQT_autogen.dir\\ParseCache.txt"
  "LearningQT_autogen"
  )
endif()
