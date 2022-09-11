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

//===========//
// Constants //
//===========//

namespace {

constexpr uint32_t g_vertex_size = 16;

} // namespace

//===================//
// Camera Controller //
//===================//

namespace {

class CameraController final
{
public:
  void set_enabled(bool enabled) { enabled_ = enabled; }

  bool enabled() const { return enabled_; }

private:
  bool enabled_ = true;
};

} // namespace

//=========//
// Logging //
//=========//

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

  template<typename... Args>
  void info(Args... args)
  {
    log("INFO: ", args...);
  }

  template<typename... Args>
  void error(Args... args)
  {
    log("ERROR: ", args...);
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

//========//
// Window //
//========//

namespace {

class Window final
{
public:
  Window() = default;

  Window(const Window&) = delete;

  ~Window()
  {
    if (window_)
      glfwDestroyWindow(window_);
  }

  bool is_created() const { return window_ != nullptr; }

  bool make_context_current()
  {
    auto* win = get_or_initialize_window();
    if (!win)
      return false;
    glfwMakeContextCurrent(win);
    return true;
  }

  void swap_buffers()
  {
    assert(window_ != nullptr);
    glfwSwapBuffers(window_);
  }

  bool set_title(const char* title)
  {
    auto* win = get_or_initialize_window();
    if (!win)
      return false;
    glfwSetWindowTitle(win, title);
    return true;
  }

  bool get_window_size(int* w, int* h)
  {
    auto* win = get_or_initialize_window();
    if (!win)
      return false;
    glfwGetWindowSize(win, w, h);
    return true;
  }

  bool get_framebuffer_size(int* w, int* h)
  {
    auto* win = get_or_initialize_window();
    if (!win)
      return false;
    glfwGetFramebufferSize(win, w, h);
    return true;
  }

  bool should_close() { return glfwWindowShouldClose(window_) != 0; }

private:
  GLFWwindow* get_or_initialize_window()
  {
    if (window_ != nullptr)
      return window_;

    glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    window_ = glfwCreateWindow(640, 480, "", nullptr, nullptr);
    if (!window_)
      return nullptr;

    glfwSetWindowUserPointer(window_, this);

    glfwSetKeyCallback(window_, key_callback);

    glfwMakeContextCurrent(window_);

    gladLoadGLES2Loader((GLADloadproc)glfwGetProcAddress);

    glEnable(GL_DEPTH);

    return window_;
  }

  void handle_key(int /* key */, int /* action */) {}

  static Window* get_self(GLFWwindow* window) { return static_cast<Window*>(glfwGetWindowUserPointer(window)); }

  static void key_callback(GLFWwindow* window, int key, int /* scancode */, int action, int /* mods */)
  {
    get_self(window)->handle_key(key, action);
  }

private:
  GLFWwindow* window_ = nullptr;
};

} // namespace

//==============//
// Vertex Array //
//==============//

namespace {

struct Vertex final
{
  glm::vec3 xyz{ 0, 0, 0 };

  unsigned char rgba[4]{ 192, 192, 192, 255 };
};

class VertexArray final
{
public:
  VertexArray() = default;

  VertexArray(const VertexArray&) = default;

  ~VertexArray()
  {
    // These should have been cleaned up before this.
    assert(buffer_ == 0);
    assert(array_ == 0);
  }

  bool init()
  {
    glGenBuffers(1, &buffer_);

    glGenVertexArrays(1, &array_);

    glBindBuffer(GL_ARRAY_BUFFER, buffer_);

    glBindVertexArray(array_);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    const auto stride = 16; // 12 bytes per position, 4 bytes per color
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, 0);
    glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, stride, reinterpret_cast<const void*>(12));

    glBindVertexArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    return glGetError() == GL_NO_ERROR;
  }

  void cleanup()
  {
    if (buffer_ != 0)
      glDeleteBuffers(1, &buffer_);

    if (array_ != 0)
      glDeleteVertexArrays(1, &array_);

    buffer_ = 0;

    array_ = 0;
  }

  void bind()
  {
    assert(!is_bound_);
    glBindBuffer(GL_ARRAY_BUFFER, buffer_);
    glBindVertexArray(array_);
    is_bound_ = true;
  }

  void unbind()
  {
    assert(is_bound_);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    is_bound_ = false;
  }

  bool buffer_data(const void* data, uint32_t vertex_count, GLenum usage = GL_DYNAMIC_DRAW)
  {
    assert(is_bound_);

    glBufferData(GL_ARRAY_BUFFER, vertex_count * g_vertex_size, data, usage);

    return glGetError() == GL_NO_ERROR;
  }

private:
  GLuint buffer_ = 0;

  GLuint array_ = 0;

  bool is_bound_ = false;
};

} // namespace

