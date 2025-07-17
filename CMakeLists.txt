set(CMAKE_POLICY_VERSION_MINIMUM 3.20)
cmake_minimum_required(VERSION 3.20)
project(giewont)
set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 20)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (EMSCRIPTEN)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s USE_GLFW=3 -s ASSERTIONS=1 -s WASM=1  -s GL_ENABLE_GET_PROC_ADDRESS=1")
    set(CMAKE_EXECUTABLE_SUFFIX ".html") # This line is used to set your executable to build with the emscripten html template so that you can directly open it.
endif ()

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE INTERNAL "")
endif ()

set(RAYLIB_VERSION 5.5)
find_package(raylib ${RAYLIB_VERSION} QUIET) # QUIET or REQUIRED

if (NOT raylib_FOUND) # If there's none, fetch and build raylib
    set(FETCHCONTENT_QUIET FALSE)
    include(FetchContent)
    FetchContent_Declare(
            raylib
            DOWNLOAD_EXTRACT_TIMESTAMP OFF
            URL https://github.com/raysan5/raylib/archive/refs/tags/${RAYLIB_VERSION}.tar.gz
    )
    FetchContent_GetProperties(raylib)

    if (NOT raylib_POPULATED) # Have we downloaded raylib yet?
        set(FETCHCONTENT_QUIET NO)
        set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE) # don't build the supplied examples
        set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(raylib)

        #        add_subdirectory(${raylib_SOURCE_DIR} ${raylib_BINARY_DIR})
    endif ()
endif ()


set(CMAKE_PROJECT_VERSION "AAAAA")


