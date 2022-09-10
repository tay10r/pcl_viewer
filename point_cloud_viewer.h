/** @file point_cloud_viewer.h
 *
 * @brief The public API for the point cloud viewer library.
 * */

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

  /** @brief The interface to the library for rendering point clouds.
   * */
  typedef struct pcl_viewer_struct pcl_viewer_z;

  pcl_viewer_z* pcl_viewer_create(const char* window_title);

  void pcl_viewer_destroy(pcl_viewer_z* viewer);

  void pcl_viewer_begin_frame(pcl_viewer_z* viewer);

  void pcl_viewer_render_points(pcl_viewer_z* viewer, const float* xyz_rgb, uint32_t point_count);

  void pcl_viewer_end_frame(pcl_viewer_z* viewer);

  void pcl_viewer_get_window_size(pcl_viewer_z* viewer, int* w, int* h);

  void pcl_viewer_get_framebuffer_size(pcl_viewer_z* viewer, int* w, int* h);

  void pcl_viewer_set_background(pcl_viewer_z* viewer, float r, float g, float b);

  void pcl_viewer_set_view_transform(pcl_viewer_z* viewer, const float* view_transform);

  void pcl_viewer_look_at(pcl_viewer_z* viewer, const float* eye, const float* center, const float* up);

  void pcl_viewer_set_projection_transform(pcl_viewer_z* viewer, const float* projection_transform);

  void pcl_viewer_set_perspective(pcl_viewer_z* viewer, float fovy, float near, float far);

  void pcl_viewer_set_model_transform(pcl_viewer_z* viewer, const float* model_transform);

  void pcl_viewer_poll_input(pcl_viewer_z* viewer);

  int pcl_viewer_should_close(pcl_viewer_z* viewer);

#ifdef __cplusplus
} /* extern "C" */
#endif
