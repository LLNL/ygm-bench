// Copyright 2019-2021 Lawrence Livermore National Security, LLC and other YGM
// Project Developers. See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: MIT

#include <random>
#include <utility.hpp>
#include <ygm/comm.hpp>
#include <ygm/container/array.hpp>
#include <ygm/detail/ygm_cereal_archive.hpp>
#include <ygm/utility.hpp>

static int updater_lifetime;

class updater {
 public:
  // Needs default constructor to send through YGM
  updater() {}

  updater(uint64_t seed) : m_state(seed), m_counter(0) {}

  void increment_counter() const { ++m_counter; }

  bool is_alive() const { return get_counter() < updater_lifetime; }

  uint32_t get_counter() const { return m_counter; }

  uint64_t get_state() const { return m_state; }

  template <typename Archive>
  void serialize(Archive &ar) {
    ar(m_state, m_counter);
  }

  void update_state(uint64_t value) const { m_state ^= value; }

 private:
  mutable uint64_t m_state;
  mutable uint32_t m_counter;
};

struct recursive_functor {
 public:
  // Creates a copy of u because YGM passes arguments by const reference
  void operator()(
      ygm::ygm_ptr<ygm::container::array<uint64_t>::impl_type> parray,
      const uint64_t index, uint64_t &value, updater u) {
    u.update_state(value);
    value = u.get_state();
    u.increment_counter();

    if (u.is_alive()) {
      uint64_t new_index = value & (parray->size() - 1);
      parray->async_visit(new_index, recursive_functor(), u);
    }
  }
};

int main(int argc, char **argv) {
  int provided;
  MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
  if (provided != MPI_THREAD_MULTIPLE) {
    throw std::runtime_error(
        "MPI_Init_thread: MPI_THREAD_MULTIPLE not provided.");
  }

  {
    int       ygm_buffer_capacity = atoi(argv[1]);
    int       ygm_num_listeners   = atoi(argv[2]);
    ygm::comm world(MPI_COMM_WORLD, ygm_buffer_capacity, ygm_num_listeners, 30);

    int     log_global_table_size{atoi(argv[3])};
    int64_t local_updaters{atoll(argv[4])};
    updater_lifetime = atoi(argv[5]);
    int         num_trials{atoi(argv[6])};
    std::string stats_output_prefix(argv[7]);

    uint64_t global_table_size = ((uint64_t)1) << log_global_table_size;
    ygm::container::array<uint64_t> arr(world, global_table_size);

    std::mt19937                            gen(world.rank());
    std::uniform_int_distribution<uint64_t> dist;

    if (world.rank0()) {
      std::cout << world.layout().node_size() << ", "
                << world.layout().local_size() << ", " << ygm_buffer_capacity
                << ", " << ygm_num_listeners << ", " << world.routing_protocol()
                << ", " << global_table_size << ", "
                << local_updaters * world.size() << ", " << updater_lifetime;
    }

    // world.cout0("Initializing array values");
    arr.for_all(
        [&dist, &gen](const auto index, auto &value) { value = dist(gen); });

    double total_update_time{0.0};

    world.cf_barrier();
    // world.cout0("Beginning AGUPS trials");

    world.stats_reset();

    for (int trial = 0; trial < num_trials; ++trial) {
      std::vector<updater> updater_vec;

      for (int64_t i = 0; i < local_updaters; ++i) {
        updater_vec.emplace_back(updater(dist(gen)));
      }

      world.barrier();

      world.set_track_stats(true);

      ygm::timer update_timer{};

      for (auto &u : updater_vec) {
        uint64_t first_index = u.get_state() & (arr.size() - 1);
        arr.async_visit(first_index, recursive_functor(), u);
      }

      world.barrier();

      world.set_track_stats(false);

      double trial_time = update_timer.elapsed();

      if (world.rank0()) {
        double trial_gups = local_updaters * world.size() * updater_lifetime /
                            trial_time / (1000 * 1000 * 1000);
        // std::cout << "Trial " << trial + 1 << " GUPS: " << trial_gups
        //<< std::endl;

        std::cout << ", " << trial_time << ", " << trial_gups;
      }

      total_update_time += trial_time;
    }

    if (world.rank0()) {
      double gups = local_updaters * world.size() * num_trials *
                    updater_lifetime / total_update_time / (1000 * 1000 * 1000);
      // std::cout << "Average GUPS: " << gups << std::endl;

      std::cout << std::endl;
    }

    auto experiment_stats = world.stats_snapshot();
    // write_stats_files(world, experiment_stats, stats_output_prefix);
  }

  MPI_Finalize();
}
