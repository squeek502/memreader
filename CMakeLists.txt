cmake_minimum_required (VERSION 2.6)
project (MemReaderModule C)

# Lua
find_package (Lua REQUIRED)

if (UNIX OR CMAKE_HOST_UNIX)
  find_library (Psapi NAMES psapi)
  find_library (Version NAMES version)
else()
  # For some reason find_library isn't working on Windows for Psapi.lib/Version.lib
  # but just adding the Psapi/Version strings to the link libraries works fine
  set(Psapi "Psapi")
  set(Version "Version")
endif()

# Our Module
file(GLOB src src/*.h src/*.c src/*.def)
add_library( memreader MODULE ${src} )
target_link_libraries ( memreader ${LUA_LIBRARIES} ${Psapi} ${Version} )
target_include_directories( memreader PRIVATE ${LUA_INCLUDE_DIR} )
set_target_properties( memreader PROPERTIES PREFIX "" )

if (UNIX OR CMAKE_HOST_UNIX)
add_library( memreader_s STATIC ${src} )
target_link_libraries ( memreader_s ${LUA_LIBRARIES} ${Psapi} ${Version} )
target_include_directories ( memreader_s PRIVATE ${LUA_INCLUDE_DIR} )
set_target_properties ( memreader_s PROPERTIES OUTPUT_NAME memreader )
endif()

# LUA is set by Luarocks
if (LUA)
  SET(INSTALL_LIB_DIR ${LIBDIR})
elseif (UNIX OR APPLE)
  set(INSTALL_LIB_DIR "${CMAKE_INSTALL_PREFIX}/lib/lua/${LUA_VERSION_MAJOR}.${LUA_VERSION_MINOR}")
endif()

if (INSTALL_LIB_DIR)
  install(TARGETS memreader ARCHIVE DESTINATION "${INSTALL_LIB_DIR}" LIBRARY DESTINATION "${INSTALL_LIB_DIR}")
endif ()
