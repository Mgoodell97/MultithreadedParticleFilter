cmake_minimum_required(VERSION 3.20)
project(MultiThreadedPF LANGUAGES CXX)

# ============================
#  Build options
# ============================
option(TRACY_ENABLE "Enable Tracy profiler" OFF)
option(SANITIZE "Enable ThreadSanitizer (clang only)" OFF)

# ============================
#  Coverage option
# ============================
option(COVERAGE "Enable coverage reporting" OFF)

# ============================
#  Compiler settings
# ============================
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ============================
#  Project structure
# ============================
set(SRC_DIR "${CMAKE_SOURCE_DIR}/src")
file(GLOB SRC_FILES "${SRC_DIR}/*.cpp")

set(MAIN_SRC "${SRC_DIR}/main.cpp")
list(REMOVE_ITEM SRC_FILES "${MAIN_SRC}")
set(LIB_SRC ${SRC_FILES})

# ============================
#  Tracy
# ============================
if(TRACY_ENABLE)
    message(STATUS "Tracy enabled")

    set(TRACY_DIR "${CMAKE_SOURCE_DIR}/third_party/tracy/public")
    set(TRACY_SRC "${TRACY_DIR}/TracyClient.cpp")

    add_definitions(-DTRACY_ENABLE)
endif()

# ============================
#  Sanitizer (clang only)
# ============================
if(SANITIZE AND CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(STATUS "ThreadSanitizer enabled")
    add_compile_options(-fsanitize=thread)
    add_link_options(-fsanitize=thread)
endif()

# ============================
#  Coverage option
# ============================
if(COVERAGE)
    if(MINGW)
        message(STATUS "Coverage only available on Linux/macOS (MinGW detected)")
    else()
        message(STATUS "Coverage enabled (Linux/macOS)")
        add_compile_options(--coverage -O0 -g -fprofile-update=atomic)
        add_link_options(--coverage)
    endif()
endif()

# ============================
#  Main executable
# ============================
add_executable(particle_filter
    ${MAIN_SRC}
    ${LIB_SRC}
    ${TRACY_SRC}
)

if(TRACY_ENABLE)
    target_include_directories(particle_filter PRIVATE ${TRACY_DIR})
endif()

if(MINGW AND TRACY_ENABLE)
    target_link_libraries(particle_filter PRIVATE ws2_32 dbghelp)
endif()

if(NOT MINGW)
    target_link_libraries(particle_filter PRIVATE pthread)
endif()

# ============================
#  GoogleTest
# ============================
set(GTEST_DIR "${CMAKE_SOURCE_DIR}/third_party/googletest/googletest")
file(GLOB TEST_SRC "${SRC_DIR}/tests/*.cpp")

add_executable(tests
    ${TEST_SRC}
    ${LIB_SRC}
    ${GTEST_DIR}/src/gtest-all.cc
    ${GTEST_DIR}/src/gtest_main.cc
)

target_include_directories(tests PRIVATE
    ${GTEST_DIR}
    ${GTEST_DIR}/include
    ${SRC_DIR}
)

# Tracy for tests
if(TRACY_ENABLE)
    target_sources(tests PRIVATE ${TRACY_SRC})
    target_include_directories(tests PRIVATE ${TRACY_DIR})
endif()

if(MINGW AND TRACY_ENABLE)
    target_link_libraries(tests PRIVATE ws2_32 dbghelp)
endif()

if(NOT MINGW)
    target_link_libraries(tests PRIVATE pthread)
endif()
