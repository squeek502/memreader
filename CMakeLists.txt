cmake_minimum_required (VERSION 2.6)
project (MemReaderModule)

# Lua
find_package (Lua REQUIRED)

# Our Module
file(GLOB src src/*.h src/*.c src/*.def)
add_library( memreader MODULE ${src} )

target_link_libraries ( memreader ${LUA_LIBRARIES} Psapi Version )
target_include_directories( memreader PRIVATE ${LUA_INCLUDE_DIR} )
