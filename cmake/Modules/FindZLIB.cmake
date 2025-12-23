# FindZLIB.cmake - Custom ZLIB finder for DSLLVM build
#
# This file provides the ZLIB::ZLIB imported target that LLVM expects
# when linking against zlib compression library.

# Create ZLIB::ZLIB imported target if it doesn't exist
if(NOT TARGET ZLIB::ZLIB)
  add_library(ZLIB::ZLIB UNKNOWN IMPORTED)

  # Set the library location
  set_target_properties(ZLIB::ZLIB PROPERTIES
    IMPORTED_LOCATION "/usr/lib/x86_64-linux-gnu/libz.so"
    INTERFACE_INCLUDE_DIRECTORIES "/usr/include"
  )

  # Also set the standard variables for compatibility
  set(ZLIB_FOUND TRUE)
  set(ZLIB_INCLUDE_DIRS "/usr/include")
  set(ZLIB_LIBRARIES "/usr/lib/x86_64-linux-gnu/libz.so")
  set(ZLIB_VERSION_STRING "1.2.11")

  message(STATUS "ZLIB::ZLIB target created for DSLLVM build")
endif()
