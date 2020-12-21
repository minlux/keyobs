cmake_minimum_required (VERSION 2.6)
project(keyobs)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -std=c++17")

#sources
add_executable(keyobs
   dep/cber/ber.c
   dep/cber/snmp.c
   dep/cJSON/cJSON.c
   src/main.cpp
)

#includes
target_include_directories(keyobs PRIVATE
   dep/cber
   dep/cJSON
   src
)


#libraries
target_link_libraries(keyobs
   pthread
)

#defines
target_compile_definitions(keyobs PUBLIC
   #TODO
)
