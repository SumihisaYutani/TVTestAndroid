# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles\\TVTestAndroid_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\TVTestAndroid_autogen.dir\\ParseCache.txt"
  "TVTestAndroid_autogen"
  )
endif()
