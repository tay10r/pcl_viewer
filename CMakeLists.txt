cmake_minimum_required(VERSION 3.14.7)

project(point_cloud_viewer VERSION 0.1.0 LANGUAGES C CXX)

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


add_library(point_cloud_viewer
  point_cloud_viewer.h
  point_cloud_viewer.cpp
  glad/include/glad/glad.h
  glad/include/KHR/khrplatform.h
  glad/src/glad.c)

target_compile_definitions(point_cloud_viewer PRIVATE GLFW_INCLUDE_NONE=1)

target_link_libraries(point_cloud_viewer PUBLIC glfw glm ${OPENGL_LIBRARIES})

target_include_directories(point_cloud_viewer
	PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}"
	PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/glad/include")


add_executable(point_cloud_viewer_test WIN32
  point_cloud_viewer_test.cpp)

target_link_libraries(point_cloud_viewer_test PRIVATE point_cloud_viewer)