//========//
// Shader //
//========//

namespace {

template<GLenum Kind>
class Shader final
{
public:
  Shader() = default;

  Shader(const Shader&) = delete;

  ~Shader() { assert(id_ == 0); }

  bool init(const char* source)
  {
    id_ = glCreateShader(Kind);

    const GLint length = static_cast<GLint>(strlen(source));

    glShaderSource(id_, 1, &source, &length);

    glCompileShader(id_);

    return check_shader(source);
  }

  void cleanup()
  {
    if (id_ == 0)
      return;

    glDeleteShader(id_);

    id_ = 0;
  }

  GLuint id() { return id_; }

private:
  bool check_shader(const char* source)
  {
    GLint is_compiled = GL_FALSE;

    glGetShaderiv(id_, GL_COMPILE_STATUS, &is_compiled);

    if (is_compiled == GL_TRUE)
      return true;

    GLint max_length = 0;

    glGetShaderiv(id_, GL_INFO_LOG_LENGTH, &max_length);

    std::string info_log;

    info_log.resize(max_length);

    glGetShaderInfoLog(id_, max_length, &max_length, &info_log[0]);

    print_shader_error(source, info_log, std::cerr);

    return false;
  }

  void print_shader_error(const char* source, const std::string& info_log, std::ostream& stream)
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

private:
  GLuint id_ = 0;
};

} // namespace

//================//
// Shader Program //
//================//

namespace {

class ShaderProgram final
{
public:
  ShaderProgram() = default;

  ShaderProgram(const ShaderProgram&) = delete;

  ~ShaderProgram() { assert(id_ == 0); }

  bool init(const char* vert_source, const char* frag_source)
  {
    Shader<GL_VERTEX_SHADER> vert_shader;

    if (!vert_shader.init(vert_source)) {
      vert_shader.cleanup();
      return false;
    }

    Shader<GL_FRAGMENT_SHADER> frag_shader;

    if (!frag_shader.init(frag_source)) {
      vert_shader.cleanup();
      frag_shader.cleanup();
      return false;
    }

    id_ = glCreateProgram();

    glAttachShader(id_, vert_shader.id());
    glAttachShader(id_, frag_shader.id());

    glLinkProgram(id_);

    glDetachShader(id_, vert_shader.id());
    glDetachShader(id_, frag_shader.id());

    return glGetError() == GL_NO_ERROR;
  }

  void cleanup()
  {
    if (id_ == 0)
      return;

    glDeleteProgram(id_);

    id_ = 0;
  }

  void bind()
  {
    assert(!is_bound_);
    glUseProgram(id_);
    is_bound_ = true;
  }

  void unbind()
  {
    assert(is_bound_);
    glUseProgram(0);
    is_bound_ = false;
  }

  GLint get_uniform_location(const char* name)
  {
    assert(is_bound_); // TODO : <-- is this needed ?

    return glGetUniformLocation(id_, name);
  }

private:
  GLuint id_ = 0;

  bool is_bound_ = false;
};

} // namespace

//======================//
// Point Shader Program //
//======================//

namespace {

namespace point_shader {

const char* vert_source = R"(
#version 300 es

uniform highp mat4 mvp;

layout(location = 0) in highp vec3 g_position;

layout(location = 1) in lowp vec4 g_color;

out lowp vec4 g_point_color;

void main()
{
  g_point_color = g_color;

  gl_Position = mvp * vec4(g_position, 1.0);
}
)";

const char* frag_source = R"(
#version 300 es

in lowp vec4 g_point_color;

out lowp vec4 g_out_color;

void main()
{
  g_out_color = g_point_color;
}
)";

} // namespace point_shader

