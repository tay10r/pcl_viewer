#include <datviz.h>

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
    : particles_(size)
    , velocity_(size, glm::vec3(0, 0, 0))
  {
    std::uniform_real_distribution<float> pos_dist(-1.0, 1.0);

    std::uniform_int_distribution<unsigned char> col_dist(127, 255);

    for (size_t i = 0; i < size; i++) {
      particles_[i].x = pos_dist(rng);
      particles_[i].y = pos_dist(rng);
      particles_[i].z = pos_dist(rng);

      particles_[i].r = col_dist(rng);
      particles_[i].g = col_dist(rng);
      particles_[i].b = 0;
      particles_[i].a = 255;
    }
  }

  const dataviz_vertex_z* data() const { return particles_.data(); }

  size_t size() const { return particles_.size() / 6; }

  void step(float time_delta, const float gravity = 1.0e-9, const float smooth = 1.0e-3)
  {
    const std::size_t particle_count = size();

    std::vector<dataviz_vertex_z> next_particles(particle_count);

    std::vector<glm::vec3> next_velocity(particle_count);

    for (size_t i = 0; i < particle_count; i++) {

      const auto* a = particles_.data() + i;

      const auto a_pos = glm::vec3(a->x, a->y, a->z);

      glm::vec3 force(0, 0, 0);

      for (size_t j = 0; j < particle_count; j++) {

        if (i == j)
          continue;

        const auto* b = particles_.data() + j;

        const auto b_pos = glm::vec3(b->x, b->y, b->z);

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

      next_particles[i].x = particles_[i].x + delta_p.x;
      next_particles[i].y = particles_[i].y + delta_p.y;
      next_particles[i].z = particles_[i].z + delta_p.z;

      next_particles[i].r = particles_[i].r;
      next_particles[i].g = particles_[i].g;
      next_particles[i].b = particles_[i].b;
      next_particles[i].a = particles_[i].a;
    }

    particles_ = std::move(next_particles);

    velocity_ = std::move(next_velocity);
  }

private:
  std::vector<dataviz_vertex_z> particles_;

  std::vector<glm::vec3> velocity_;
};

void
log_callback(void* /* user_data */, const char* msg, unsigned int /* msg_size */)
{
  std::cerr << msg << std::endl;
}

} // namespace

#ifdef _WIN32
int
WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
#else
int
main()
#endif
{
  datviz_global_init();

  std::mt19937 rng(1234);

  const std::uint32_t point_count = 2000;

  MajorSystem major_system(point_count, rng);

  datviz_z* viewer = datviz_create();

  datviz_add_logger(viewer, nullptr, log_callback);

  datviz_set_window_title(viewer, "Example Point Cloud");

  if (!viewer) {
    std::cerr << "Failed to create point cloud viewer window." << std::endl;
    return EXIT_FAILURE;
  }

  datviz_set_perspective(viewer, glm::radians(45.0f), 0.01f, 10.0f);

  while (!datviz_should_close(viewer)) {

    datviz_begin_frame(viewer);

    datviz_render_points(viewer, major_system.data(), point_count);

    datviz_end_frame(viewer);

    datviz_poll_input(viewer);

    major_system.step(1);
  }

  datviz_destroy(viewer);

  datviz_global_cleanup();

  return 0;
}
