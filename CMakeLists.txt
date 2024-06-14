cmake_minimum_required(VERSION 3.5)
project(giewont)
set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 20)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (EMSCRIPTEN)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s USE_GLFW=3 -s ASSERTIONS=1 -s WASM=1  -s GL_ENABLE_GET_PROC_ADDRESS=1")
    set(CMAKE_EXECUTABLE_SUFFIX ".html") # This line is used to set your executable to build with the emscripten html template so that you can directly open it.
endif ()

set(RAYLIB_VERSION 5.0)
find_package(raylib ${RAYLIB_VERSION} QUIET) # QUIET or REQUIRED

find_package(LibDataChannel REQUIRED)

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


# Add nlohmann/json
add_subdirectory(libs/json)

# List common sources between client and server
set(COMMON_SOURCES
    src/CameraEntity.cpp
    src/CharacterEntity.cpp
    src/Entity.cpp
    src/Game.cpp
    src/LevelLoader.cpp
    src/nbnet.cpp
    src/PhysEntity.cpp
    src/ResourceManager.cpp
    src/SpawnEntity.cpp
    src/TilemapEntity.cpp
)
set(COMMON_INCLUDES
    "${CMAKE_CURRENT_LIST_DIR}/src/"  
    "${CMAKE_CURRENT_LIST_DIR}/src/math"
    "${CMAKE_CURRENT_LIST_DIR}/src/net"
) 

### Client build

set(CLIENT_EXE_NAME "giewont_client")

set(CLIENT_SOURCES 
    src/client/main_client.cpp
    src/client/ClientGame.cpp
)

set (CLIENT_INCLUDES
    "${CMAKE_CURRENT_LIST_DIR}/src/client"
)

add_executable(${CLIENT_EXE_NAME})
target_compile_definitions(${CLIENT_EXE_NAME} PUBLIC -DGIEWONT_IS_CLIENT -DGIEWONT_HAS_GRAPHICS)
target_sources(${CLIENT_EXE_NAME} PRIVATE ${COMMON_SOURCES} ${CLIENT_SOURCES})
target_include_directories(${CLIENT_EXE_NAME} PRIVATE ${COMMON_INCLUDES} ${CLIENT_INCLUDES})
target_include_directories(${CLIENT_EXE_NAME} PRIVATE "${CMAKE_CURRENT_LIST_DIR}/libs/nbnet")

target_link_libraries(${CLIENT_EXE_NAME} PRIVATE raylib nlohmann_json::nlohmann_json)
target_compile_options(${CLIENT_EXE_NAME} PRIVATE -ggdb)

if (EMSCRIPTEN)
    target_compile_options(${CLIENT_EXE_NAME} PRIVATE -fwasm-exceptions)
    target_link_options(${CLIENT_EXE_NAME} PRIVATE -fwasm-exceptions --embed-file ${CMAKE_CURRENT_SOURCE_DIR}/assets@/assets)
    target_compile_definitions(${CLIENT_EXE_NAME} PUBLIC -DGIEWONT_ASSETS_PATH="/assets/")
    set_target_properties(${CLIENT_EXE_NAME} PROPERTIES OUTPUT_NAME "index")
else()
    target_compile_definitions(${CLIENT_EXE_NAME} PUBLIC -DGIEWONT_ASSETS_PATH="../assets/")
    target_link_libraries(${CLIENT_EXE_NAME} PUBLIC LibDataChannel::LibDataChannel)
endif()


### Server build

set(SERVER_EXE_NAME "giewont_server")

set(SERVER_SOURCES 
    src/server/main_server.cpp
    src/server/ServerGame.cpp
)

set (SERVER_INCLUDES
    "${CMAKE_CURRENT_LIST_DIR}/src/server"
)

add_executable(${SERVER_EXE_NAME})
target_compile_definitions(${SERVER_EXE_NAME} PUBLIC -DGIEWONT_IS_SERVER)
target_sources(${SERVER_EXE_NAME} PRIVATE ${COMMON_SOURCES} ${SERVER_SOURCES})
target_include_directories(${SERVER_EXE_NAME} PRIVATE ${COMMON_INCLUDES} ${SERVER_INCLUDES})
target_include_directories(${SERVER_EXE_NAME} PRIVATE "${CMAKE_CURRENT_LIST_DIR}/libs/nbnet")

target_link_libraries(${SERVER_EXE_NAME} PRIVATE nlohmann_json::nlohmann_json LibDataChannel::LibDataChannel)
target_compile_options(${SERVER_EXE_NAME} PRIVATE -ggdb)

