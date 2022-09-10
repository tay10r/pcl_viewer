#include "point_cloud_viewer.h"

#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <string.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

struct pcl_viewer_struct final
{
  GLFWwindow* window = nullptr;

  GLuint vertex_array = 0;

  GLuint vertex_buffer = 0;

  GLuint program = 0;

  GLint mvp_location = -1;

  glm::mat4 projection_transform = glm::perspective(glm::radians(45.0f), 1.0f, 0.01f, 100.0f);

  glm::mat4 view_transform = glm::lookAt(glm::vec3(2, 2, 3), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));

  glm::mat4 model_transform = glm::mat4(1.0f);

  glm::mat4 mvp() const { return projection_transform * view_transform * model_transform; }
};

namespace {

void
load_shader_source(GLuint shader, const char* source)
{
  const GLint length = static_cast<GLint>(strlen(source));

  glShaderSource(shader, 1, &source, &length);
}

const char* vert_shader_source = R"(
#version 300 es

uniform highp mat4 mvp;

layout(location = 0) in highp vec3 g_position;

layout(location = 1) in lowp vec3 g_color;

out lowp vec3 g_point_color;

void main()
{
  g_point_color = g_color;

  gl_Position = mvp * vec4(g_position, 1.0);
}
)";

const char* frag_shader_source = R"(
#version 300 es

in lowp vec3 g_point_color;

out lowp vec4 g_out_color;

void main()
{
  g_out_color = vec4(g_point_color, 1.0);
}
)";

void
print_shader_error(const char* source, const std::string& info_log, std::ostream& stream)
{
  size_t line = 1;

  const auto source_len = strlen(source);

  std::ostringstream tmp_stream;

  tmp_stream << std::setw(3);

  for (size_t i = 0; i < source_len; i++) {
    if (source[i] == '\n') {
      line++;
    }

    tmp_stream << source[i];

    if ((source[i] == '\n') && ((i + 1) < source_len)) {
      tmp_stream << ' ' << line << " | ";
    }
  }

  tmp_stream << info_log;

  stream << tmp_stream.str();
}

bool
check_shader(GLuint shader, const char* source)
{
  GLint is_compiled = GL_FALSE;

  glGetShaderiv(shader, GL_COMPILE_STATUS, &is_compiled);

  if (is_compiled == GL_TRUE)
    return true;

  GLint max_length = 0;

  glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &max_length);

  std::string info_log;

  info_log.resize(max_length);

  glGetShaderInfoLog(shader, max_length, &max_length, &info_log[0]);

  print_shader_error(source, info_log, std::cerr);

  return false;
}

void
key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
  if ((key == GLFW_KEY_ESCAPE) && (action == GLFW_PRESS))
    glfwSetWindowShouldClose(window, GLFW_TRUE);
}

} // namespace

pcl_viewer_struct*
pcl_viewer_create(const char* title)
{
  if (glfwInit() != GLFW_TRUE) {
    return nullptr;
  }

  glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
  glfwWindowHint(GLFW_SAMPLES, 4);
  glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

  auto* viewer = new pcl_viewer_struct();

  viewer->window = glfwCreateWindow(1280, 720, title, nullptr, nullptr);
  if (!viewer->window) {
    delete viewer;
    return nullptr;
  }

  glfwSetWindowUserPointer(viewer->window, viewer);

  glfwSetKeyCallback(viewer->window, key_callback);

  glfwMakeContextCurrent(viewer->window);

  gladLoadGLES2Loader((GLADloadproc)glfwGetProcAddress);

  glClearColor(0, 0, 0, 1);

  glEnable(GL_DEPTH);

  glGenVertexArrays(1, &viewer->vertex_array);

  glGenBuffers(1, &viewer->vertex_buffer);

  glBindVertexArray(viewer->vertex_array);

  glBindBuffer(GL_ARRAY_BUFFER, viewer->vertex_buffer);

  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 6, 0);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 6, reinterpret_cast<const void*>(sizeof(float) * 3));

  glBindBuffer(GL_ARRAY_BUFFER, 0);

  glBindVertexArray(0);

  GLuint vert_shader = glCreateShader(GL_VERTEX_SHADER);
  GLuint frag_shader = glCreateShader(GL_FRAGMENT_SHADER);

  load_shader_source(vert_shader, vert_shader_source);
  load_shader_source(frag_shader, frag_shader_source);

  glCompileShader(vert_shader);
  glCompileShader(frag_shader);

  if (!check_shader(vert_shader, vert_shader_source) || !check_shader(frag_shader, frag_shader_source)) {
    glDeleteShader(vert_shader);
    glDeleteShader(frag_shader);
    glfwDestroyWindow(viewer->window);
    delete viewer;
    return nullptr;
  }

  viewer->program = glCreateProgram();

  glAttachShader(viewer->program, vert_shader);
  glAttachShader(viewer->program, frag_shader);

  glLinkProgram(viewer->program);

  glDetachShader(viewer->program, frag_shader);
  glDetachShader(viewer->program, vert_shader);

  glDeleteShader(vert_shader);
  glDeleteShader(frag_shader);

  GLint link_status = GL_FALSE;

  glGetProgramiv(viewer->program, GL_LINK_STATUS, &link_status);

  if (link_status != GL_TRUE) {
    glDeleteProgram(viewer->program);
    glfwDestroyWindow(viewer->window);
    delete viewer;
    return nullptr;
  }

  glUseProgram(viewer->program);

  viewer->mvp_location = glGetUniformLocation(viewer->program, "mvp");

  return viewer;
}

