// Copyright 2019-2021 Lawrence Livermore National Security, LLC and other YGM
// Project Developers. See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: MIT

#include <ygm/comm.hpp>
#include <ygm/utility.hpp>

static int num_trips;
static int curr_trip = 0;

struct around_the_world_functor {
 public:
  template <typename Comm>
  void operator()(Comm *world) {
    if (curr_trip < num_trips) {
      world->async((world->rank() + 1) % world->size(),
                   around_the_world_functor());
      ++curr_trip;
    }
  }
};

int main(int argc, char **argv) {
  ygm::comm world(&argc, &argv);

  num_trips = atoi(argv[1]);
  int num_trials{atoi(argv[2])};

  if (world.rank0()) {
    std::cout << world.layout().node_size() << ", "
              << world.layout().local_size() << ", " << world.routing_protocol()
              << ", " << num_trips;
  }

  world.barrier();

  for (int trial = 0; trial < num_trials; ++trial) {
    curr_trip = 0;

    auto begin_stats = world.stats_snapshot();
    world.barrier();

    world.set_track_stats(true);

    ygm::timer trip_timer{};

    if (world.rank0()) {
      world.async(1, around_the_world_functor());
    }

    world.barrier();

    world.set_track_stats(false);

    double elapsed = trip_timer.elapsed();

    auto trial_stats = world.stats_snapshot() - begin_stats;

    // world.cout0("Went around the world ", num_trips, " times in ", elapsed,
    //" seconds\nAverage trip time: ", elapsed / num_trips);

    size_t message_bytes{0};
    size_t header_bytes{0};

    for (const auto &bytes : trial_stats.get_destination_message_bytes()) {
      message_bytes += bytes;
    }
    for (const auto &bytes : trial_stats.get_destination_header_bytes()) {
      header_bytes += bytes;
    }

    message_bytes = world.all_reduce_sum(message_bytes);
    header_bytes  = world.all_reduce_sum(header_bytes);

    if (world.rank0()) {
      auto total_hops = num_trips * world.size();
      std::cout << ", " << elapsed << ", " << total_hops / elapsed << ", "
                << header_bytes << ", " << message_bytes << ", "
                << trial_stats.get_all_reduce_count();
    }
  }

  if (world.rank0()) {
    std::cout << std::endl;
  }

  return 0;
}
