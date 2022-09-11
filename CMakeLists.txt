cmake_minimum_required(VERSION 3.14.7)

project(point_cloud_viewer
  VERSION 0.1.0
  LANGUAGES C CXX
  DESCRIPTION "A simple library for rendering point clouds interactively."
)

option(DATAVIZ_BUILD_DOCS "Whether or not to build the documentation." OFF)
option(DATVIZ_COMPILER_WARNINGS "Whether or not to compile with warnings." OFF)

if(DATVIZ_COMPILER_WARNINGS)
  if(CMAKE_COMPILER_IS_GNUCXX)
    set(cxx_warnings -Wall -Wextra -Werror -Wfatal-errors)
  endif(CMAKE_COMPILER_IS_GNUCXX)
endif(DATVIZ_COMPILER_WARNINGS)

######################
# Fetch Dependencies #
######################

find_package(OpenGL REQUIRED)

include(FetchContent)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_Declare(glfw URL "https://github.com/glfw/glfw/archive/refs/tags/3.3.8.zip")
FetchContent_MakeAvailable(glfw)

FetchContent_Declare(glm URL "https://github.com/g-truc/glm/archive/refs/tags/0.9.9.8.zip")
FetchContent_MakeAvailable(glm)

###################
# Declare Library #
###################

add_library(point_cloud_viewer
  datviz.h
  datviz.cpp
  datviz_glad.h
  datviz_glad.c
  datviz_khrplatform.h
)

target_compile_options(point_cloud_viewer PRIVATE ${cxx_warnings})

target_compile_definitions(point_cloud_viewer PRIVATE GLFW_INCLUDE_NONE=1)

target_link_libraries(point_cloud_viewer PUBLIC glfw glm ${OPENGL_LIBRARIES})

target_include_directories(point_cloud_viewer
  PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

########################
# Declare Test Program #
########################

add_executable(datviz_example WIN32
  example/main.cpp)

target_link_libraries(datviz_example PRIVATE point_cloud_viewer)

#############################
# Declare Doxygen Generator #
#############################

if(DATAVIZ_BUILD_DOCS)

  find_package(Doxygen REQUIRED)

  if(Doxygen_FOUND)

    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in"
      "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile")

    add_custom_target(point_cloud_viewer_docs ALL
      COMMAND ${DOXYGEN_EXECUTABLE})

  endif(Doxygen_FOUND)

endif(DATAVIZ_BUILD_DOCS)
