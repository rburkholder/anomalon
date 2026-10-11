cmake_minimum_required(VERSION 3.20)
PROJECT(anomalon LANGUAGES CXX)

if (POLICY CMP0167)
  cmake_policy(SET CMP0167 NEW)
endif()

# Force modern C++20 standard requirements
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Determine the build mode (default to Release for max performance)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
endif()

# ---------------------------------------------------------------------------
# Find Required Packages
# ---------------------------------------------------------------------------

# 1. Find OpenSSL (Required for TLS/SSL encryption in Beast)
find_package(OpenSSL REQUIRED)

# 2. Find Boost (Beast is header-only, but its Asio backend needs compiled targets)
#    We require context/coroutine if your Asio layout needs deep stack traits,
#    but raw C++20 coroutines mostly rely on the core system threads.
find_package(Boost REQUIRED COMPONENTS system)

# 3. Find Lua (sol2 acts as the binder wrapping around the raw Lua C libraries)
#find_package(Lua REQUIRED)

# 4. Find sol2 (Modern C++ Lua Binding Library)
#    If sol2 is installed via a package manager like vcpkg or apt, use find_package.
#    Alternatively, if you are treating sol2 as a header-only submodule,
#    you can point to its include path directly using target_include_directories instead.
#find_package(sol2 CONFIG REQUIRED)

# ---------------------------------------------------------------------------
# Define Engine Target and Source Layout
# ---------------------------------------------------------------------------

add_executable(
  ${PROJECT_NAME}
    src/main.cpp
)

# ---------------------------------------------------------------------------
# Header Include Paths
# ---------------------------------------------------------------------------

#target_include_directories(anomalon PRIVATE
#    include
#    ${LUA_INCLUDE_DIR}
#)

# ---------------------------------------------------------------------------
# Link Dependency Libraries
# ---------------------------------------------------------------------------

target_compile_definitions( ${PROJECT_NAME} PUBLIC SOL_LUAJIT=1 )
target_compile_definitions( ${PROJECT_NAME} PUBLIC SOL_ALL_SAFETIES_ON=1 )

target_include_directories(
  ${PROJECT_NAME} PUBLIC
    /usr/local/include/luajit-2.1
  )

target_link_directories(
  ${PROJECT_NAME} PUBLIC
    /usr/local/lib
  )

target_link_libraries(
  ${PROJECT_NAME} PRIVATE
    OpenSSL::SSL
    OpenSSL::Crypto
    luajit-5.1
    ${Boost_LIBRARIES}
    pthread
)

# ---------------------------------------------------------------------------
# Compiler Flags Configuration (Platform Specific)
# ---------------------------------------------------------------------------

  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_compile_options(anomalon PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -O3             # Aggressive speed optimization for production
        -march=native   # Optimizes binary specifically for your host CPU architecture
    )
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_options(anomalon PRIVATE
        /W4             # High warning thresholds for Visual Studio
        /O2             # Maximize execution speed
        /permissive-    # Enforces strict standard C++ conformance
    )
endif()

# ---------------------------------------------------------------------------
# Post-Build Asset Copy Rule
# ---------------------------------------------------------------------------
# Automatically clones the server.lua scripts folder over to your build output
# directory so Anomalon can find the file immediately when you run the binary.
add_custom_command(TARGET anomalon POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/scripts
    $<TARGET_FILE_DIR:anomalon>/scripts
)
