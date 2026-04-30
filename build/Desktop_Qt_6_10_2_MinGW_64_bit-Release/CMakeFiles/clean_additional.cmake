# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Release")
  file(REMOVE_RECURSE
  "CMakeFiles\\Valence_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\Valence_autogen.dir\\ParseCache.txt"
  "Valence_autogen"
  )
endif()
