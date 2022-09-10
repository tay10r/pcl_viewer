#include <point_cloud_viewer.h>

#include <glm/glm.hpp>

#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <random>
#include <vector>

#include <cstdlib>

#ifdef _WIN32
#include <Windows.h>
#endif

namespace {

class MajorSystem final
{
public:
  template<typename Rng>
  MajorSystem(size_t size, Rng& rng)
    : particles_(size * 6)
    , velocity_(size, glm::vec3(0, 0, 0))
  {
    std::uniform_real_distribution<float> pos_dist(-1.0, 1.0);
    std::uniform_real_distribution<float> col_dist(0.5, 1.0);

    for (size_t i = 0; i < size; i++) {
      particles_[(i * 6) + 0] = pos_dist(rng);
      particles_[(i * 6) + 1] = pos_dist(rng);
      particles_[(i * 6) + 2] = pos_dist(rng);

      particles_[(i * 6) + 3] = col_dist(rng);
      particles_[(i * 6) + 4] = col_dist(rng);
      particles_[(i * 6) + 5] = 0;
    }
  }

  const float* data() const { return particles_.data(); }

  size_t size() const { return particles_.size() / 6; }

  void step(float time_delta, const float gravity = 1.0e-9, const float smooth = 1.0e-3)
  {
    const std::size_t particle_count = size();

    std::vector<float> next_particles(particle_count * 6);

    std::vector<glm::vec3> next_velocity(particle_count);

    for (size_t i = 0; i < particle_count; i++) {

      const auto* a = particles_.data() + (i * 6);

      const auto a_pos = glm::vec3(a[0], a[1], a[2]);

      glm::vec3 force(0, 0, 0);

      for (size_t j = 0; j < particle_count; j++) {

        if (i == j)
          continue;

        const auto* b = particles_.data() + (j * 6);

        const auto b_pos = glm::vec3(b[0], b[1], b[2]);

        const auto delta = b_pos - a_pos;

        const auto distance_squared = glm::dot(delta, delta);

        if (distance_squared < smooth)
          continue;

        const auto dir = glm::normalize(b_pos - a_pos);

        force = force + (dir * (1 / (distance_squared + (smooth * smooth))));
      }

      const auto accel = force * gravity;

      const auto delta_p_1 = (0.5f * time_delta * time_delta) * accel;
      const auto delta_p_2 = time_delta * velocity_[i];
      const auto delta_p = delta_p_1 + delta_p_2;

      next_velocity[i] = velocity_[i] + (accel * time_delta);

      next_particles[(i * 6) + 0] = particles_[(i * 6) + 0] + delta_p.x;
      next_particles[(i * 6) + 1] = particles_[(i * 6) + 1] + delta_p.y;
      next_particles[(i * 6) + 2] = particles_[(i * 6) + 2] + delta_p.z;

      next_particles[(i * 6) + 3] = particles_[(i * 6) + 3];
      next_particles[(i * 6) + 4] = particles_[(i * 6) + 4];
      next_particles[(i * 6) + 5] = particles_[(i * 6) + 5];
    }

    particles_ = std::move(next_particles);

    velocity_ = std::move(next_velocity);
  }

private:
  std::vector<float> particles_;

  std::vector<glm::vec3> velocity_;
};

} // namespace

#ifdef _WIN32
int
WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
#else
int
main()
#endif
{
  std::mt19937 rng(1234);

  const std::uint32_t point_count = 200;

  MajorSystem major_system(point_count, rng);

  pcl_viewer_z* viewer = pcl_viewer_create("Example Point Cloud");
  if (!viewer) {
    std::cerr << "Failed to create point cloud viewer window." << std::endl;
    return EXIT_FAILURE;
  }

  pcl_viewer_set_perspective(viewer, glm::radians(45.0f), 0.01f, 10.0f);

  while (!pcl_viewer_should_close(viewer)) {

    pcl_viewer_begin_frame(viewer);

    pcl_viewer_render_points(viewer, major_system.data(), point_count);

    pcl_viewer_end_frame(viewer);

    pcl_viewer_poll_input(viewer);

    for (int i = 0; i < 10; i++)
      major_system.step(1e-1);
  }

  pcl_viewer_destroy(viewer);

  return 0;
}