class PointShaderProgram final
{
public:
  bool init()
  {
    if (!shader_program_.init(point_shader::vert_source, point_shader::frag_source))
      return false;

    mvp_location_ = shader_program_.get_uniform_location("mvp");

    return true;
  }

  void cleanup()
  {
    shader_program_.cleanup();

    vertex_array_.cleanup();
  }

  bool render_points(const dataviz_vertex_z* vertices, uint32_t point_count, const glm::mat4& mvp)
  {
    vertex_array_.bind();

    shader_program_.bind();

    vertex_array_.buffer_data(vertices, point_count);

    glUniformMatrix4fv(mvp_location_, 1, GL_FALSE, glm::value_ptr(mvp));

    glDrawArrays(GL_POINTS, 0, point_count);

    shader_program_.unbind();

    vertex_array_.unbind();

    return glGetError() == GL_NO_ERROR;
  }

private:
  ShaderProgram shader_program_;

  VertexArray vertex_array_;

  GLint mvp_location_ = -1;
};

} // namespace

//=========//
// Library //
//=========//

namespace {

class Library final
{
public:
  void add_logger(void* logger_data, datviz_logger_callback callback)
  {
    log_.add_logger(Logger{ logger_data, callback });
  }

  void get_window_size(int* w, int* h) { window_.get_window_size(w, h); }

  void get_framebuffer_size(int* w, int* h) { window_.get_framebuffer_size(w, h); }

  void cleanup() { cleanup_opengl_objects(); }

  void make_context_current() { window_.make_context_current(); }

  void set_camera_controller_enabled(bool enabled) { camera_controller_.set_enabled(enabled); }

  void set_window_title(const char* title) { window_.set_title(title); }

  void set_background_color(const glm::vec4& bg) { background_color_ = bg; }

  void set_model_transform(const glm::mat4& transform) { model_transform_ = transform; }

  void set_view_transform(const glm::mat4& transform) { view_transform_ = transform; }

  void set_projection_transform(const glm::mat4& transform) { projection_transform_ = transform; }

  bool begin_frame()
  {
    if (!window_.make_context_current())
      return false;

    glClearColor(background_color_[0], background_color_[1], background_color_[2], background_color_[3]);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    int w = 0;
    int h = 0;
    window_.get_framebuffer_size(&w, &h);

    glViewport(0, 0, w, h);

    return glGetError() == GL_NO_ERROR;
  }

  void end_frame() { window_.swap_buffers(); }

  void render_points(const dataviz_vertex_z* vertices, uint32_t vertex_count)
  {
    point_shader_program_.render_points(vertices, vertex_count, mvp());
  }

  bool should_close() { return window_.should_close(); }

private:
  glm::mat4 mvp() const { return projection_transform_ * view_transform_ * model_transform_; }

  void cleanup_opengl_objects()
  {
    if (!window_.is_created())
      return;

    if (!window_.make_context_current())
      return;

    point_shader_program_.cleanup();
  }

private:
  LoggerProxy log_;

  CameraController camera_controller_;

  Window window_;

  PointShaderProgram point_shader_program_;

  glm::vec4 background_color_{ 0, 0, 0, 1 };

  glm::mat4 model_transform_{ glm::mat4(1.0f) };

  glm::mat4 view_transform_{ glm::mat4(1.0f) };

  glm::mat4 projection_transform_{ glm::mat4(1.0f) };
};

} // namespace

//============//
// Public API //
//============//

struct datviz_struct final
{
  Library library;
#if 0
  LoggerProxy log;

  GLuint vertex_array = 0;

  GLuint vertex_buffer = 0;

  GLuint program = 0;

  GLint mvp_location = -1;

  glm::mat4 projection_transform = glm::perspective(glm::radians(45.0f), 1.0f, 0.01f, 100.0f);

  glm::mat4 view_transform = glm::lookAt(glm::vec3(2, 2, 3), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));

  glm::mat4 model_transform = glm::mat4(1.0f);

  glm::mat4 mvp() const { return projection_transform * view_transform * model_transform; }
