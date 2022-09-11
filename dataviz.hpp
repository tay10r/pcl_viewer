/// @file datviz.h
///
/// @brief The public API for the data visualization library.

#pragma once

#include <stdint.h>

namespace dataviz {

/** @brief A structure containing all the data associated with one vertex.
 *
 * @details This is used to describe geometry of various types.
 * */
struct dataviz_vertex
{
  /** The X position coordinate of the vertex. */
  float x;
  /** The Y position coordinate of the vertex. */
  float y;
  /** The Z position coordinate of the vertex. */
  float z;
  /** The red channel value of the vertex. */
  unsigned char r;
  /** The green channel value of the vertex. */
  unsigned char g;
  /** The blue channel value of the vertex. */
  unsigned char b;
  /** The alpha channel value of the vertex. */
  unsigned char a;
};

typedef dataviz_vertex dataviz_vertex_z;

/** @brief Initializes global resources used by the library.
 *
 * @return Zero on success, non-zero on failure.
 * */
int
datviz_global_init(void);

/** @brief Releases global resources used by the library.
 * */
void
datviz_global_cleanup(void);

/** @brief The type of the callback used for logging information.
 * */
typedef void (*datviz_logger_callback)(void* user_data, const char* message_data, uint32_t message_size);

/** @brief The interface to the library for rendering visualizations.
 * */
typedef struct datviz_struct datviz_z;

/** @brief Creates a new instance of the data visualization viewer.
 *
 * @return A new data visualization instance.
 *         Use @ref datviz_destroy to release the associated memory.
 *         A null pointer may be returned if window initialization fails.
 * */
datviz_z*
datviz_create(void);

/** @brief Releases memory allocated by a data visualization.
 *
 * @param viz A data visualization instance that was returned with @ref datviz_create.
 *               A null pointer may be passed to this function, in which case nothing will happen.
 * */
void
datviz_destroy(datviz_z* viz);

/** @brief Adds a logger callback to the data visualization.
 *
 * @details This feature can be used to get more diagnostic information about errors.
 *
 * @param viz The viewer to add the logger to.
 *
 * @param logger_data A pointer to extra client data to be passed to the logger callback.
 *                    This pointer may be null, if it is not used by the logger callback.
 *
 * @param callback The callback to pass the logging information to.
 * */
void
datviz_add_logger(datviz_z* viz, void* logger_data, datviz_logger_callback callback);

/** @brief Sets the title of the window.
 *
 * @param viz The viewer to set the window title of.
 *
 * @param title The title to assign the window.
 * */
void
datviz_set_window_title(datviz_z* viz, const char* title);

/** @brief Sets whether or not user camera controls are enabled.
 *
 * @note Camera controls are enabled by default.
 *
 * @param viz The visualization to enable or disable camera controls with.
 *
 * @param enabled A boolean integer that indicates whether or not interactive camera controls are enabled.
 * */
void
datviz_set_camera_controls_enabled(datviz_z* viz, int enabled);

/** @brief Allows rendering functions to be called for the next frame to be displayed.
 *
 * @details Internally, this function does a few things.
 *          The first thing it does is make the window GL context current.
 *          The second it does is clear the color buffer and depth buffer.
 *          Finally, the function will set the viewport transform to the width and height of the framebuffer.
 *
 * @param viz The viewer to begin a new frame on.
 * */
void
datviz_begin_frame(datviz_z* viz);

/** @brief Renders points onto the current framebuffer.
 *
 * @param viz The viewer to render points onto.
 *
 * @param xyz_rgb The buffer containing the positions and color of each particle.
 *                Unless otherwise specified, the library expects three floats that
 *                describe each point position followed by three floats that describe
 *                each point color.
 *
 * @param point_count The number of points to be rendered.
 * */
void
datviz_render_points(datviz_z* viz, const dataviz_vertex* xyz_rgb, uint32_t point_count);

/** @brief Performs the buffer swap that causes the rendered contents to be displayed on the window.
 *
 * @param viz The viewer to complete the frame with.
 * */
void
datviz_end_frame(datviz_z* viz);

// TODO: What do these report when the window is created hidden?

/** @brief Gets the size of the window, in terms of pixels.
 *
 * @param viz The viewer to get the window size of.
 *
 * @param w The variable to assign the window width to, in pixels.
 *
 * @param h The variable to assign the window height to, in pixels.
 * */
void
datviz_get_window_size(datviz_z* viz, int* w, int* h);

/** @brief Gets the size of the framebuffer, in terms of pixels.
 *
 * @param viz The viewer to get the framebuffer size of.
 *
 * @param w The variable to assign the framebuffer width to, in pixels.
 *
 * @param h The variable to assign the framebuffer height to, in pixels.
 * */
void
datviz_get_framebuffer_size(datviz_z* viz, int* w, int* h);

/** @brief Sets the background color of the frame.
 *
 * @details This affects the color that fills the frame when @ref datviz_begin_frame gets called.
 *
 * @param viz The viewer to set the background of.
 *
 * @param r The red channel value of the background color.
 *
 * @param g The green channel value of the background color.
 *
 * @param b The blue channel value of the background color.
 *
 * @param a The alpha channel value of the background color.
 * */
void
datviz_set_background(datviz_z* viz, float r, float g, float b, float a);

/** @brief Sets the view transform of the render functions.
 *
 * @param viz The viewer to set the view transform of.
 *
 * @param view_transform The 4x4 view space transformation, in column-major order.
 * */
void
datviz_set_view_transform(datviz_z* viz, const float* view_transform);

/** @brief Sets the view transform of the render functions by using three vectors.
 *
 * @param viz The viewer to set the view transform of.
 *
 * @param eye A 3D position vector indicating the position of the camera.
 *
 * @param center A 3D position vector indicating the position that the camera is looking at.
 *
 * @param up A 3D unit vector to indicate which direction is up.
 * */
void
datviz_look_at(datviz_z* viz, const float* eye, const float* center, const float* up);

/** @brief Sets the projection space transformation.
 *
 * @param viz The viewer to set the projection space transformation of.
 *
 * @param projection_transform A 4x4 matrix, in column-major order, to indicate the projection space transformation.
 * */
void
datviz_set_projection_transform(datviz_z* viz, const float* projection_transform);

/** @brief Sets the projection space transformation using a perspective transform.
 *
 * @note The aspect ratio is calculated from the width and height of the window.
 *
 * @note Calling this function in the render loop is useful to handle the user changing the window aspect ratio.
 *
 * @param viz The viewer to set the projection space transformation of.
 *
 * @param fovy The vertical field of view angle, in radians.
 *
 * @param near The distance to the near plane.
 *
 * @param far The distance to the far plane.
 * */
void
datviz_set_perspective(datviz_z* viz, float fovy, float near, float far);

/** @brief Sets the model transformation to apply to the rendered primitives.
 *
 * @param viz The viewer to set the model space transformation of.
 *
 * @param model_transform A 4x4 matrix, in column-major order, to indicate the model space transformation of.
 * */
void
datviz_set_model_transform(datviz_z* viz, const float* model_transform);

/** @brief Checks for window input from either a mouse or keyboard.
 *
 * @details Calling this function is required for handling user interactions.
 *
 * @param viz The viewer to check input for.
 * */
void
datviz_poll_input(datviz_z* viz);

/** @brief Indicates whether or not the viewer should be closed.
 *
 * @details This function is affected by the user clicking the window exit button or the escape key.
 *
 * @param viz The viewer to check for a close signal.
 *
 * @return Non-zero if the viewer should close, zero otherwise.
 * */
int
datviz_should_close(datviz_z* viz);

} // namespace dataviz
