// Copyright 2019-2021 Lawrence Livermore National Security, LLC and other YGM
// Project Developers. See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: MIT

#include <rmat_edge_generator.hpp>
#include <ygm/comm.hpp>
#include <ygm/container/array.hpp>
#include <ygm/detail/ygm_cereal_archive.hpp>
#include <ygm/utility.hpp>

int main(int argc, char **argv) {
  {
    ygm::comm world(&argc, &argv);

    int     log_global_table_size{atoi(argv[1])};
    int64_t local_updates{atoll(argv[2])};
    int     num_trials{atoi(argv[3])};

    uint64_t global_table_size = ((uint64_t)1) << log_global_table_size;
    // world.cout0("Creating global table with size ", global_table_size);
    if (world.rank0()) {
      std::cout << world.layout().node_size() << ", "
                << world.layout().local_size()
                << ", " /*<< ygm_buffer_capacity*/
                << ", " /*<< world.routing_protocol()*/
                << ", " << global_table_size << ", "
                << local_updates * world.size();
    }

    ygm::container::array<uint64_t> arr(world, global_table_size);

    std::mt19937                            gen(world.rank());
    std::uniform_int_distribution<uint64_t> dist(0, global_table_size - 1);

    double total_update_time{0.0};

    for (int trial = 0; trial < num_trials; ++trial) {
      std::vector<uint64_t>           indices;
      distributed_rmat_edge_generator rmat(world, log_global_table_size,
                                           local_updates * world.size(), trial);

      rmat.for_all([&indices](const auto first, const auto second) {
        indices.push_back(first);
      });

      world.barrier();

      ygm::timer update_timer{};

      for (const auto &index : indices) {
        arr.async_visit(index, [](const auto i, auto v) { ++v; });
      }

      world.barrier();

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
  }

  return 0;
}
