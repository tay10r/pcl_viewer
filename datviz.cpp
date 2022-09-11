#include "datviz.h"

#include "datviz_glad.h"

#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <assert.h>
#include <string.h>

namespace {

class Logger final
{
public:
  Logger(void* data, datviz_logger_callback callback)
    : data_(data)
    , callback_(callback)
  {
  }

  void log(const std::string& msg) { callback_(data_, msg.c_str(), msg.size()); }

private:
  void* data_ = nullptr;

  datviz_logger_callback callback_ = nullptr;
};

class LoggerProxy final
{
public:
  void add_logger(Logger logger) { loggers_.emplace_back(logger); }

  template<typename... Args>
  void log(Args... args)
  {
    std::ostringstream stream;

    log_impl(stream, args...);

    const auto message = stream.str();

    for (auto& l : loggers_)
      l.log(message);
  }

private:
  template<typename... Args, typename Arg>
  void log_impl(std::ostream& stream, Arg arg, Args... args)
  {
    stream << arg;

    log_impl(stream, args...);
  }

  template<typename Arg>
  void log_impl(std::ostream& stream, Arg arg)
  {
    stream << arg << std::endl;
  }

private:
  std::vector<Logger> loggers_;
};

} // namespace

struct datviz_struct final
{
  LoggerProxy log;

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

  tmp_stream << std::setw(3) << "   " << 1 << " | ";

  for (size_t i = 0; i < source_len; i++) {
    if (source[i] == '\n')
      line++;

    tmp_stream << source[i];

    if ((source[i] == '\n') && ((i + 1) < source_len))
      tmp_stream << "   " << line << " | ";
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

datviz_struct*
datviz_create(void)
{
  if (glfwInit() != GLFW_TRUE) {
    return nullptr;
  }

  glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
  glfwWindowHint(GLFW_SAMPLES, 4);
  glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

  auto* viz = new datviz_struct();

  viz->window = glfwCreateWindow(1280, 720, "", nullptr, nullptr);
  if (!viz->window) {
    delete viz;
    return nullptr;
  }

  glfwSetWindowUserPointer(viz->window, viz);

  glfwSetKeyCallback(viz->window, key_callback);

  glfwMakeContextCurrent(viz->window);

  gladLoadGLES2Loader((GLADloadproc)glfwGetProcAddress);

  glClearColor(0, 0, 0, 1);

  glEnable(GL_DEPTH);

  glGenVertexArrays(1, &viz->vertex_array);

  glGenBuffers(1, &viz->vertex_buffer);

  glBindVertexArray(viz->vertex_array);

  glBindBuffer(GL_ARRAY_BUFFER, viz->vertex_buffer);

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
    glfwDestroyWindow(viz->window);
    delete viz;
    return nullptr;
  }

  viz->program = glCreateProgram();

  glAttachShader(viz->program, vert_shader);
  glAttachShader(viz->program, frag_shader);

  glLinkProgram(viz->program);

  glDetachShader(viz->program, frag_shader);
  glDetachShader(viz->program, vert_shader);

  glDeleteShader(vert_shader);
  glDeleteShader(frag_shader);

  GLint link_status = GL_FALSE;

  glGetProgramiv(viz->program, GL_LINK_STATUS, &link_status);

  if (link_status != GL_TRUE) {
    glDeleteProgram(viz->program);
    glfwDestroyWindow(viz->window);
    delete viz;
    return nullptr;
  }

  glUseProgram(viz->program);

  viz->mvp_location = glGetUniformLocation(viz->program, "mvp");

  return viz;
}

void
datviz_destroy(datviz_z* viz)
{
  if (!viz)
    return;

  glfwMakeContextCurrent(viz->window);

  glDisable(GL_DEPTH);

  glDeleteBuffers(1, &viz->vertex_buffer);

  glDeleteVertexArrays(1, &viz->vertex_array);

  glDeleteProgram(viz->program);

  glfwDestroyWindow(viz->window);

  delete viz;

  glfwTerminate();
}

void
datviz_set_window_title(datviz_z* viz, const char* title)
{
  glfwSetWindowTitle(viz->window, title);
}

void
datviz_get_window_size(datviz_z* viz, int* w, int* h)
{
  assert(viz != nullptr);

  glfwGetWindowSize(viz->window, w, h);
}

void
datviz_get_framebuffer_size(datviz_z* viz, int* w, int* h)
{
  glfwGetFramebufferSize(viz->window, w, h);
}

void
datviz_set_background(datviz_z* viz, float r, float g, float b)
{
  if (!viz)
    return;

  glfwMakeContextCurrent(viz->window);

  glClearColor(r, g, b, 1.0f);
}

void
datviz_set_model_transform(datviz_z* viz, const float* model_transform)
{
  viz->model_transform = glm::make_mat4x4(model_transform);
}

void
datviz_set_view_transform(datviz_z* viz, const float* view_transform)
{
  viz->view_transform = glm::make_mat4x4(view_transform);
}

void
datviz_look_at(datviz_z* viz, const float* eye, const float* center, const float* up)
{
  viz->view_transform = glm::lookAt(glm::make_vec3(eye), glm::make_vec3(center), glm::make_vec3(up));
}

void
datviz_set_projection_transform(datviz_z* viz, const float* projection_transform)
{
  viz->projection_transform = glm::make_mat4x4(projection_transform);
}

void
datviz_set_perspective(datviz_z* viz, float fovy, float near, float far)
{
  int w = 0;
  int h = 0;
  datviz_get_framebuffer_size(viz, &w, &h);
  viz->projection_transform = glm::perspective(float(w) / float(h), fovy, near, far);
}

void
datviz_begin_frame(datviz_z* viz)
{
  glfwMakeContextCurrent(viz->window);

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  int w = 0;
  int h = 0;

  glfwGetFramebufferSize(viz->window, &w, &h);

  glViewport(0, 0, w, h);
}

void
datviz_render_points(datviz_z* viz, const void* attrib, uint32_t count)
{
  glBindBuffer(GL_ARRAY_BUFFER, viz->vertex_buffer);

  glBufferData(GL_ARRAY_BUFFER, count * sizeof(float) * 6, attrib, GL_DYNAMIC_DRAW);

  glBindVertexArray(viz->vertex_array);

  glUseProgram(viz->program);

  glUniformMatrix4fv(viz->mvp_location, 1, GL_FALSE, glm::value_ptr(viz->mvp()));

  glDrawArrays(GL_POINTS, 0, count);

  glBindVertexArray(0);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void
datviz_end_frame(datviz_z* viz)
{
  glfwSwapBuffers(viz->window);
}

void
datviz_poll_input(datviz_z* viz)
{
  glfwPollEvents();
}

int
datviz_should_close(datviz_z* viz)
{
  if (!viz)
    return 1;

  return glfwWindowShouldClose(viz->window);
}
