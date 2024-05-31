cmake_minimum_required(VERSION 3.5)
project(giewont)
set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 20)

if (EMSCRIPTEN)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s USE_GLFW=3 -s ASSERTIONS=1 -s WASM=1  -s GL_ENABLE_GET_PROC_ADDRESS=1")
    set(CMAKE_EXECUTABLE_SUFFIX ".html") # This line is used to set your executable to build with the emscripten html template so that you can directly open it.
   
endif ()

set(RAYLIB_VERSION 5.0)
find_package(raylib ${RAYLIB_VERSION} QUIET) # QUIET or REQUIRED

if(NOT raylib_FOUND) # If there's none, fetch and build raylib
    set(FETCHCONTENT_QUIET FALSE)
    include(FetchContent)
    FetchContent_Declare(
        raylib
        DOWNLOAD_EXTRACT_TIMESTAMP OFF
        URL https://github.com/raysan5/raylib/archive/refs/tags/${RAYLIB_VERSION}.tar.gz
    )
    FetchContent_GetProperties(raylib)

    if(NOT raylib_POPULATED) # Have we downloaded raylib yet?
        set(FETCHCONTENT_QUIET NO)
        FetchContent_Populate(raylib)
        set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE) # don't build the supplied examples
        set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
        add_subdirectory(${raylib_SOURCE_DIR} ${raylib_BINARY_DIR})
    endif()
endif()

# Adding our source files
file(GLOB_RECURSE PROJECT_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/src/*.cpp") # Define PROJECT_SOURCES as a list of all source files
set(PROJECT_INCLUDE "${CMAKE_CURRENT_LIST_DIR}/src/") # Define PROJECT_INCLUDE to be the path to the include directory of the project

# Add nlohmann/json
add_subdirectory(libs/json)


# Declaring our executable
add_executable(${PROJECT_NAME})
target_sources(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_INCLUDE})
target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_LIST_DIR}/libs/nbnet")
target_link_libraries(${PROJECT_NAME} PRIVATE raylib nlohmann_json::nlohmann_json)

if (EMSCRIPTEN)
    target_compile_options(${PROJECT_NAME} PRIVATE -fwasm-exceptions)
    target_link_options(${PROJECT_NAME} PRIVATE -fwasm-exceptions --embed-file ${CMAKE_CURRENT_SOURCE_DIR}/assets@/assets)
    target_compile_definitions(${PROJECT_NAME} PUBLIC -DGIEWONT_ASSETS_PATH="/assets/")
else()
    target_compile_definitions(${PROJECT_NAME} PUBLIC -DGIEWONT_ASSETS_PATH="../assets/")
endif()

target_compile_options(${PROJECT_NAME} PRIVATE -ggdb)
# Setting ASSETS_PATH

