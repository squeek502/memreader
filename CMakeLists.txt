cmake_minimum_required (VERSION 2.6)
project (MemReaderModule)

# Lua
find_package (Lua REQUIRED)

# Our Module
add_library( memreader MODULE memreader.c memreader.def )
target_link_libraries ( memreader ${LUA_LIBRARIES} Psapi )
target_include_directories( memreader PRIVATE ${LUA_INCLUDE_DIR} )

#add_executable( memreadertest memreader.c )
#target_link_libraries ( memreadertest ${LUA_LIBRARIES} Psapi )
#target_include_directories( memreadertest PRIVATE ${LUA_INCLUDE_DIR} )
