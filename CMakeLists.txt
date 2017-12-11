cmake_minimum_required (VERSION 2.6)
project (MemReaderModule C)

# Lua
find_package (Lua REQUIRED)
find_library (Psapi psapi Psapi)
find_library (Version version Version)

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
