// Copyright 2019-2021 Lawrence Livermore National Security, LLC and other YGM
// Project Developers. See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: MIT

#include <rmat_edge_generator.hpp>
#include <utility.hpp>
#include <ygm/comm.hpp>
#include <ygm/container/array.hpp>
#include <ygm/detail/ygm_cereal_archive.hpp>
#include <ygm/utility.hpp>

int main(int argc, char **argv) {
  int provided;
  MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
  if (provided != MPI_THREAD_MULTIPLE) {
    throw std::runtime_error(
        "MPI_Init_thread: MPI_THREAD_MULTIPLE not provided.");
  }

  {
    int ygm_buffer_capacity = atoi(argv[1]);

    ygm::comm world(MPI_COMM_WORLD, ygm_buffer_capacity);

    int         log_global_table_size{atoi(argv[2])};
    int64_t     local_updates{atoll(argv[3])};
    int         num_trials{atoi(argv[4])};
    std::string stats_output_prefix(argv[5]);

    uint64_t global_table_size = ((uint64_t)1) << log_global_table_size;
    // world.cout0("Creating global table with size ", global_table_size);
    if (world.rank0()) {
      std::cout << world.layout().node_size() << ", "
                << world.layout().local_size() << ", " << ygm_buffer_capacity
                << ", " << world.routing_protocol() << ", " << global_table_size
                << ", " << local_updates * world.size();
    }

    ygm::container::array<uint64_t> arr(world, global_table_size);

    std::mt19937                            gen(world.rank());
    std::uniform_int_distribution<uint64_t> dist(0, global_table_size - 1);

    double total_update_time{0.0};

    world.stats_reset();

    for (int trial = 0; trial < num_trials; ++trial) {
      std::vector<uint64_t>           indices;
      distributed_rmat_edge_generator rmat(world, log_global_table_size,
                                           local_updates * world.size(), trial);

      rmat.for_all([&indices](const auto first, const auto second) {
        indices.push_back(first);
      });

      world.barrier();

      world.set_track_stats(true);

      ygm::timer update_timer{};

      for (const auto &index : indices) {
        arr.async_visit(index, [](const auto i, auto v) { ++v; });
      }

      world.barrier();

      world.set_track_stats(false);

      double trial_time = update_timer.elapsed();

      if (world.rank0()) {
        double trial_rate =
            local_updates * world.size() / trial_time / (1000 * 1000 * 1000);
        // std::cout << "Trial " << trial + 1 << std::endl;
        // std::cout << "Time: " << trial_time << " sec\n"
        //<< "Updates per second (billions): " << trial_rate << "\n"
        //<< std::endl;

        std::cout << ", " << trial_time << ", " << trial_rate;
      }

      total_update_time += trial_time;
    }

    uint64_t global_bytes = world.global_bytes_sent();
    uint64_t global_message_bytes =
        local_updates * world.size() * num_trials * 8;

    if (world.rank0()) {
      double average_rate = local_updates * world.size() * num_trials /
                            total_update_time / (1000 * 1000 * 1000);
      // std::cout << "Average updates per second (billions): " << average_rate
      //<< std::endl;
      // std::cout << "Average bandwidth per rank (GB/s): "
      //<< global_bytes / total_update_time / world.size() /
      //(1024 * 1024 * 1024)
      //<< std::endl;
      // std::cout << "Average useful bandwidth per rank (GB/s): "
      //<< global_message_bytes / total_update_time / world.size() /
      //(1024 * 1024 * 1024)
      //<< std::endl;

      std::cout << std::endl;
    }

    auto experiment_stats = world.stats_snapshot();
    write_stats_files(world, experiment_stats, stats_output_prefix);
  }

  MPI_Finalize();
}