add_subdirectory(libs/json)
set(ENABLE_PROGRAMS OFF CACHE BOOL "" FORCE)
set(ENABLE_TESTING OFF CACHE BOOL "" FORCE)
add_compile_definitions(MBEDTLS_SSL_DTLS_SRTP=1)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    add_compile_options(-Wno-error=unterminated-string-initialization)
endif ()
set(MBEDTLS_TARGET_PREFIX "mbedtls-" CACHE STRING "" FORCE)
set(DISABLE_PACKAGE_CONFIG_AND_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(libs/mbedtls)
add_library(MbedTLS::MbedTLS ALIAS mbedtls-mbedtls)

# set(EXTERNAL_MBEDTLS_C_FLAGS "-Wno-error=unterminated-string-initialization")
# ExternalProject_Add(
#         mbedtls
#         SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/libs/mbedtls"
#         INSTALL_DIR ${CMAKE_BINARY_DIR}
#         CMAKE_ARGS 
#             -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR> 
#             -DCMAKE_C_FLAGS=${EXTERNAL_MBEDTLS_C_FLAGS} 
#             -DENABLE_TESTING:BOOL=OFF
#             -DENABLE_PROGRAMS:BOOL=OFF
#         )

function(print_all_targets DIR)
    get_property(TGTS DIRECTORY "${DIR}" PROPERTY BUILDSYSTEM_TARGETS)
    foreach(TGT IN LISTS TGTS)
        message(STATUS "Target: ${TGT}")
        # TODO: Do something about it
    endforeach()

    get_property(SUBDIRS DIRECTORY "${DIR}" PROPERTY SUBDIRECTORIES)
    foreach(SUBDIR IN LISTS SUBDIRS)
        print_all_targets("${SUBDIR}")
    endforeach()
endfunction()

print_all_targets(.)

set(NO_TESTS ON CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(INSTALL_DEPS_LIBS OFF CACHE BOOL "" FORCE)
set(NO_MEDIA ON CACHE BOOL "" FORCE)
set(NO_EXAMPLES ON CACHE BOOL "" FORCE)
set(USE_MBEDTLS ON CACHE BOOL "" FORCE)
add_subdirectory(libs/libdatachannel)


### Capnproto

set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(EXTERNAL_CAPNP ON CACHE BOOL "" FORCE)
add_compile_definitions(-DKJ_NO_COMPILER_CHECK -DKJ_USE_FIBERS=0)
add_subdirectory(libs/capnproto)
set(CAPNP_LIBRARIES "CapnProto::capnp")

capnp_generate_cpp(CAPNP_SRCS CAPNP_HDRS "${CMAKE_CURRENT_LIST_DIR}/src/net/schema.capnp")

# List common sources between client and server
set(COMMON_SOURCES
        ${CAPNP_SRCS}
        src/CameraEntity.cpp
        src/Entity.cpp
        src/Game.cpp
        src/LevelLoader.cpp
        src/nbnet.cpp
        src/PhysEntity.cpp
        src/ResourceManager.cpp
        src/SpawnEntity.cpp
        src/TilemapEntity.cpp
        src/ParticleSystemEntity.cpp
        src/BackdropEntity.cpp
        src/FlagEntity.cpp
        src/DataBinder.cpp
        src/TeamBaseEntity.cpp
        src/GameplayManager.cpp
        src/GrenadeEntity.cpp
        src/entities/character/CharacterEntity.cpp
        src/entities/character/KeyboardCharacterController.cpp
        src/entities/character/CharacterController.cpp
        src/entities/character/DumbAICharacterController.cpp
        src/entities/character/SmartAICharacterController.cpp
        src/entities/character/GoapSensors.cpp
        src/entities/character/GoapInterfaces.cpp
        src/entities/character/GoapSelectors.cpp
        src/entities/character/GoapGoals.cpp
        src/entities/character/GoapActions.cpp
        src/entities/character/GoapGoToEntityAction.cpp
        src/Util.cpp
        src/entities/gui/MainMenuGUIEntity.cpp
        src/entities/gui/TeamChoiceGUIEntity.cpp
        src/entities/decorative/TombstoneEntity.cpp
        src/math/RandUtil.cpp
        src/entities/gameplay/ProjectileEntity.cpp
        src/entities/gameplay/BonusEntity.cpp
        src/entities/debug/DebugMarkerEntity.cpp
)

set(COMMON_INCLUDES
        "${CMAKE_CURRENT_LIST_DIR}/src/"
        "${CMAKE_CURRENT_LIST_DIR}/src/math"
        "${CMAKE_CURRENT_LIST_DIR}/src/net"
        ${CAPNP_INCLUDE_DIRS}
        "${CMAKE_CURRENT_LIST_DIR}/libs/capnproto/c++/src"
        ${CMAKE_CURRENT_BINARY_DIR}/src/net # capnproto generated files
)

### Client build


add_subdirectory("${CMAKE_SOURCE_DIR}/libs/imgui" imgui EXCLUDE_FROM_ALL)
add_subdirectory("${CMAKE_SOURCE_DIR}/libs/rlImGui" rlImGui EXCLUDE_FROM_ALL)

add_compile_definitions(-DKJ_NO_COMPILER_CHECK=1)

set(CLIENT_EXE_NAME "giewont_client")

set(CLIENT_SOURCES
        src/client/main_client.cpp
        src/client/ClientGame.cpp
        src/client/DrawableGame.cpp
        src/client/DebugGUI.cpp
        
)

set(CLIENT_INCLUDES
        "${CMAKE_CURRENT_LIST_DIR}/src/client"
)

add_executable(${CLIENT_EXE_NAME}
        src/client/DrawableGame.cpp
        src/client/DrawableGame.h)
target_compile_definitions(${CLIENT_EXE_NAME} PUBLIC -DGIEWONT_IS_CLIENT -DGIEWONT_HAS_GRAPHICS)
target_sources(${CLIENT_EXE_NAME} PRIVATE ${COMMON_SOURCES} ${CLIENT_SOURCES})
target_include_directories(${CLIENT_EXE_NAME} PRIVATE ${COMMON_INCLUDES} ${CLIENT_INCLUDES})
target_include_directories(${CLIENT_EXE_NAME} PRIVATE "${CMAKE_CURRENT_LIST_DIR}/libs/nbnet")

message("CAPNP_LIBRARIES: ${CAPNP_LIBRARIES}")

target_link_libraries(${CLIENT_EXE_NAME} PRIVATE raylib nlohmann_json::nlohmann_json imgui rlImGui ${CAPNP_LIBRARIES})


# target_compile_definitions(${CLIENT_EXE_NAME} ${CAPNP_DEFINITIONS})


target_compile_options(${CLIENT_EXE_NAME} PRIVATE -ggdb)

if (EMSCRIPTEN)
    target_compile_options(${CLIENT_EXE_NAME} PRIVATE -fwasm-exceptions)
    target_link_options(${CLIENT_EXE_NAME} PRIVATE -fwasm-exceptions --embed-file ${CMAKE_CURRENT_SOURCE_DIR}/assets@/assets)
    target_compile_definitions(${CLIENT_EXE_NAME} PUBLIC -DGIEWONT_ASSETS_PATH="/assets/")
    set_target_properties(${CLIENT_EXE_NAME} PROPERTIES OUTPUT_NAME "index")
else ()
    target_compile_definitions(${CLIENT_EXE_NAME} PUBLIC -DKJ_NO_COMPILER_CHECK)
    target_link_libraries(${CLIENT_EXE_NAME} PUBLIC datachannel-static)
endif ()


### Server build
if (NOT EMSCRIPTEN)

    set(SERVER_EXE_NAME "giewont_server")

    set(SERVER_SOURCES
            src/server/main_server.cpp
            src/server/ServerGame.cpp
    )

    set(SERVER_INCLUDES
            "${CMAKE_CURRENT_LIST_DIR}/src/server"
    )

    add_executable(${SERVER_EXE_NAME})
    target_compile_definitions(${SERVER_EXE_NAME} PUBLIC -DGIEWONT_IS_SERVER)
    target_sources(${SERVER_EXE_NAME} PRIVATE ${COMMON_SOURCES} ${SERVER_SOURCES})
    target_include_directories(${SERVER_EXE_NAME} PRIVATE ${COMMON_INCLUDES} ${SERVER_INCLUDES})
    target_include_directories(${SERVER_EXE_NAME} PRIVATE "${CMAKE_CURRENT_LIST_DIR}/libs/nbnet")
    target_compile_definitions(${SERVER_EXE_NAME} PUBLIC -DKJ_NO_COMPILER_CHECK)
    # target_compile_definitions(${SERVER_EXE_NAME} ${CAPNP_DEFINITIONS})

    target_link_libraries(${SERVER_EXE_NAME} PRIVATE nlohmann_json::nlohmann_json datachannel-static ${CAPNP_LIBRARIES})

    # if using gcc...
    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(${SERVER_EXE_NAME} PRIVATE -ggdb)
    endif ()

endif ()