#endif
};

namespace {

#if 0
void
key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
  if ((key == GLFW_KEY_ESCAPE) && (action == GLFW_PRESS))
    glfwSetWindowShouldClose(window, GLFW_TRUE);
}
#endif

} // namespace

datviz_struct*
datviz_create(void)
{
  return new datviz_struct();
}
#if 0
  glClearColor(0, 0, 0, 1);

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
#endif

void
datviz_destroy(datviz_z* viz)
{
  if (!viz)
    return;

  viz->library.cleanup();

  delete viz;
#if 0
  glfwMakeContextCurrent(viz->window);

  glDisable(GL_DEPTH);

  glDeleteBuffers(1, &viz->vertex_buffer);

  glDeleteVertexArrays(1, &viz->vertex_array);

  glDeleteProgram(viz->program);

  glfwDestroyWindow(viz->window);
#endif
}

void
datviz_add_logger(datviz_z* viz, void* logger_data, datviz_logger_callback callback)
{
  assert(viz != nullptr);
  viz->library.add_logger(logger_data, callback);
}

void
datviz_set_camera_controls_enabled(datviz_z* viz, int enabled)
{
  assert(viz != nullptr);

  viz->library.set_camera_controller_enabled(!!enabled);
}

void
datviz_set_window_title(datviz_z* viz, const char* title)
{
  assert(viz != nullptr);

  viz->library.set_window_title(title);
}

void
datviz_get_window_size(datviz_z* viz, int* w, int* h)
{
  assert(viz != nullptr);

  viz->library.get_window_size(w, h);
}

void
datviz_get_framebuffer_size(datviz_z* viz, int* w, int* h)
{
  assert(viz != nullptr);

  viz->library.get_framebuffer_size(w, h);
}

void
datviz_set_background(datviz_z* viz, float r, float g, float b, float a)
{
  assert(viz != nullptr);

  viz->library.set_background_color(glm::vec4(r, g, b, a));
}

void
datviz_set_model_transform(datviz_z* viz, const float* model_transform)
{
  assert(viz != nullptr);

  viz->library.set_model_transform(glm::make_mat4x4(model_transform));
}

void
datviz_set_view_transform(datviz_z* viz, const float* view_transform)
{
  assert(viz != nullptr);

  viz->library.set_view_transform(glm::make_mat4x4(view_transform));
}

void
datviz_look_at(datviz_z* viz, const float* eye, const float* center, const float* up)
{
  const auto view = glm::lookAt(glm::make_vec3(eye), glm::make_vec3(center), glm::make_vec3(up));

  datviz_set_view_transform(viz, glm::value_ptr(view));
}

void
datviz_set_projection_transform(datviz_z* viz, const float* projection_transform)
{
  assert(viz != nullptr);

  viz->library.set_projection_transform(glm::make_mat4x4(projection_transform));
}

void
datviz_set_perspective(datviz_z* viz, float fovy, float near, float far)
{
  int w = 0;
  int h = 0;
  datviz_get_framebuffer_size(viz, &w, &h);

  const auto projection = glm::perspective(float(w) / float(h), fovy, near, far);

  datviz_set_projection_transform(viz, glm::value_ptr(projection));
}

void
datviz_begin_frame(datviz_z* viz)
{
  assert(viz != nullptr);

  viz->library.begin_frame();
}

void
datviz_render_points(datviz_z* viz, const dataviz_vertex_z* vertices, uint32_t count)
{
  assert(viz != nullptr);

  viz->library.render_points(vertices, count);
}

void
datviz_end_frame(datviz_z* viz)
{
  assert(viz != nullptr);

  viz->library.end_frame();
}

void
datviz_poll_input(datviz_z*)
{
  glfwPollEvents();
}

int
datviz_should_close(datviz_z* viz)
{
  assert(viz != nullptr);

  return viz->library.should_close();
}

//========//
// Global //
//========//

int
datviz_global_init()
{
  return glfwInit() != 0 ? -1 : 0;
}

void
datviz_global_cleanup()
{
  glfwTerminate();
}
