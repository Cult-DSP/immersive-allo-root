cmake_minimum_required(VERSION 3.20)
project(immersive-allo-root)
set(APP_NAME immersive-allo-root)

# cmake scripts are kept in the cmake directory
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

# ── Build type default ──────────────────────────────────────────────────────
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "No build type set — defaulting to Release")
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# ── Compiler/platform specific flags ────────────────────────────────────────
if(MSVC)
  set(CMAKE_BUILD_PARALLEL_LEVEL $ENV{NUMBER_OF_PROCESSORS})
endif()

# ── Suppress warnings ───────────────────────────────────────────────────────
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-w)
endif()

# ── Spatial Root options ────────────────────────────────────────────────────
set(SPATIALROOT_BUILD_ENGINE   ON  CACHE BOOL "Build spatialroot_realtime engine")
set(SPATIALROOT_BUILD_OFFLINE  OFF CACHE BOOL "Don't build offline renderer")
set(SPATIALROOT_BUILD_CULT     OFF CACHE BOOL "Don't build cult-transcoder CLI")
set(SPATIALROOT_BUILD_GUI      OFF CACHE BOOL "Don't build GUI")
set(SPATIALROOT_BUILD_DEVTOOLS OFF CACHE BOOL "Don't build dev tools")

# cult-allolib needs graphics ON for our app to use DistributedApp
set(CULT_ALLOLIB_ENABLE_GRAPHICS   ON  CACHE BOOL "Enable cult-allolib graphics")
set(CULT_ALLOLIB_ENABLE_APP        ON  CACHE BOOL "Enable cult-allolib app domains")
set(CULT_ALLOLIB_ENABLE_MIDI       ON  CACHE BOOL "Enable cult-allolib MIDI (needed by App)")
set(CULT_ALLOLIB_ENABLE_DOCS       ON  CACHE BOOL "Enable cult-allolib docs (needed by PersistentConfig)")
set(ALLOLIB_BUILD_EXAMPLES         OFF CACHE BOOL "Don't build AlloLib examples")
set(ALLOLIB_BUILD_TESTS            OFF CACHE BOOL "Don't build AlloLib tests")

# Allow older subprojects that declare very old cmake_minimum_required()
if(NOT DEFINED CMAKE_POLICY_VERSION_MINIMUM)
  set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "Minimum CMake policy version to allow older subprojects" FORCE)
endif()

# ── AlloLib (via Spatial Root's cult-allolib) ───────────────────────────────
# We add SpatialRoot as the subdirectory. Its CMakeLists.txt adds
# internal/cult-allolib with the options we've set above.
# The al target is created by cult-allolib.

add_subdirectory(spatialroot)

# Fix missing include paths in cult_allolib_app (submodule's own build needs
# cpptoml and rtmidi headers to compile DistributedApp/GUIDomain sources).
if(TARGET cult_allolib_app)
    target_include_directories(cult_allolib_app PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/spatialroot/internal/cult-allolib/external/cpptoml/include
        ${CMAKE_CURRENT_SOURCE_DIR}/spatialroot/internal/cult-allolib/external/rtmidi
    )
endif()

# ── App executable ──────────────────────────────────────────────────────────
# al_ControlNav.cpp is not compiled by any cult_allolib library target but is
# needed by the App domain (al_OpenGLGraphicsDomain links against it).
set(CONTROL_NAV_SRC ${CMAKE_CURRENT_SOURCE_DIR}/spatialroot/internal/cult-allolib/src/io/al_ControlNav.cpp)

add_executable(${APP_NAME}
    src/main.cpp
    ${CONTROL_NAV_SRC}
)

target_include_directories(${APP_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/miniShader/shaderUtility
    ${CMAKE_CURRENT_SOURCE_DIR}/spatialroot/internal/cult-allolib/external/cpptoml/include
    ${CMAKE_CURRENT_SOURCE_DIR}/spatialroot/internal/cult-allolib/external/rtmidi
)

target_link_libraries(${APP_NAME} PRIVATE
    EngineSessionCore
    al
)

# ── Output directory ────────────────────────────────────────────────────────
set_target_properties(${APP_NAME} PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/bin
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_LIST_DIR}/debug
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_LIST_DIR}/bin
)