void
pcl_viewer_destroy(pcl_viewer_z* viewer)
{
  if (!viewer)
    return;

  glfwMakeContextCurrent(viewer->window);

  glDisable(GL_DEPTH);

  glDeleteBuffers(1, &viewer->vertex_buffer);

  glDeleteVertexArrays(1, &viewer->vertex_array);

  glfwDestroyWindow(viewer->window);

  delete viewer;

  glfwTerminate();
}

void
pcl_viewer_get_window_size(pcl_viewer_z* viewer, int* w, int* h)
{
  glfwGetWindowSize(viewer->window, w, h);
}

void
pcl_viewer_get_framebuffer_size(pcl_viewer_z* viewer, int* w, int* h)
{
  glfwGetFramebufferSize(viewer->window, w, h);
}

void
pcl_viewer_set_background(pcl_viewer_z* viewer, float r, float g, float b)
{
  if (!viewer)
    return;

  glfwMakeContextCurrent(viewer->window);

  glClearColor(r, g, b, 1.0f);
}

void
pcl_viewer_set_model_transform(pcl_viewer_z* viewer, const float* model_transform)
{
  viewer->model_transform = glm::make_mat4x4(model_transform);
}

void
pcl_viewer_set_view_transform(pcl_viewer_z* viewer, const float* view_transform)
{
  viewer->view_transform = glm::make_mat4x4(view_transform);
}

void
pcl_viewer_look_at(pcl_viewer_z* viewer, const float* eye, const float* center, const float* up)
{
  viewer->view_transform = glm::lookAt(glm::make_vec3(eye), glm::make_vec3(center), glm::make_vec3(up));
}

void
pcl_viewer_set_projection_transform(pcl_viewer_z* viewer, const float* projection_transform)
{
  viewer->projection_transform = glm::make_mat4x4(projection_transform);
}

void
pcl_viewer_set_perspective(pcl_viewer_z* viewer, float fovy, float near, float far)
{
  int w = 0;
  int h = 0;
  pcl_viewer_get_framebuffer_size(viewer, &w, &h);
  viewer->projection_transform = glm::perspective(float(w) / float(h), fovy, near, far);
}

void
pcl_viewer_begin_frame(pcl_viewer_z* viewer)
{
  glfwMakeContextCurrent(viewer->window);

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  int w = 0;
  int h = 0;

  glfwGetFramebufferSize(viewer->window, &w, &h);

  glViewport(0, 0, w, h);
}

void
pcl_viewer_render_points(pcl_viewer_z* viewer, const float* attrib, uint32_t count)
{
  glBindBuffer(GL_ARRAY_BUFFER, viewer->vertex_buffer);

  glBufferData(GL_ARRAY_BUFFER, count * sizeof(float) * 6, attrib, GL_DYNAMIC_DRAW);

  glBindVertexArray(viewer->vertex_array);

  glUseProgram(viewer->program);

  glUniformMatrix4fv(viewer->mvp_location, 1, GL_FALSE, glm::value_ptr(viewer->mvp()));

  glDrawArrays(GL_POINTS, 0, count);

  glBindVertexArray(0);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void
pcl_viewer_end_frame(pcl_viewer_z* viewer)
{
  glfwSwapBuffers(viewer->window);
}

void
pcl_viewer_poll_input(pcl_viewer_z* viewer)
{
  glfwPollEvents();
}

int
pcl_viewer_should_close(pcl_viewer_z* viewer)
{
  if (!viewer)
    return 1;

  return glfwWindowShouldClose(viewer->window);
}
